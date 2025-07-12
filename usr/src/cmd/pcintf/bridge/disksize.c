/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/disksize.c	1.1.1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)disksize.c	4.2	LCC);	/* Modified: 10/2/90 19:34:04 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#if	!defined(BERK42FILE) || defined(CCI) || defined(LOCUS)
#ifndef	PARAM_GETS_TYPES
#include	<sys/types.h>
#ifdef  XENIX   
#include        <sys/iobuf.h>
#endif  /* XENIX */
#endif	/* ~PARAM_GETS_TYPES */

#include	<param.h>

#if defined(CCI)
#include	<sys/fs.h>
#include 	<mtab.h>
#elif defined(SYS5_4) || defined(MIPS)
#include	<sys/fs/s5param.h>
#include	<sys/filsys.h>
#include	<mnttab.h>
#elif defined(XENIX)
#include	<stdio.h>
#include	<filsys.h>
#include	<mnttab.h>
#else
#include	<filsys.h>
#include	<mnttab.h>
#endif	/* CCI, etc ... */

#include	<stat.h>
#include	<fcntl.h>
#include	"const.h"
#include	<string.h>
#include	<ustat.h>
#include	<log.h>

/*			External Functions & Variables			*/

#ifdef	XENIX
#define	SBLOCK	2
#endif

#ifndef	SBLOCK
#ifndef	LOCUS
#define	SBLOCK	1
#else	/* LOCUS */
#define	SBLOCK	2
#endif	/* LOCUS */
#endif	/* !SBLOCK */

#define	SUPERBLOCK	(SBLOCK*512)

#if	defined(IX370) || defined(CCI)
#define	USTAT_BLKS	4096
#else	/* !IX370 */
#ifdef  LOCUS
#define	USTAT_BLKS	1024
#else
#define USTAT_BLKS      512
#endif  /* !LOCUS */
#endif	/* !IX370 */
	
#ifdef XENIX
#define	USTAT_BLKS	1024
#define SMAG S_S3MAGIC
#define STYP S_B512
#else
#ifdef	AIX_RT
#define	SMAG FSmagic
#define	STYP Fs1b
#else	/* AIX_RT */
#define SMAG FsMAGIC
#define STYP Fs1b
#endif	/* AIX_RT */
#endif	/* XENIX */

#define	FAKEUSTAT (0xd33 * FREESPACE_UNITS)


int
	mnttbl_entries;			/* Number of entries in table */

struct	dev_entry {
	dev_t	rdev;			/* Major/mior device number */
	long	disksize;		/* Size of file system blocks */
};

struct	dev_entry
	*disktab = NULL;		/* Pointer to block-size table */

extern char
	*memory();

extern long
	lseek();

#endif	/* !BERK42FILE || CCI || LOCUS */



/*
 *    bld_disktab() -	builds a table which contains the blocksizes
 *				for each mounted filesystem.
 */

void
bld_disktab()
{
#if	(!defined(LOCUS) && !defined(BERKELEY42)) || defined(CCI)
    register int
	i,
	blocksize,
	mnttabdesc,			/* File descriptor of /etc/mnttab */
	devicedesc;			/* File descriptor of device */
    long freebytes = 0;			/* number of free blocks in fs */

    char 
	devicename[MAX_PATH];

#ifdef	CCI
    static struct fs
#else
    static struct filsys
#endif	/* CCI */
	superblock;			/* Contains super-block from device */

#ifdef	CCI
    struct mtab
#else
    struct mnttab
#endif	/* CCI */
	mnttab_entry;			/* Entry of /etc/mnttab */

    struct stat
	filstat;			/* Contains data from stat() */

    struct	ustat
	ustatbuf;			/* Ustat buffer */ 

    blocksize = USTAT_BLKS;

/* For currently mounted file systems... */
    if (stat(MNT_TABLE, &filstat) < 0)
	return;

    if ((mnttabdesc = open(MNT_TABLE, O_RDONLY)) < 0)
	return;

/* Calculate the number of entries in /etc/mnttab */
#ifdef	CCI
    mnttbl_entries = filstat.st_size/sizeof(struct mtab);
#else
    mnttbl_entries = filstat.st_size/sizeof(struct mnttab);
#endif	/* CCI */

/* Get enough memory for blocksize table */
    if ((disktab = (struct dev_entry *)
#ifdef	CCI
      memory(mnttbl_entries * (int) sizeof(struct mtab))) == NULL)
#else
      memory(mnttbl_entries * (int) sizeof(struct mnttab))) == NULL)
