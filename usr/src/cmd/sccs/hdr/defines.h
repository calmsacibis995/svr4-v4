/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:hdr/defines.h	6.8"
# include	<sys/types.h>
# include	<stdio.h>
# include	<macros.h>
# include	<fatal.h>
# include	<time.h>

/*
 */

# define CTLSTR		"%c%c\n"

# define CTLCHAR	1
# define HEAD		'h'

# define STATS		's'

# define BDELTAB	'd'
# define INCLUDE	'i'
# define EXCLUDE	'x'
# define IGNORE		'g'
# define MRNUM		'm'
# define COMMENTS	'c'
# define EDELTAB	'e'

# define BUSERNAM	'u'
# define EUSERNAM	'U'

# define NFLAGS	26

# define FLAG		'f'
# define NULLFLAG	'n'
# define JOINTFLAG	'j'
# define DEFTFLAG	'd'
# define TYPEFLAG	't'
# define VALFLAG	'v'
# define CMFFLAG	'z'
# define BRCHFLAG	'b'
# define IDFLAG		'i'
# define MODFLAG	'm'
# define FLORFLAG	'f'
# define CEILFLAG	'c'
# define QSECTFLAG	'q'
# define LOCKFLAG	'l'
# define ENCODEFLAG	'e'

# define BUSERTXT	't'
# define EUSERTXT	'T'

# define INS		'I'
# define DEL		'D'
# define END		'E'

# define MINR		1		/* minimum release number */
# define MAXR		 9999	/* maximum release number */
# define FILESIZE	1020
# define MAXLINE	1024
# define MAX		9999
# define DELIVER	'*'
# define LOGSIZE	(16)
# define FAILPUT    fatal("fputs could not write to file (ut13)")
/*
	Declares for external subroutines and/or functions
*/

extern	char	*sname();
extern	char	*cat();
extern	char	*dname();
extern	char	*repeat();
extern	char	*satoi();
extern	char	*strend();
extern	char	*trnslat();
extern	char	*zero();
extern	char	*zeropad();

/*
	SCCS Internal Structures.
*/

struct apply {
	char	a_inline;	/* in the line of normally applied deltas */
	int	a_code;		/* APPLY, NOAPPLY or SX_EMPTY */
	int	a_reason;
};
#define APPLY	  (1)
#define NOAPPLY  (-1)
#define SX_EMPTY	  (0)

# define IGNR		0100
# define USER		040
# define INCL		1
# define EXCL		2
# define CUTOFF		4
# define INCLUSER	(USER | INCL)
# define EXCLUSER	(USER | EXCL)
# define IGNRUSER	(USER | IGNR)


struct queue {
	struct queue *q_next;
	int    q_sernum;	/* serial number */
	char    q_keep;		/* keep switch setting */
	char	q_iord;		/* INS or DEL */
	char	q_ixmsg;	/* caused inex msg */
	char	q_user;		/* inex'ed by user */
};
#define YES	 (1)
#define NO	(-1)


struct	sid {
	int	s_rel;
	int	s_lev;
	int	s_br;
	int	s_seq;
};


struct	deltab {
	struct	sid	d_sid;
	int	d_serial;
	int	d_pred;
	long	d_datetime;
	char	d_pgmr[LOGSIZE];
	char	d_type;
};

struct	ixg {
	struct	ixg	*i_next;
	char	i_type;
	char	i_cnt;
	int	i_ser[1];
};


struct	idel {
	struct	sid	i_sid;
	struct	ixg	*i_ixg;
	int	i_pred;
	long	i_datetime;
};


# define maxser(pkt)	((pkt)->p_idel->i_pred)
# define sccsfile(f)	imatch("s.", sname(f))

struct packet {
	char	p_file[FILESIZE];	/* file name containing module */
	struct	sid	p_reqsid;	/* requested SID, then new SID */
	struct	sid	p_gotsid;	/* gotten SID */
	struct	sid	p_inssid;	/* SID which inserted current line */
	char	p_verbose;	/* verbose flags (see #define's below) */
	char	p_upd;		/* update flag (!0 = update mode) */
	long	p_cutoff;	/* specified cutoff date-time */
	int	p_ihash;	/* initial (input) hash */
	int	p_chash;	/* current (input) hash */
	int	p_nhash;	/* new (output) hash */
	int	p_glnno;	/* line number of current gfile line */
	int	p_slnno;	/* line number of current input line */
	char	p_wrttn;		/* written flag (!0 = written) */
	char	p_keep;		/* keep switch for readmod() */
	struct	apply	*p_apply;	/* ptr to apply array */
	struct	queue	*p_q;	/* ptr to control queue */
	FILE	*p_iop;		/* input file */
	char	p_buf[BUFSIZ];	/* input file buffer */
	char	p_line[BUFSIZ];	/* buffer for getline() */
	long	p_cdt;		/* date/time of newest applied delta */
	char	*p_lfile;	/* 0 = no l-file; else ptr to l arg */
	struct	idel	*p_idel;	/* ptr to internal delta table */
	FILE	*p_stdout;	/* standard output for warnings and messages */
	FILE	*p_gout;	/* g-file output file */
	char	p_user;		/* !0 = user on user list */
	char	p_chkeof;	/* 0 = eof generates error */
	int	p_maxr;		/* largest release number */
	int	p_ixmsg;	/* inex msg counter */
	int	p_reopen;	/* reopen flag used by getline on eof */
	int	p_ixuser;	/* HADI | HADX (in get) */
	int	do_chksum;	/* for getline(), 1 = do check sum */
};
/*
	Masks for p_verbose
*/

# define RLACCESS	(1)
# define NLINES		(2)
# define DOLIST		(4)
# define UNACK		(8)
# define NEWRL		(16)
# define WARNING	(32)


struct	stats {
	int	s_ins;
	int	s_del;
	int	s_unc;
};


struct	pfile	{
	struct	sid	pf_gsid;
	struct	sid	pf_nsid;
	char	pf_user[LOGSIZE];
	long	pf_date;
	char	*pf_ilist;
	char	*pf_elist;
	char 	*pf_cmrlist;
};


# define RESPSIZE	1024
# define NVARGS	64
# define VSTART 3

/*
**	The following five definitions (copy, xfopen, xfcreat, remove,
**	USXALLOC) are taken from macros.h 1.1
*/

# define copy(srce,dest)	cat(dest,srce,0)
# define xfopen(file,mode)	fdfopen(xopen(file,mode),mode)
# define xfcreat(file,mode)	fdfopen(xcreat(file,mode),1)
# define remove(file)		xunlink(file)
# define USXALLOC() \
		char *alloc(n) {return((char *)xalloc((unsigned)n));} \
		free(n) char *n; {xfree(n);} \
		char *malloc(n) unsigned n; {int p; p=xalloc(n); \
			return((char *)(p != -1?p:0));}

