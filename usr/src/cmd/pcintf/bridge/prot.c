/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/prot.c	1.1"
#include	"system.h"
#include 	"sccs.h"
SCCSID(@(#)prot.c	4.4	LCC);	/* Modified: 13:06:36 11/13/89 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/* prot.c   copy protection routines for point-to-point PCI */

#ifdef	AIX_RT
#include "pci_types.h"
#include "const.h"
#else	/* !AIX_RT */
#include "const.h"
#include "pci_types.h"
#endif	/* AIX_RT */
#include "utmp.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#ifdef	LOCUS
#include <sf.h>	
#endif	LOCUS
#include <stat.h>

#ifdef	POINT_TO_POINT
#define PROTSUFFIX	"/pci/prot"
#define SUFFIXLEN	9
#define seekMe() 	{if (lseek(myfd, myoffset, 0)<0)\
			fatal(lmf_format_string((char *) NULL, 0, \
				lmf_get_message("PROT1",\
				"Can't lseek to my record, errno=%1\n"), \
				"%d", errno));\
			}


/* arguments to dupSearch() */
#define FLAG 0	/* flag duplicates */
#define TERM 1	/* mark duplicates for termination */

extern long lseek(), time();
extern unsigned alarm();
extern char *malloc();
extern int errno;
extern xmtPacket();		/* trasnmit routine */
extern struct ni2 ndata;	/* net info (dummy in this case) */
extern int swap_how;		/* byte order flag */
extern char *mydevname;

#ifdef RS232_COPY_PROTECTION
int	rs232_violation = FALSE;	/* checked in xmtPacket */
#endif

#ifndef SYS5
extern char *rindex(), *ttyname();
#else
extern struct utmp *getutent();	/* sys5 utmp utilities */
extern void endutent();
#endif
#ifdef	LOCUS
char *protFile();		/* returns a ptr to the prot file path */
#endif

struct protrec {
	char serial[SERIALSIZE];
	char chk_flg;
	char chk_serial[SERIALSIZE];
	time_t	starttime;
};

static int myfd;			/* file desc for protection file */
static long myoffset;			/* file offset of my record */
static time_t my_stime;			/* start time of my server */
extern char myserial[];			/* serial number of my bridge */
int mypid;		/* my process id */
int myppid;		/* my parent's pid */
struct protrec zeroes;
#define CHK_SIZE	(sizeof zeroes.chk_flg)
static struct output out;

#ifndef SYS5
/* Ersatz utmp routines a la sys5.  These meet the needs of the following */
/* copy protection code, but do not fully simulate the sys5 routines */
/* getutent() returns a Berkelyesque utmp struct, not a sys 5 struct */

static int utmpfd = -1;	/* file descriptor from utmp open */

static struct utmp *
getutent()
{
	static struct utmp ut;

	if (utmpfd < 0)
	{	if ((utmpfd=open("/etc/utmp",0))<0)
			return(NULL);
	}
	if (read(utmpfd, &ut, sizeof(struct utmp))<sizeof(struct utmp))
		return(NULL);
	else
		return(&ut);
}

static void
endutent()
{	close(utmpfd); }

#endif /* !SYS5 */

/* look through /etc/utmp to find which record I should use, compute */
/* my offset in the protection file, and seek to it */
/* Under sys5, we match on pid; otherwise on terminal name */
void
find_rec()
{
	int rec_count=0;	/* index for my record, from /etc/utmp */
	struct utmp *ut;	/* utmp entry from getutent() */
	int found = FALSE;	/* boolean, TRUE => I found mmy record */
#ifndef SYS5
	char *ttyn, *tail;

	if ((ttyn = ttyname(0))==NULL)
		fatal(lmf_get_message("PROT2","Stdin not a tty!\n"));
	else
		log("got ttyname %s\n",ttyn);
	tail = rindex(ttyn, '/');
	if (tail)
		ttyn=tail+1;
	while((ut=getutent())!=NULL)
	{	if (!strncmp(ut->ut_line, ttyn, sizeof(ut->ut_line)))
#else
	while((ut=getutent())!=NULL)
 { if ((ut->ut_pid == mypid) ||
      ((myppid != 1) && (ut->ut_pid == myppid)) )
#endif !SYS5
		{	found=TRUE;
log("findrec:utpid %d, mypid %d, myppid %d\n",ut->ut_pid,mypid,myppid);
			break;
		}
		else
			++rec_count;
	}
	endutent();
	if (found==FALSE)
		fatal(lmf_get_message("PROT3",
			"Can't find my entry in /etc/utmp\n"));
	else
	{	myoffset = rec_count * sizeof(struct protrec);
		log("myoffset 0x%x\n",myoffset);
		seekMe();
	}
}

/* initialization- open file (create if needed) and find my record */
/* file must be open for read and write, hence the re-open after a creat */
/* clear my record to zeroes */
void
protInit()
{

	int f1,f2,f3;	/* dummy file descriptors */


	mypid = getpid();
	myppid = getppid();

	for (f1=0;f1<SERIALSIZE;f1++) {
		zeroes.serial[f1] = zeroes.chk_serial[f1] = '0';
	}
	zeroes.chk_flg = '0';
	zeroes.starttime = (time_t) -1;		/* this means just a consvr */

/* this crud ensures that the next open won't give us stdin stdout or stderr */
	f1=open("/dev/null",0);
	f2=open("/dev/null",0);
	f3=open("/dev/null",0);
openfile:
#ifdef	LOCUS
	if ((myfd=open(protFile(),2))<0)
#else
#define	PROTFILE	"/usr/pci/prot"
	if ((myfd=open(PROTFILE,2))<0)
#endif
	{	if (errno == ENOENT)
#ifdef	LOCUS
		{	if ((myfd=creat(protFile(),0666))<0)
#else
		{	if ((myfd=creat(PROTFILE,0666))<0)
#endif
				fatal(lmf_format_string((char *) NULL, 0, 
				  lmf_get_message("PROT4",
				  "Can't create protection file, errno = %1\n"),
				  "%d", errno));
			else
			{	log("Created new protection file.\n");
				close(myfd);
				goto openfile;
			}
		}
		else
			fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("PROT5",
				"Can't open protection file, errno = %1\n"),
				"%d", errno));
	}		
	close(f1);
	close(f2);
	close(f3);
	find_rec();
	seekMe();
	if (write(myfd, &zeroes, sizeof zeroes)<sizeof zeroes)
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("PROT6",
			"Error clearing my record, errno=%1\n"),
			"%d", errno));
}


