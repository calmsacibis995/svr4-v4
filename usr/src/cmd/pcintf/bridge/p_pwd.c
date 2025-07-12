/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_pwd.c	1.1.1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)p_pwd.c	4.2	LCC);	/* Modified: 8/15/89 17:29:41 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "pci_types.h"

#include	<string.h>
#include	"pw_gecos.h"

extern	long atol();	


void
pci_pwd(mode, addr)
int mode;
    struct	output	*addr;		/* Pointer to response buffer */
{
    register	int
	datalength;			/* Length of response string */

    char
	directory[MAX_PATH];		/* String contains directory name */

	directory[0] = '\0';
	directory[1] = '\0';
	cvt_fname_to_dos(mode, CurDir, CurDir, directory);

	datalength = strlen(directory);

	/* Fill response buffer */
	/* DOS working directory name does not include leading slash */
	(void) strcpy(addr->text, &directory[1]);
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;
	addr->hdr.t_cnt = datalength ? datalength : 1;
}

#ifdef	LOCUS
/*
 *	parseGecos() - Routine to parset the pw_gecos field in the
 * 		       struct passwd and put the subfields in struct 
 *		       struct pw_gecos;
 *
 *	Format of the pw_gecos field is:
 *		useName/fileLimit;siteInfo;siteAccessPerm
 */
struct pw_gecos *
parseGecos(gecosPtr) 
char	 *gecosPtr;
{
static struct pw_gecos theGecos = 
	{(char *)NULL, 0L, (char *)NULL, (char *)NULL};
char	*nameFLimit;

	/* Get User Name and File Limit, siteInfo and Access Perm */
	nameFLimit = strtok(gecosPtr, ";");
	if (nameFLimit != (char *)NULL)
	    theGecos.siteInfo = strtok((char *)NULL, ";");
	if (theGecos.siteInfo != (char *)NULL)
	    theGecos.siteAccessPerm = strtok((char *)NULL, ";");
	theGecos.userName = strtok(nameFLimit, "/");
	if (theGecos.userName != (char *)NULL) 
	    theGecos.fileLimit = atol(strtok((char *)NULL, "/"));

	/* Set the NULL strings to \0 */
	if (theGecos.userName == (char *)NULL)
	    theGecos.userName = "\0";
	if (theGecos.siteInfo == (char *)NULL)
	    theGecos.siteInfo = "\0";
	if (theGecos.siteAccessPerm == (char *)NULL)
	    theGecos.siteAccessPerm = "\0";
	return &theGecos;
};

#endif	/* LOCUS */
