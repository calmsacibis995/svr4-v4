/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_EMAP_H
#define _SYS_EMAP_H
#ident	"@(#)head.sys:sys/emap.h	1.1.2.4"
/*
 *	Copyright (C) The Santa Cruz Operation, 1986, 1987, 1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */

/* #ident	"@)#(head.sys:emap.h	1.3" */

/* Channel mapping ioctl's */
/*	LDIOC  ('D'<< 8) */	/* defined in termio.h */

							/* BEGIN SCO_INTL */
/* Channel Mapping ioctl command definitions */
#define LDSMAP (LDIOC|10)
#define LDGMAP (LDIOC|11)
#define LDNMAP (LDIOC|12)

/* Emapping state (t_mstate) */
#define	ES_NULL		0	/* Mapping not enabled */
#define	ES_START	1	/* Base mapping state */
#define	ES_DEAD		2	/* Dead key received */
#define	ES_COMP1	3	/* Compose key received */
#define	ES_COMP2	4	/* Compose and 1st following keys received */
#define	EMBSHIFT	10	/* log2 BSIZE (E_TABSZ 1K) */
#define	EMBMASK		01777	/* E_TABSZ - 1 */

#define	NEMBUFS	10		/* Max number of buffers for mapping table */

typedef	struct emtab	*emp_t;
typedef	struct emind	*emip_t;
typedef	struct emout	*emop_t;
typedef	unsigned char	*emcp_t;

extern	emcp_t	emmapout();

/* Emap control structure */
struct emap {
	emp_t	e_tp[NEMBUFS];		/* table of ptrs to mapping tables */
	struct	buf *e_bp;		/* buf hdr for mapping tables */
	short	e_count;		/* use count */
	short	e_ndind;		/* number of dead indexes */
	short	e_ncind;		/* number of compose indexes */
	short	e_nsind;		/* number of string indexes */
};

extern struct emap emap[];		/* allocated in space.h */


/* Emapping tables structures */

struct emind {
	unsigned char	e_key;
	unsigned char	e_ind;
};

struct emout {
	unsigned char	e_key;
	unsigned char	e_out;
};

struct emtab {
	unsigned char	e_imap[256];	/* 8-bit  input map */
	unsigned char	e_omap[256];	/* 8-bit output map */
	unsigned char	e_comp;		/* compose key */
	unsigned char	e_beep;		/* beep on error flag */
	short		e_cind;		/* offset of compose indexes */
	short		e_dctab;	/* offset of dead/compose table */
	short		e_sind;		/* offset of string indexes */
	short		e_stab;		/* offset of string table */
	struct emind	e_dind[1];	/* start of dead key indexes */
};

struct emp_tty {
	char	t_mstate;	/* emapping state */
	unsigned char	t_mchar;/*saved emapping char */
	char	t_merr;		/* emapping error flag */
	char	t_xstate;	/* extended state */
	struct xmap	*t_xmp;  /*ptr to extended struct */
	unsigned char	t_schar;/* save timeout char instead of using lflag */
	char	t_yyy[3];	/* reserved */
};

#define	E_TABSZ		1024		/* size of an emtab */

#define	ESTRUCTOFF(structure, field)	(int) &(((struct structure *)0)->field)
#define	E_DIND		(ESTRUCTOFF(emtab, e_dind[0]))
#define	E_ESC		'\0'		/* key maps to dead/compose/string */

							/* END SCO_INTL */
#endif /* _SYS_EMAP_H */
