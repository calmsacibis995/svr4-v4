/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifndef _VM_SEG_DEV_H
#define _VM_SEG_DEV_H

#ident	"@(#)kern-vm:seg_dev.h	1.3"

/*
 * Structure who's pointer is passed to the segvn_create routine
 */
struct segdev_crargs {
	int	(*mapfunc)();	/* map function to call */
	u_int	offset;		/* starting offset */
	dev_t	dev;		/* device number */
	u_char	prot;		/* protection */
	u_char	maxprot;	/* maximum protection */
};

/*
 * (Semi) private data maintained by the seg_dev driver per segment mapping
 */
struct	segdev_data {
	int	(*mapfunc)();	/* really returns struct pte, not int */
	u_int	offset;		/* device offset for start of mapping */
	struct	vnode *vp;	/* Vnode associated with device */
	u_char	pageprot;	/* true if per page protections present */
	u_char	prot;		/* current segment prot if pageprot == 0 */
	u_char	maxprot;	/* maximum segment protections */
	struct	vpage *vpage;	/* per-page information, if needed */
};

#ifdef _KERNEL

#if defined(__STDC__)
extern int segdev_create(struct seg *, void *);
#else
extern int segdev_create();
#endif

#endif /* _KERNEL */

#endif	/* _VM_SEG_DEV_H */
