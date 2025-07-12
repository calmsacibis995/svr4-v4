/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_fstatus.c	1.1.1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)p_fstatus.c	4.2	LCC);	/* Modified: 17:22:06 8/15/89 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include	"pci_types.h"
#include	<stat.h>
#include	<errno.h>

#ifdef EXL316
#include	<sys/statfs.h>
#endif

#if	!defined(BERK42FILE) || defined(CCI) || defined(LOCUS)
extern	long
	diskfree(),
	disksize();
#endif	/* !BERK42FILE || CCI || LOCUS */


void
pci_fsize(dos_fname, recordsize, addr, request)
    char	*dos_fname;		/* Name of file */
    int		recordsize;		/* Return size in units of records */
    struct	output	*addr;		/* Pointer to response buffer */
    int		request;		/* DOS request number simulated */
{
    char	filename[MAX_PATH];
    register	int
	length;				/* Length of file in units */
	
    struct	stat
	filstat;


	if (recordsize <= 0) {
	    addr->hdr.res = FAILURE;
	    return;
	}

	/* If unmapping filename returns collision return failure */
	if (cvt_fname_to_unix(MAPPED, dos_fname, filename)) {
	    addr->hdr.res = FAILURE;
	    return;
	}

	if ((stat(filename, &filstat)) < 0)
	{
	    err_handler(&addr->hdr.res, request, filename);
	    return;
	}

	length = (filstat.st_size + recordsize - 1) / recordsize;

    /* Fill-in response header */
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;
	addr->hdr.offset = length;
}



void
pci_setstatus(dos_fname, mode, addr, request)
char
	*dos_fname;		/* Name of file */
int
	mode;			/* Mode to change file to */
struct output
	*addr;			/* Pointer to response buffer */
int
	request;		/* DOS request number simulated */
{
	char	filename[MAX_PATH];

	/* If unmapping filename returns collision return failure */
	if (cvt_fname_to_unix(MAPPED, dos_fname, filename)) {
	    addr->hdr.res = PATH_NOT_FOUND;
	    return;
	}

	if (chmod(filename, mode) < 0) {
		err_handler(&addr->hdr.res, request, filename);
		return;
	}

	/* Fill-in response header */
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;
}


#ifdef	LOCUS
#include <dstat.h>
#endif

void
pci_fstatus(addr, request)
    struct	output	*addr;		/* Pointer to response buffer */
    int		request;		/* DOS request number simulated */
{
    struct	stat
	filstat;
#if	defined(BERK42FILE) && !defined(CCI) && !defined(LOCUS)
    long 
	freeblk,
	totlblk;
#endif	  /* BERK42FILE and not CCI and not LOCUS */

#ifdef EXL316
	struct statfs sfs;

	if (statfs(CurDir, &sfs, sizeof (struct statfs), 0) < 0) {
	    err_handler(&addr->hdr.res, request, CurDir);
	    return;
	}
#else /* !EXL316 */
#ifdef	LOCUS	/* VER_28X */
	struct dstat ds_buf;

	if (dstat(CurDir, &ds_buf, sizeof(ds_buf), 0) < 0) {
	    log("pci_fstatus: dstat failed, errno: %d\n", errno);
	    err_handler(&addr->hdr.res, request, CurDir);
	    return;
	}
#else

	if (stat((*CurDir == '\0') ? "/" : CurDir, &filstat) < 0) {
	    err_handler(&addr->hdr.res, request,
					(*CurDir == '\0') ? "/" : CurDir);
	    return;
	}
#endif
#endif /* EXL316 */

    /* Fill-in response header */
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;
#ifdef EXL316
	addr->hdr.b_cnt = (short)((sfs.f_bfree * sfs.f_bsize)/FREESPACE_UNITS);
	addr->hdr.mode = (short)((sfs.f_blocks * sfs.f_bsize)/FREESPACE_UNITS);
#else /* !EXL316 */
#if	defined(BERK42FILE) && !defined(CCI) && !defined(LOCUS)
	diskstat(filstat.st_dev,&totlblk,&freeblk);

	addr->hdr.b_cnt =(short)(freeblk / FREESPACE_UNITS);
	addr->hdr.mode = (short)(totlblk / FREESPACE_UNITS);

#else	  /* BERK42FILE and not CCI and not LOCUS */

#ifdef	LOCUS
	addr->hdr.b_cnt = diskfree(ds_buf.dst_gfs) / FREESPACE_UNITS;
	addr->hdr.mode = (short)(disksize(ds_buf.dst_gfs) / FREESPACE_UNITS);
#else	/* !LOCUS */

	addr->hdr.b_cnt = diskfree(filstat.st_dev) / FREESPACE_UNITS;

#ifdef	ICT
	if ( addr->hdr.b_cnt > 32000 )
		addr->hdr.b_cnt = 29000;
#endif	/* ICT */

	addr->hdr.mode = (short)(disksize(filstat.st_dev) / FREESPACE_UNITS);

#endif	/* LOCUS */
#endif	  /* BERK42FILE and not CCI and not LOCUS */
#endif	/* EXL316 */
	debug(4, ("pci_fstatus:dev %#x, b_cnt %d, mode %d\n",
		filstat.st_dev, addr->hdr.b_cnt, addr->hdr.mode));
}

void
pci_devinfo(vdescriptor, addr, request)
    int		vdescriptor;		/* PCI virtual file descriptor */
    struct	output	*addr;		/* Pointer to response buffer */
    int		request;		/* DOS request number simulated */
{
    int adescriptor;                    /* Actual UNIX descriptor */
    int status;

/* Get actual UNIX file descriptor */
    if ((adescriptor = swap_in(vdescriptor, 0)) < 0)
    {
	addr->hdr.res = (adescriptor == NO_FDESC) ? FILDES_INVALID : FAILURE;
	return;
    }

    if (isatty(adescriptor))
    {
	/* is Not a file */
	/* filling in DOS device info word */
	/* with CTRL=0 ISDEV=1 EOF=1 RAW=1 ISCLK=0 ISNUL=0 */
	/* "reserved" bits = 0 */
	/* decide on bits ISCOT and ISCIN ("console output","console input" */
	/* Am not sure What this really means, am just seeing if */
	/* stdin, or stdout/stderr */
	switch (adescriptor)
	{
	case 0:
	    status = 0x00e1;    /* console input */
	    break;
	case 1:
	case 2:
	    status = 0x00e2;    /* console output */
	    break;
	default:
	    status = 0x00e0;    /* not stdio */
	    break;
	}
    }
    else /* is a file */
    {
	/* only need to set two bits: ISDEV=0.  EOF=0 when */
	/* "channel has been written" */
	if (write_done(vdescriptor))
	    status = 0x0000;
	else
	    status = 0x0040;
    }
    /* Fill-in response buffer */
    addr->hdr.res = SUCCESS;
    addr->hdr.stat = NEW;
    addr->hdr.mode = status;
}
