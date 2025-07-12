/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_chmod.c	1.1.1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)p_chmod.c	4.2	LCC);	/* Modified: 17:06:31 8/15/89 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include	"pci_types.h"
#include	<errno.h>
#include	<stat.h>

/*				External Routines			*/


extern int 
	errno,
	swap_in(),		/* Causes a virtual descriptor to be paged in */
	attribute();		/* Set attribute bits in output frame */


void
pci_chmod(in, out)
struct input
	*in;				/* Request packet */
struct output
	*out;				/* Response packet */
{
char
	fileName[MAX_PATH];			/* Name of file to chmod */
struct stat
	fileStat;			/* Unix file info */
int
	dosAttr = 0,			/* Returned DOS file attribute */
	newMode;			/* New Unix file modes */

	out->hdr.stat = NEW;
	out->hdr.res = SUCCESS;

	/* Massage MS-DOS pathname */
	if (cvt_fname_to_unix(MAPPED, in->text, fileName)) {
		out->hdr.res = ACCESS_DENIED;
		return;
	}

	if (stat(fileName, &fileStat) < 0) {
		err_handler(&out->hdr.res, CHMOD, fileName);
		return;
	}

	/* mode != 0 ==> set modes otherwise get modes */
	if (in->hdr.mode != 0) {
		/* note: do not complain about archive bit */
		if (in->hdr.attr & (SYSTEM | VOLUME_LABEL | SUB_DIRECTORY)) {
			out->hdr.res = ACCESS_DENIED;
			return;
		}

		if (in->hdr.attr & READ_ONLY)
			newMode = fileStat.st_mode & ~ALL_WRITE;
		else
			newMode = fileStat.st_mode | O_WRITE;

		if (chmod(fileName, newMode) < 0) {
			err_handler(&out->hdr.res, CHMOD, fileName);
			return;
		}
	} else
		out->hdr.attr = attribute(&fileStat);

	return;
}
