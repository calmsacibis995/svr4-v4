/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_chdir.c	1.1.1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)p_chdir.c	4.2	LCC);	/* Modified: 17:06:12 8/15/89 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include <ctype.h>
#include <errno.h>
#include "pci_types.h"

extern	void
	getcwd();		/* Update current directory working string */

extern	int errno;		/* contains error code from system calls */

void
pci_chdir(dos_dir, addr, request)
    char	*dos_dir;		/* DOS Target directory */
    struct	output	*addr;
    int		request;		/* DOS request number */
{
	register char *p;
	extern char *strrchr();
	char directory[MAX_PATH];	/* UNIX version of directory */


	/* Translate MS-DOS directory to UNIX */
	if (cvt_fname_to_unix(MAPPED, dos_dir, directory)) {
		addr->hdr.res = PATH_NOT_FOUND;
		return;
	}

	/* this is dumb, but DOS doesn't like doing chdirs to places ending
	*  with backslashes
	*/

	if ( ((p = strrchr(directory,'/')) && (p != directory) && (*++p == 0))
		|| (*directory == 0) ) {
		addr->hdr.res = PATH_NOT_FOUND;
		return;
	}


    /* Update current working directory string */
	if (chdir(directory)) {
		err_handler(&addr->hdr.res, request, directory);
		return;
	}

    /* Construct new cwd and store in MS-DOS form */
	getcwd(directory, CurDir);	

/*
 * Becuase MS-DOS fills in the DPB with the current working directory
 * we must get the current directory and send it back to the bridge.
 * The mapped mode is used, because we want it to look like a standard
 * MS-DOS entry.
 */

	pci_pwd(MAPPED, addr);
}

