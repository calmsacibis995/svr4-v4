/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_NMAP_H
#define _SYS_NMAP_H
#ident	"@(#)head.sys:sys/nmap.h	1.1.1.1"
/*
 *	Copyright (C) The Santa Cruz Operation, 1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
							/* BEGIN SCO_INTL */
/* Channel mapping ioctl's */

#define	NMIOC  ('n'<< 8)	/* So not to conflict with LDIOCs */
#define NMSMAP (NMIOC|1)	/* set no-map */
#define NMGMAP (NMIOC|2)	/* get no-map */
#define NMNMAP (NMIOC|3)	/* clear no-map */

#ifdef i386
typedef	struct nmtab	*nmp_t;
typedef	struct nmseq	*nmsp_t;
typedef	unsigned char	*nmcp_t;
#endif

/* Nmap control structure */

struct nxmap {
	unsigned char	n_count;	/* Usage count of this map */
	nmp_t		n_p;		/* Ptr. to sequence table */
	struct buf	*n_bp;		/* Ptr. to table buffer header */
};

/* Nmap sequence table structs */

struct nmtab {
	unsigned char	n_iseqs;	/* Number of input sequences */
	unsigned char	n_aseqs;	/* Number of input + output sequences */
	short		n_seqidx[1];	/* Array of sequence offsets */
};

struct nmseq {
	unsigned char	n_nmcnt;	/* Sequence trailer length */
	char		n_nmseq[1];	/* Sequence lead-in(null terminated) */
};

extern struct nxmap nxmap[];
							/* END SCO_INTL */

#endif	/* _SYS_NMAP_H */
