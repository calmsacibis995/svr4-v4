/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_create.c	1.1.1.2"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)p_create.c	4.4	LCC);	/* Modified: 17:59:58 2/25/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include	"pci_types.h"
#include	<stat.h>
#include	<time.h>
#include	<errno.h>
#include	<string.h>


/*				External Routines			*/

extern	long get_dos_time();	/* returns the virtual DOS time stamp on file */
extern  int bdate();                /* Converts date to MS-DOS format */
extern  int btime();                /* Converts time into MS-DOS format */
extern  int swap_in();              /* Causes a virtual descriptor to be paged in */
extern  int attribute();            /* Set attribute bits in output frame */
extern  int create_file();          /* PCI access routine for creating files */

extern	char *memory();         /* Allocate a dynamic buffer */

#if	defined(XENIX) || !defined(SYS5)
extern char *mktemp();          /* Create a unique temporary filename */
#else
extern char *tempnam();		/* Create a unique temporary filename */
#endif

extern  struct tm *localtime(); /* Load file date into tm structure */



/*			Imported Structures				*/


extern  int errno;              /* Contains error code from system calls */
extern  int print_desc[NPRINT];    /* File descriptor of print/spool file */

extern  char *print_name[NPRINT];  /* Filename of file to be printed */

struct temp_slot temp_slot[NSLOT];

void
pci_create(dos_fname, mode, dosAttr, pid, addr, request)
char
	*dos_fname;			/* File name(with possible metachars) */
int
	mode,				/* Mode to open file with */
	dosAttr,			/* Dos file attribute */
	pid;				/* Process id of PC process */

struct output
	*addr;				/* Pointer to response buffer */
