/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_delete.c	1.1.1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)p_delete.c	4.3	LCC);	/* Modified: 16:22:14 8/18/89 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "pci_types.h"
#include "table.h"
#include <stat.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <xdir.h>

/*				External Routines			*/


extern  void getpath();         /* Extracts pathname portion from string */

extern  int match();            /* Match UNIX filename to DOS search pattern */
extern  int wildcard();         /* Determines if string has wildcards */
extern  int attribute();        /* Set attribute bits in output frame */

extern struct temp_slot temp_slot[];

extern int
	errno;

void
pci_delete(dos_fname, attr, addr, request)
    char	*dos_fname;		/* Name of file to unlink */
    int		attr;			/* MS-DOS file attribute */
    struct	output	*addr;		/* Address of response buffer */
    int		request;		/* DOS request number simulated */
{
    register	int
	    hflg,			/* Hidden attribute flag */
	    found = FALSE;		/* Loop flag */
    int     reg_found = FALSE;		/* loop flag */
    int     tslot;                      /* temp file slot number */

    char
	    filename[MAX_PATH],		/* File name converted to unix */
	    *dotptr,			/* Pointer to "." in file name */
	    *slashptr,			/* Pointer to last '/' in filename */
	    pipefilename[MAX_PATH],	/* File name of pipe */
	    pathcomponent[MAX_PATH],	/* Path component of filename */
	    mappedname[MAX_FILENAME],	/* mapped version of candidate
					*  directory entries
					*/
	    *namecomponent;		/* File component of filename */

    DIR
	    *dirdp;			/* Pointer to opendir() structure */

    struct	direct
	    *direntryptr;		/* Pointer to directory entry */

    unsigned short userId;
    struct stat statb;

	userId = getuid();

    /* Translate MS-DOS filename to UNIX filename */

    /* If unmapping filename returns collision return failure */
	if (cvt_fname_to_unix(UNMAPPED, dos_fname, filename)) {
	    addr->hdr.res = ACCESS_DENIED;
	    return;
	}

	/*** Kludge to allow access to protected directories for tmps ***/
	if ((tslot = redirdir(filename, 0)) >= 0) {
	    strcpy(filename, "/tmp/");
	    strcat(filename, temp_slot[tslot].fname);
	    temp_slot[tslot].s_flags = 0;
	}

	hflg = (attr & HIDDEN) ? TRUE : FALSE;

    /* Delete all files that match with "name" */
	if (wildcard(filename, strlen(filename))) {
	    set_tables(U2U);
	    lcs_set_options(LCS_MODE_NO_MULTIPLE|LCS_MODE_LOWERCASE, 0,country);

	    /* Split search pattern into directory and filename component */
	    if ((slashptr = strrchr(filename, '/')) == NULL) {
		namecomponent = filename;
		strcpy(pathcomponent, CurDir);
	    } else {
		namecomponent = slashptr + 1;
		if (slashptr == filename)
		    strcpy(pathcomponent, "/");
		else {
		    *slashptr = '\0';
		    lcs_translate_string(pathcomponent, MAX_PATH, filename);
		    unmapfilename(CurDir, pathcomponent);
		    *slashptr = '/';
		}
	    }
	    if ((dirdp = opendir(pathcomponent)) == NULL) {
		addr->hdr.res = PATH_NOT_FOUND;
		return;
	    }

	    while ((direntryptr = readdir(dirdp)) != NULL)
	    {
		if ((is_dot(direntryptr->d_name)) ||
		    (is_dot_dot(direntryptr->d_name)) ||
		    ((!hflg) && (direntryptr->d_name[0] == '.')))
			continue;
		strcpy(mappedname,direntryptr->d_name);
		mapfilename(pathcomponent,mappedname);
		if (match(mappedname, namecomponent, MAPPED))
		{
		    if (stat(direntryptr->d_name, &statb) >= 0)
		    {
			/* these are type possible file types:	*/
			/*	S_IFDIR directory		*/
			/*	S_IFCHR character special	*/
			/*	S_IFBLK block special		*/
			/*	S_IFREG regular			*/
			/*	S_IFIFO fifo			*/
			/* only unlink "regular" files		*/

			if ((statb.st_mode & S_IFMT) == S_IFREG) {
			    reg_found = TRUE;
			    /* delete if root or have write permission */
			    if ((!userId || !access(direntryptr->d_name, 2)) &&
						!unlink(direntryptr->d_name)) {
				found = TRUE;
				del_fname(direntryptr->d_name);
			    } else {
			    }
			}
		    } else {
			reg_found = TRUE;	/* so we call err_handler() */
		    }
		}/*endifmatch*/
	    }
	    closedir(dirdp);
	}
	else    /* no wild cards */
	{
	    if (cvt_fname_to_unix(MAPPED, dos_fname, filename)) {
		    addr->hdr.res = ACCESS_DENIED;
		    return;
	    }

	    /* If this is for a pipe open the pipe in /tmp */
	    if ((strncmp(filename, "/%pipe", PIPEPREFIXLEN)) == 0) {
		strcpy(pipefilename, "/tmp");
		dotptr = strchr(filename, '.');
		sprintf(dotptr+1, "%d", getpid());
		strcat(pipefilename, filename);
		strcpy(filename, pipefilename);
	    }

	    if (stat(filename, &statb) >= 0)
	    {
		/* these are type possible file types:	*/
		/*	S_IFDIR directory		*/
		/*	S_IFCHR character special	*/
		/*	S_IFBLK block special		*/
		/*	S_IFREG regular			*/
		/*	S_IFIFO fifo			*/
		/* only unlink "regular" files		*/

		if ((statb.st_mode & S_IFMT) == S_IFREG) {
		    reg_found = TRUE;
		    /* delete if root or have write permission */
		    if ((!userId || !access(filename, 2)) &&
			 !unlink(filename)) {
			found = TRUE;
			del_fname(filename);
		    }
		}
	    } else {
		reg_found = TRUE;	/* so we call err_handler() */
	    }
	}
	/* fill in response header */
	if (found) {
	    addr->hdr.res = SUCCESS;
	    addr->hdr.stat = NEW;
	} else if (!reg_found) {
	    addr->hdr.res = FILE_NOT_FOUND;
	    return;
	} else {
	    /* use the errno value from the last error encountered */
	    err_handler(&addr->hdr.res, request, filename);
	}
	return;
}

