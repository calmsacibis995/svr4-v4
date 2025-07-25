/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_XMAP_H
#define _SYS_XMAP_H
#ident	"@(#)head.sys:sys/xmap.h	1.1.1.1"
/*
 *	Copyright (C) The Santa Cruz Operation, 1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */

							/* BEGIN SCO_INTL */
/*
 *	xmap structure - provides a per-tty structure for recording
 *	state information by emap and nmap routines.  Also used by
 *	select to avoid cluttering up tty structure.
 */

struct xmap {
	/*
	 *	select fields
	 */
	struct proc *	xm_selrd;	/* Process waiting on selwait (read) */
	struct proc *	xm_selwr;	/* Process waiting on selwait (write)*/
	/*
 	 *	emap fields (used to be in tty structure)
	 */
	struct emap *	xm_emp;		/* emapping table */
	unsigned char	xm_emchar;	/* saved emapping char */
	unsigned char	xm_emonmap;	/* True if should call nmmapout */
	/*
	 *	nmap fields
	 */
	struct nxmap *	xm_nmp;		/* nmapping table */
#ifdef i386
	char *		xm_nmiseqp;	/* Current pos in lead-in (input) */
	char *		xm_nmoseqp;	/* Current pos in lead-in (output) */
#endif
	unsigned short	xm_nmincnt;	/* # chars left in trailer (input) */
	unsigned short	xm_nmoncnt;	/* # chars left in trailer (output) */
	unsigned char	xm_nmiseqn;	/* Index of current lead-in (input) */
	unsigned char	xm_nmoseqn;	/* Index of current lead-in (output) */
};
							/* END SCO_INTL */

#endif	/* _SYS_XMAP_H */