#ifdef	LOCUS
/*
 * protFile		This routine returns the ptr to a statically 
 *			allocated block that contains the path of the
 *			prot file for this server.
 */

char *
protFile()
{
	static char protfile[48];
	register char *cp, *pf = &protfile[0];

	if ((cp = sflocal(site(0L))) == NULL)	/* site() expects a long */
		fatal(lmf_get_message("PROT7",
			"protFile: cannot get local prot file path.\n"));
	endsf();

	strcpy(pf, cp);
	strcat(pf, PROTSUFFIX);
	return(&protfile[0]);
}
#endif

#ifndef	LOCUS
commit(x)
int x;
{ return 1; }
#endif

/* dupSearch() search for other records in the file which contain my */
/* serial number. For each one found, set its chk_flg, or mark */
/* it for termination, according to the argument "action" */
void
dupSearch(action)
int action;		/* flag, or terminate? */
{
	int readstat;		/* return status from read() */
 	struct protrec rec;	/* record read from prot file */

	if (lseek(myfd, 0L, 0)<0)
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("PROT8",
			"Can't lseek to top of file, errno=%1\n"),
			"%d", errno));
	while ((readstat=read(myfd, &rec, sizeof(rec))) == sizeof(rec))
	{	if (serialEq(rec.serial, myserial) &&
		    ((lseek(myfd, 0L, 1)-sizeof(rec)) != myoffset))
		{	if (action == FLAG)
				rec.chk_flg = '1';
			else
				serialCpy(rec.chk_serial,myserial);
			if (lseek(myfd, (long) -sizeof(rec), 1)<0)
				fatal(lmf_format_string((char *) NULL, 0, 
				  lmf_get_message("PROT9",
				  "Can't lseek to dup record, errno=%1\n"),
				  "%d", errno));
			if (write(myfd, &rec, sizeof(rec))<sizeof(rec))
				fatal(lmf_format_string((char *) NULL, 0, 
				  lmf_get_message("PROT10",
				  "Error flagging dup record, errno=%1\n"),
				  "%d", errno));
			if (commit(myfd) < 0)
			    fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("PROT11",
				"dupSearch: commit error, errno=%1\n"),
				"%d", errno));
		}
	}
	if (readstat !=0)
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("PROT12",
			"Read error in dupSearch(), errno=%1\n"),
			"%d", errno));
}