#endif
    {
	return;
    }

/* For each device with a mounted file system find block size */
    for (i = 0; i < mnttbl_entries; i++)
    {
	if (read(mnttabdesc, &mnttab_entry, sizeof(mnttab_entry))
	  < sizeof(mnttab_entry))
	{
	    close(mnttabdesc);
	    return;
	}

    /* Check device, open it and read super-block */
#ifdef	CCI
	(void) strcpy(devicename, mnttab_entry.m_dname);
#else
	(void) strcpy(devicename, "/dev/");
	(void) strcat(devicename, mnttab_entry.mt_dev);
#endif	/* CCI */
	if (stat(devicename, &filstat) < 0)
	    continue;

	if ((devicedesc = open(devicename, O_RDONLY)) < 0)
	    continue;

	if (lseek(devicedesc, (long) SUPERBLOCK, 0) != SUPERBLOCK)
	{
	    close(devicedesc);
	    continue;
	}

#ifdef	CCI
	if (read(devicedesc, &superblock, sizeof(struct fs)) < 0)
#else
	if (read(devicedesc, &superblock, sizeof(struct filsys)) < 0)
#endif	/* CCI */
	{
	    close(devicedesc);
	    continue;
	}

    /* Store total nuber of bytes in filesystem */
	disktab[i].rdev = filstat.st_rdev;

#if (!defined(LOCUS) && !defined(IX370) && !defined(CCI))
      /* standard sys5 and Xenix can have file sysytems with different */
      /* sized blocks. So look in superblock to determine what size */
	if (superblock.s_magic != SMAG)
	    blocksize = 512;
	else
	    blocksize = (superblock.s_type == STYP) ? 512 : 1024;
#endif  /* (!defined(LOCUS) && !defined(IX370)) */

#ifdef	CCI
	blocksize = superblock.fs_fsize;
	disktab[i].disksize = superblock.fs_dsize * blocksize;
#else
	disktab[i].disksize = superblock.s_fsize * blocksize;
#endif

	close(devicedesc);
    }

    close(mnttabdesc);
    return;
}


/*
 *      disksize() -    Scan the disktab searching for the specified device.
 *                      Returns the number of blocks in the file system.
 */

long
disksize(device)
int	device;
{
    register int i;

    if (disktab != NULL)
    {
	for (i = 0; i < mnttbl_entries; i++)
	{
	    if (device == disktab[i].rdev)
		return(disktab[i].disksize);
	}
    }
    return(FAKEUSTAT);
}
/*
 *      diskfree() -    Returns the numberof free blocks in the file system.
 */

long
diskfree(device)
int	device;
{
    extern int errno;
    struct ustat ustatbuf;              /* Ustat buffer */


    /* NOTE: ustat() always returns the number of free blocks in units of */
    /* 512 for generic System V. Locus uses units of 1024. IX370 uses 4096. */
    /* Xenix System V uses 1024 blocks !!!! w/ustat */
    if (ustat(device, &ustatbuf) < 0)
	return(FAKEUSTAT);
	
    return(ustatbuf.f_tfree*USTAT_BLKS);
#endif	/* (!BERKELEY42 && !LOCUS) || defined(CCI) */
}

/*
 * These routines are for LOCUS only !!!
 */

#ifdef	LOCUS

#include <dustat.h>
/*
 *      disksize() -    Determines the number of blocks in the file system
 *			specified by the GFS number passed.  Returns the
 *			number of BYTES in the file system.
 */

long
disksize(gfs)
gfs_t	gfs;
{
    extern int errno;
    struct dustat du_buf;              /* dustat buffer */

    if (dustat(gfs, 0, &du_buf, sizeof(du_buf)) < 0) {
	log("disksize: dustat failed, errno: %d\n", errno);
	return(FAKEUSTAT);
    }

    return(du_buf.du_fsize * du_buf.du_bsize);
}

/*
 *      diskfree() -    Determines the number of free blocks in the file
 *			system specified by the GFS number passed.  Returns
 *			the number of free BYTES !
 */

long
diskfree(gfs)
gfs_t	gfs;
{
    extern int errno;
    struct dustat du_buf;              /* dustat buffer */

    if (dustat(gfs, 0, &du_buf, sizeof(du_buf)) < 0) {
	log("diskfree: dustat failed, errno: %d\n", errno);
	return(FAKEUSTAT);
    }
	
    return(du_buf.du_tfree * du_buf.du_bsize);
}
#endif
