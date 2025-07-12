/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_mkdir.c	1.1.1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)p_mkdir.c	4.2	LCC);	/* Modified: 17:27:56 8/15/89 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "pci_types.h"

#include	<string.h>


void
pci_mkdir(dos_dir, addr, request)
    char	*dos_dir;		/* Name of directory to create */
    struct	output	*addr;		/* Pointer to response buffer */
    int		request;		/* Number of DOS request simulated */
{
    char directory[MAX_PATH];

    /* Translate MS-DOS to UNIX directory name */
    if (cvt_fname_to_unix(MAPPED, dos_dir, directory)) {
	addr->hdr.res = FILE_EXISTS;
	return;
    }

    if (mkdir(directory, 0777) < 0)	/* mode modified by umask */
	err_handler(&addr->hdr.res, request, directory);
    else
	addr->hdr.res = SUCCESS;
    addr->hdr.stat = NEW;
}