/* startup (CONNECT request) processing- write my serial num in my record */
/* and flag any duplicates */
void
protStart()
{
	my_stime = time((time_t *) 0);

	seekMe();
	if (write(myfd, myserial, SERIALSIZE) < SERIALSIZE)
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("PROT13",
			"Can't write my serial number, errno=%1\n"),
			"%d", errno));
	if (lseek(myfd, (long) (sizeof(struct protrec) -
		SERIALSIZE - sizeof(my_stime)), 1) < 0)
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("PROT14",
			"Can't seek to my start time, errno=%1\n"),
			"%d", errno));
	if (write(myfd, &my_stime, sizeof(time_t)) < sizeof(time_t))
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("PROT15",
			"Can't write my start time, errno=%1\n"),
			"%d", errno));
	if (commit(myfd) < 0)
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("PROT16",
			"protStart: commit error, errno=%1\n"),
			"%d", errno)); 
	dupSearch(FLAG);
}

/* killPc()  send a packet to crash the PC */
/* Called in the event of a security violation */
void
killPc()
{
	out.pre.select = UNSOLICITED;
	out.hdr.req = PC_CRASH;
	xmtPacket(&out, &ndata, swap_how);
}

/* poll-time processing- if my chk_flg is set, search and destroy */
/* duplicates. if my serial number is altered, or I am marked for */
/* termination, zap the PC and exit */
/* This routine is called from sig_catcher whenever we get a SIGALRM */
/* Since other parts of the server use alarm, we may be called */
/* unnecessarily (too soon), in which case we return immediately */

void
protPoll()
{
int i = 0; int j = 0;
	int readstat;			/* ret val from read() */
	struct protrec myrec;		/* copy of my record */
	static long lastpoll;
	long thispoll;

	time(&thispoll);
	if (thispoll-lastpoll < POLLINTERVAL)
		return;	
	else
		lastpoll = thispoll;
	seekMe();
	while ((readstat=read(myfd, &myrec, sizeof(myrec)))==sizeof(myrec))
	{	if (myrec.chk_flg != '0') {
#ifdef	LOCUS
			if (myrec.chk_flg == '3') /* caught w/it's pants down */
			{
				log("killing because chk_flg = 3\n");
				kill_session(&myrec);
			}
#endif
			if (lseek(myfd, myoffset+SERIALSIZE, 0)<0)
				fatal(lmf_format_string((char *) NULL, 0, 
				  lmf_get_message("PROT17",
				  "Can't lseek to my chk_flg, errno=%1\n"), 
				  "%d", errno));
			if (write(myfd, &zeroes.chk_flg, CHK_SIZE)< CHK_SIZE)
				fatal(lmf_format_string((char *) NULL, 0, 
					lmf_get_message("PROT18",
					"Can't clear my chk_flg, errno=%1\n"),
					"%d", errno));
			if (commit(myfd) < 0)
			    fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("PROT19",
				"protPoll: commit error, errno=%1\n"),
				"%d", errno));

			dupSearch(TERM);
			seekMe();
		}
		else
			break;
	}
	if (readstat != sizeof(myrec))
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("PROT20",
			"Error reading my record, errno=%1\n"),
			"%d", errno));

 if ((i = (!serialEq(myrec.serial, myserial))) ||
     (j = (serialEq(myrec.chk_serial, myserial))))
	{	
		log("killing because of serial check\n");
		kill_session(&myrec);

	}