int
	request;			/* DOS request number simulated */
{
    register int
	vdescriptor, 			/* PCI virtual file descriptor */
	adescriptor;			/* Actual UNIX file descriptor */

    int
	stringlen, 			/* Length of filename */
	ddev,	 			/* dev of directory */
	tslot,                          /* temp file slot number */
	printx = -1,                    /* printer table index */
	unlink_on_sig = FALSE;

    long
	dos_time_stamp;			/* Virtual DOS time stamp on file */

    ino_t 
	dino;				/*Inode of directory */ 

    char
	*dotptr, 			/* Pointer to "." in filename */
	filename[MAX_PATH],		/* Unix style filename */
	pipefilename[MAX_PATH],         /* Filename of pipe file in /tmp */
	pathcomponent[MAX_PATH];	/* Path part of full filename */

    struct tm 
	*timeptr;			/* Pointer for locattime() */

    struct	stat
	filstat;			/* File status structure for stat() */


	dino = 0;
      /* Transform filename if need be */
	if (cvt_fname_to_unix(MAPPED, dos_fname, filename)) {
		addr->hdr.res = ACCESS_DENIED;
		return;
	}
	if (strpbrk(filename, "?*")) {		/* disallow wildcards */
	    addr->hdr.res = FILE_NOT_FOUND;	/* sic */
	    return;
	}
	if ((dosAttr & SUB_DIRECTORY) ||
	    (mode == TEMPFILE && (dosAttr & VOLUME_LABEL))) {
	    addr->hdr.res = ACCESS_DENIED;
	    return;
	}
	if (mode == PRINT) {
	  /* Create a tmp file for print spooling */
	    if ((printx = find_printx(-1)) == -1) {
		log("Print file table full!\n\n");
		addr->hdr.res = TOO_MANY_FILES;
		return;
	    }
#if	defined(XENIX) || !defined(SYS5)
	    print_name[printx] = memory(MAX_FNAME + 1);
	    strcpy(print_name[printx], "/tmp/cr_XXXXXX");
	    print_name[printx] = mktemp(print_name[printx]);
#else
	    print_name[printx] = tempnam("", "pci");
#endif
	    strcpy(filename, print_name[printx]);
	    unlink_on_sig = TRUE;
	    log("Opened print file.  printx = %d, name = %s\n\n",printx,
		print_name[printx]);
	} else {

	    if (mode != TEMPFILE &&
		strncmp(filename, "/%pipe", PIPEPREFIXLEN) != 0) {
		if ((tslot = redirdir(filename, 0)) >= 0) {
		    strcpy(filename, "/tmp/");
		    strcat(filename, temp_slot[tslot].fname);
		} /*endif "redirected" */
		strcpy(pathcomponent, "/");
	    }

#ifdef  JANUS
	    fromfunny(filename); /* this translates funny names, as well as */
			      /* autoexec.bat, con, ibmbio.com, ibmdos.com  */
#endif  /* JANUS */

	    addr->hdr.t_cnt = 0;

	    if (mode == TEMPFILE) {
		if (strlen(dos_fname) == 0)   /* null string becomes root */
		    strcpy(filename, "/");

		dotptr = strlen(filename) + filename; /* point at ending null */

		if ((*(dotptr-1) != '/') && (*(dotptr-1) != '\\'))
		{
		    *(dotptr++) = '/';
		    *dotptr = '\0';         /* add in terminator */
		}

		/*** Check that directory is writeable ***/
		if (access(filename, 2) &&
		    ((errno == EACCES) || (errno == EROFS)))
		{
		    /* Not writeable */
		    if (stat(filename, &filstat)) {
			err_handler(&addr->hdr.res, request, filename);
			return;     /* Directory has serious problems */
		    }
		    dino = filstat.st_ino;  /* Remember directory inode */
		    ddev = filstat.st_dev;
		    strcpy(filename, "/tmp/00XXXXXX");
		    dotptr = &filename[5];
		} else {
		    /* add in the temp string */
		    strcat(filename, "00XXXXXX");
		}
		dotptr[1] --;
		do {
		    dotptr[1]++;
		    mktemp(filename);
		} while (stat(filename, &filstat) >= 0);

	    /* just return name portion to DOS */

		unlink_on_sig = TRUE;
		strncpy(pathcomponent, filename, (dotptr - filename));
		pathcomponent[dotptr - filename] = '\0';
	    } /*endif TEMP */
	    /* If this is for a pipe open the pipe in /tmp */
	    else if ((strncmp(filename, "/%pipe", PIPEPREFIXLEN)) == 0)
	    {
		    strcpy(pipefilename, "/tmp");
		    dotptr = strchr(filename, '.');
		    sprintf(dotptr+1, "%d", getpid());
		    strcat(pipefilename, filename);
		    strcpy(filename, pipefilename);
		    unlink_on_sig = TRUE;
	    } /*endif PIPE */
	}

      /* At this point am done converting filename. */
      /* Now, create the file, and get a virtual descriptor. */

	if (mode == CREATENEW && stat(filename, &filstat) >= 0) {
		addr->hdr.res = FILE_EXISTS;
		return;
	}

	vdescriptor = create_file(filename, 2,
				pid, unlink_on_sig, dosAttr, mode);

	if (vdescriptor < 0) {
	    addr->hdr.res = -vdescriptor;
	    return;
	}

	/* Get the actual descriptor */
	if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
	    addr->hdr.res =
		(adescriptor == NO_FDESC) ? FILDES_INVALID : ACCESS_DENIED;
	    return;
	}

	if ((fstat(adescriptor, &filstat)) < 0)
	{
	    err_handler(&addr->hdr.res, request, NULL);
	    return;
	}

    /* If for print/spooling, store file descriptor */
	if (printx != -1)
	    print_desc[printx] = vdescriptor;

	if (mode == TEMPFILE) {
	    cvt_fname_to_dos(MAPPED, pathcomponent, dotptr, addr->text);
	    addr->hdr.t_cnt = strlen(addr->text) + 1;
	    if (dino) {	/* if put the temp file in /tmp then put filename */
			/* in temp slot array, so it can be found later */
	        for (tslot = 0; tslot < NSLOT; tslot++) {
		    if (temp_slot[tslot].s_flags == 0) {
		        strcpy(temp_slot[tslot].fname, dotptr);
		        temp_slot[tslot].s_ino = dino;
		        temp_slot[tslot].s_dev = ddev;
		        temp_slot[tslot].s_flags = 1;
		        break;
		    }
	        }/*endfor*/
	    }
	}

    /* Fill-in response header */
	addr->hdr.res    = SUCCESS;
	addr->hdr.stat   = NEW;
	addr->hdr.fdsc   = vdescriptor;
	addr->hdr.offset = lseek(adescriptor, 0L, 1);
	addr->hdr.inode = (u_short)filstat.st_ino;
	addr->hdr.f_size = filstat.st_size;
	addr->hdr.attr   = attribute(&filstat);

	dos_time_stamp = get_dos_time (vdescriptor);
	timeptr = localtime(&(dos_time_stamp));
	addr->hdr.date = bdate(timeptr);
	addr->hdr.time = btime(timeptr);

	addr->hdr.mode = ((filstat.st_mode & 070000) == 0) ? 1 : 0;
}


/*--------------------------------------------------------*/
/* Test for match of filename in relocated filename table
/* Return zero if no match.
/* Return index into temp filename table plus 1.
/*--------------------------------------------------------*/
int redirdir(filename, first_tslot)
	char *filename;
	int first_tslot;
{
	char dirname[MAX_PATH];
	char fname[14];
	struct stat filstat;
	register char *sp, *cp;
	register int ddev, tslot;
	register ino_t dino;

	for(cp=sp=filename; *cp; cp++)
		if ( *cp == '/') sp = cp;
	if (*sp == '/')
	{	strcpy(fname, sp+1);
		*sp = 0;
		strcpy(dirname, filename);
		*sp = '/';
		if (dirname[0] == 0) strcpy(dirname, "/");
	} else
	{	strcpy(fname, sp);
		strcpy(dirname, ".");
	}
	if (fname[0] != '0')
	{
	    return -1;
	}
	if (stat(dirname, &filstat))
	{
		return -1;
	}
	dino = filstat.st_ino;
	ddev = filstat.st_dev;

	for (tslot = first_tslot; tslot < NSLOT; tslot++)
	{
	    if (temp_slot[tslot].s_flags
		&& temp_slot[tslot].s_ino == dino
		&& temp_slot[tslot].s_dev == ddev
		&& match(temp_slot[tslot].fname, fname, MAPPED))
		    return tslot;
	}
	return -1;
}
