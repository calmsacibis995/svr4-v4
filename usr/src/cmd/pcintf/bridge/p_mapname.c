/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_mapname.c	1.1.1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)p_mapname.c	4.3	LCC);	/* Modified: 17:25:38 10/26/89 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "pci_types.h"

/*
 * The MAPFILE function maps a dos filename to the corresponding unix
 * filename if mode is zero and performs the inverse operation if mode   
 * is one.  There is an extended IOCTL function on the bridge that makes    
 * use of this service.
 *    
 */

void
pci_mapname(name, mode, addr)
char *name;
int mode;
struct output *addr;
{
    char nametemp[MAX_PATH];
    register char *np;
    int flag;
    
    if (mode) { /* map unix name to dos name */
	cvt_to_unix(name, nametemp);
	/*
	 *  The bridge sends the pathname with backslashes,
	 *  so convert them to forward slashes.
	 */
	for (np = nametemp; *np; np++) {
	    if (*np == '\\')
		*np = '/';
	}
 	flag = cvt_fname_to_dos(MAPPED, CurDir, nametemp, addr->text);
    }
    else { /* map dos name to unix name */
	flag = cvt_fname_to_unix(MAPPED, name, nametemp);
	cvt_to_dos(nametemp, addr->text);
    }
    Vlog(("  out: %s  flag: %d\n", "%s %d\n", addr->text, flag));
    if (flag == 0) {
	addr->hdr.t_cnt = strlen(addr->text) + 1;
	addr->hdr.res = SUCCESS;
    }
    else {
	addr->hdr.t_cnt = 0;
	addr->hdr.res = FILE_NOT_FOUND; 
    }
}