#ifdef	LOCUS
	multisite_chk();
#endif

}


kill_session(myrec)
struct protrec *myrec;
{	
		char myser[SERIALSTR];
		char myrecser[SERIALSTR];
		char mychkser[SERIALSTR];

#ifndef	RS232PCI
		killPc();
#endif
		serialstr(myserial,myser);
	serialstr(myrec->serial,myrecser);
	serialstr(myrec->chk_serial,mychkser);
#ifdef RS232_COPY_PROTECTION
	log("protPoll:dev %s:%s:myser %s,myrecser %s,checkser %s\n",
	mydevname,"Copy protection violation",myser,myrecser,mychkser);
	rs232_violation = TRUE;		/* tell xmtPacket to kill PC */
#else
		fatal(lmf_format_string((char *) NULL, 0, 
		  lmf_get_message("PROT21",
		  "protPoll:dev %1:Copy protection violation:myser %2,myrecser %e,checkser %4\n"),
		  "%s%s%s%s", mydevname, myser,myrecser,mychkser));
#endif

}


serialstr(serial,str)
char serial[];
char str[];
{
	register int i;

	register char *s = str;
	char str1[3];

	for (i=0;i<SERIALSIZE;i++)
	{
		sprintf(str1,"%02x",serial[i] & 0xff);
		*s++=str1[0];
		*s++=str1[1];
	}
	*s++ = 0;
}

serial2str(serial,serstr)
char serial[];
char serstr[];
{
	char str1[3];
	register int i;
	register char *s=serstr;

	for (i=0;i<SERIALSIZE;i++) {
		sprintf(str1,"%02x",serial[i] & 0xff);
		*s++=str1[0];
		*s++=str1[1];
	}
	*s++=0;
}
str2serial(serstr,serial)
char serstr[];
char serial[];
{
	static char str1[3] = {0,0,0};
	register int i;
	register char *s = serstr;
	register char *p;

	for (i=0;i<SERIALSIZE;i++) {
		str1[0] = *s++;
		str1[1] = *s++;
		serial[i] = atoi(str1);
	}
}


#ifdef	LOCUS
/*
 * The following data structures and routines are for the multi-site version of
 * copy protection.
 */

 struct pp_site_tbl {
	sitestat_t	p_sstat;		/* site status */
	char		*p_lprot;		/* ptr to local prot file */
	time_t		p_ltm;			/* ltm for this site's file */
};

static struct pp_site_tbl  pp_tbl[MAXSITE];	/* table for all sites */
sitestat_t	sitep[MAXSITE];			/* buf for getsites call */


/*
 * multisite_chk		This routine goes through all the sites, except
 *				for the current site to check for a 'prot' file
 *				in the <LOCAL>/pci directory.  If the file was
 *				modified since it was last checked then it is 
 *				searched for a duplicate entry. (see routine
 *				srch_file.
 *
 *	Entry:
 *		None
 *
 *	Returns:
 *		0		Completion such that the caller doesn't care
 *		1		Duplicate record was found
 *
 */

multisite_chk()
{
	register struct pp_site_tbl	*tbl_ptr;
	register sitestat_t		*site_ptr;
	register int			i, my_site;


    /* first [re]build site status of the pp table. */

	if (getsites(sitep, MAXSITE) < 0) {
		log("pp_consvr: getsites error. errno: %d\n", errno);
		return(0);
	}
	site_ptr = sitep;	/* get ptr to copy to pp table */
	tbl_ptr = pp_tbl;
	for (i = 0; i < MAXSITE; i++, tbl_ptr++)	/* copy site status */
		tbl_ptr->p_sstat = *site_ptr++;

	tbl_ptr = &pp_tbl[1];	/* reset table ptr to start at site #1 */
	/*
	 * since we use the sf(3) routines in different places we need 
	 * the setf() and endsf() calls here.
	 */
	setsf();		/* reset site file */
	my_site = site(0L);	/* get my site number, make it a long */
	for (i = 1; i < MAXSITE; i++, tbl_ptr++) {    /* go through and check */
		if (i == my_site)
			continue;		/* ignore my site */
		if (tbl_ptr->p_sstat & GS_UP) {
			if (tbl_ptr->p_ltm == 0) 
				if (get_lprot(i, tbl_ptr))
					continue; /* error: same as site down */
			if (chk_file_time(tbl_ptr))
				if (srch_file(tbl_ptr))
					return(1);
		}
		else
			tbl_ptr->p_ltm = 0;	/* site down, clear ltm field */
	} /* end for */
	endsf();		/* close site file */
	return(0);
}


