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

#ident	"@(#)kern-fs:ufs/ufs_tables.c	1.3"

#include <sys/types.h>
#include <sys/param.h>

/*
 * Bit patterns for identifying fragments in the block map
 * used as ((map & around) == inside)
 */
int around[9] = {
	0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff, 0x1ff, 0x3ff
};
int inside[9] = {
	0x0, 0x2, 0x6, 0xe, 0x1e, 0x3e, 0x7e, 0xfe, 0x1fe
};

/*
 * Given a block map bit pattern, the frag tables tell whether a
 * particular size fragment is available. 
 *
 * used as:
 * if ((1 << (size - 1)) & fragtbl[fs->fs_frag][map] {
 *	at least one fragment of the indicated size is available
 * }
 *
 * These tables are used by the scanc instruction on the VAX to
 * quickly find an appropriate fragment.
 */
u_char fragtbl124[256] = {
	0x00, 0x16, 0x16, 0x2a, 0x16, 0x16, 0x26, 0x4e,
	0x16, 0x16, 0x16, 0x3e, 0x2a, 0x3e, 0x4e, 0x8a,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x2a, 0x3e, 0x3e, 0x2a, 0x3e, 0x3e, 0x2e, 0x6e,
	0x3e, 0x3e, 0x3e, 0x3e, 0x2a, 0x3e, 0x6e, 0xaa,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x26, 0x36, 0x36, 0x2e, 0x36, 0x36, 0x26, 0x6e,
	0x36, 0x36, 0x36, 0x3e, 0x2e, 0x3e, 0x6e, 0xae,
	0x4e, 0x5e, 0x5e, 0x6e, 0x5e, 0x5e, 0x6e, 0x4e,
	0x5e, 0x5e, 0x5e, 0x7e, 0x6e, 0x7e, 0x4e, 0xce,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x7e,
	0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x7e, 0xbe,
	0x2a, 0x3e, 0x3e, 0x2a, 0x3e, 0x3e, 0x2e, 0x6e,
	0x3e, 0x3e, 0x3e, 0x3e, 0x2a, 0x3e, 0x6e, 0xaa,
	0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x7e,
	0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x7e, 0xbe,
	0x4e, 0x5e, 0x5e, 0x6e, 0x5e, 0x5e, 0x6e, 0x4e,
	0x5e, 0x5e, 0x5e, 0x7e, 0x6e, 0x7e, 0x4e, 0xce,
	0x8a, 0x9e, 0x9e, 0xaa, 0x9e, 0x9e, 0xae, 0xce,
	0x9e, 0x9e, 0x9e, 0xbe, 0xaa, 0xbe, 0xce, 0x8a,
};

u_char fragtbl8[256] = {
	0x00, 0x01, 0x01, 0x02, 0x01, 0x01, 0x02, 0x04,
	0x01, 0x01, 0x01, 0x03, 0x02, 0x03, 0x04, 0x08,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x02, 0x03, 0x03, 0x02, 0x04, 0x05, 0x08, 0x10,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x05, 0x09,
	0x02, 0x03, 0x03, 0x02, 0x03, 0x03, 0x02, 0x06,
	0x04, 0x05, 0x05, 0x06, 0x08, 0x09, 0x10, 0x20,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x05, 0x09,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x03, 0x03, 0x03, 0x03, 0x05, 0x05, 0x09, 0x11,
	0x02, 0x03, 0x03, 0x02, 0x03, 0x03, 0x02, 0x06,
	0x03, 0x03, 0x03, 0x03, 0x02, 0x03, 0x06, 0x0a,
	0x04, 0x05, 0x05, 0x06, 0x05, 0x05, 0x06, 0x04,
	0x08, 0x09, 0x09, 0x0a, 0x10, 0x11, 0x20, 0x40,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x05, 0x09,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x03, 0x03, 0x03, 0x03, 0x05, 0x05, 0x09, 0x11,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x05, 0x09,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x07,
	0x05, 0x05, 0x05, 0x07, 0x09, 0x09, 0x11, 0x21,
	0x02, 0x03, 0x03, 0x02, 0x03, 0x03, 0x02, 0x06,
	0x03, 0x03, 0x03, 0x03, 0x02, 0x03, 0x06, 0x0a,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x07,
	0x02, 0x03, 0x03, 0x02, 0x06, 0x07, 0x0a, 0x12,
	0x04, 0x05, 0x05, 0x06, 0x05, 0x05, 0x06, 0x04,
	0x05, 0x05, 0x05, 0x07, 0x06, 0x07, 0x04, 0x0c,
	0x08, 0x09, 0x09, 0x0a, 0x09, 0x09, 0x0a, 0x0c,
	0x10, 0x11, 0x11, 0x12, 0x20, 0x21, 0x40, 0x80,
};

/*
 * The actual fragtbl array.
 */
u_char *fragtbl[MAXFRAG + 1] = {
	0, fragtbl124, fragtbl124, 0, fragtbl124, 0, 0, 0, fragtbl8,
};