/*
 * get_lprot			This routine builds the correct path of the
 *				local prot file from the site number.  It
 *				updates the ptr to point to the path.
 *
 *	Entry:
 *		cptr		ptr to update with path.
 *		site		number of desired path.
 *
 *	Returns:
 *		1		Something went wrong. 
 *		0		Success
 *
 */

get_lprot(site, tptr)
register int site;
register struct pp_site_tbl *tptr;
{
	register char	*c, *s;

	if ((c = sflocal(site)) == NULL) {
		log("get_lprot: sflocal failed\n");
		return(1);
	}

	if (tptr->p_lprot)
		free(tptr->p_lprot);	/* free previously allocated storage */
	if ((s = malloc(strlen(c) + SUFFIXLEN + 1)) == NULL) {
		log("get_lprot: malloc failed, errno: %d\n", errno);
		return(1);
	}

	*s = 0x00;
	strcpy(s, c);
	tptr->p_lprot = strcat(s, PROTSUFFIX);
	return(0);
}


/*
 * chk_file_time		This routine checks the last time modified
 *				time of the particular site.  All the info
 *				we need is in the pp_prot_tbl stucture.
 *
 *	Entry:
 *		tptr		pointer to the desired site entry
 *
 *	Returns:
 *		1		a discrepency in file times found
 *		0		file times are the same (or error to
 *				the caller this means do nothing.)
 *
 */

chk_file_time(tptr)
register struct pp_site_tbl *tptr;
{
	struct stat	stat_buf;		/* buf for stat call */

	if (stat(tptr->p_lprot, &stat_buf) < 0) {
		log("chk_file_time: stat: errno: %d, path: %s\n", errno,
		    tptr->p_lprot);
		tptr->p_ltm = 0;		/* clear ltm field */
		return(0);
	}

	if (tptr->p_ltm == stat_buf.st_mtime)
		return(0);
	
	tptr->p_ltm = stat_buf.st_mtime;	/* set ltm */
	return(1);				/* different, tell caller */
	
}


/*
 * srch_file			This routine searches prot file specified by
 *				the passed pp_prot_tbl ptr.  It looks for a
 *				match by serial number.  If a match is found
 *				then the start times are compared.  The 
 *				newest server will be flagged with a special
 *				flag.  When that server reads through and finds
 *				it, it will terminate.
 *
 *	Entry:
 *		tptr		ptr to the specfied pp prot tbl entry
 *
 *	Returns:
 *		0		No action for the caller to take.
 *
 *		1		A matching record was found in which this
 *				server is the newest one.
 *
 */

srch_file(tptr)
register struct pp_site_tbl *tptr;
{
	register int fd;
	struct protrec cur_rec;

	if ((fd = open(tptr->p_lprot, O_RDWR)) < 0) 
		return(0);

	/* search through each record */
	while (read(fd, &cur_rec, sizeof(cur_rec)) == sizeof(cur_rec)) {
		if (serialEq(cur_rec.serial, myserial)) {
			if (cur_rec.starttime >= my_stime) { /* check stime */
				log("Possible violator, setting chk_flg to 3\n");
				cur_rec.chk_flg = '3';	/* special terminate */
				if (lseek(fd, (long) -sizeof(cur_rec), 1) < 0)
					continue;

				if (write(fd, &cur_rec, sizeof(cur_rec)) < 0) {
					log("srch_file: write to %s failed \
,errno: %d\n", tptr->p_lprot, errno);
					close(fd);
					return(0);
				}
			}
			close(fd);
			return(0);
		}
	}

	close(fd);
	return(0);
}

#endif	/* IX rel 2 multi-site */
#endif	POINT_TO_POINT
