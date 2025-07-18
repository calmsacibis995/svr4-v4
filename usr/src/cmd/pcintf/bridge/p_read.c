/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_read.c	1.1.1.1"
#include	"sccs.h"
SCCSID(@(#)p_read.c	4.4	LCC);	/* Modified: 2/20/90 21:46:19 */

/*****************************************************************************

	Copyright (c) 1984-90 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/*
 *	MODIFICATION HISTORY
 *	[12/16/87 JD] Jeremy Daw.	SPR# 2234
 *		Calls to err_handler were passing only one arguement. 
 *	Err_handler expects two and so was pulling a bogus value off the stack.
 *	The bug showed up when accessing an fcb that had already been closed,
 *	this is legal! What happened is if the fcb had been closed and we did
 *	a read on it dos bridge would check the return code and reopen the
 *	file. But since err_handler was called without enuff arguements the
 *	correct error code was not getting back to dos bridge and so it thought
 *	the read had failed for reasons other than an unopened fdes, so it never
 *	tried to reopen the fcb. I thought this explanation might be usefull
 *	for archaeological reasons.
 *	TBD, p_fstatus calls err_handler without the second arguement. Can't
 *	be fixed this release, but it needs to be.
 */

#include	"pci_types.h"
#include	<stat.h>
#include	<errno.h>

#ifdef RLOCK  /* record locking */
#include	<rlock.h>
extern struct vFile	
		*validFid();		/* virtual file validation routine */
extern int	ioStart();		/* Check for locks & excludes */
					/* others from lock table. */
extern void 	ioDone();		/* Unlocks the lock table */
#endif  /* RLOCK */


extern long
	lseek(),
	time();

extern  int swap_in();                  /* Swap-in file context */


extern  int errno;                      /* Global errno from system calls */
extern  int request;                    /* Current request type */
extern  int swap_how;                   /* How to swap output packets */
extern  int brg_seqnum;                 /* Current sequence number */
extern  int outputframelength;          /* Length of last output buffer */

extern  char err_class, err_action, err_locus;
extern  char *optr;                     /* Pointer to last frame sent */

extern  struct output out1;             /* Output frame buffers */

extern  struct ni2 ndata;               /* Ethernet header */

int     read_state;                     /* Current state */

unsigned int bytesread;                 /* Number of bytes returned */
unsigned int brequested;                /* Number of bytes requested */

struct  output out2, out3;              /* Read ahead output buffers */
struct  output *saved_address;          /* Save address of next output buffer */


int
pciran_read(vdescriptor, offset, whence, inode, addr)
    int		vdescriptor;		/* PCI virtual file descriptor */
    long	offset;			/* Offset to read from */
    int		whence;			/* Base to lseek from */
    u_short	inode;			/* Actual UNIX inode of file */
    register	struct	output	*addr;	/* Pointer to response buffer */
{
    register	int
	adescriptor;			/* Actual UNIX fiel descriptor */


    /* Swap-in UNIX descriptor */
	if ((adescriptor = swap_in(vdescriptor, inode)) < 0) {
	    addr->hdr.res = FILDES_INVALID;
	    return FALSE;
	}

	if ((lseek(adescriptor, offset, whence)) < 0) {
	    err_handler(&addr->hdr.res, request, NULL);	/* [12/16/87 JD] */
	    return FALSE;
	}
	return TRUE;
}


/*
 *	What happens is that server() makes a pciseq_read request and if
 *	there is an error condition we return without setting the packet
 *	size. In the case of RS232 it could be large. This is because large
 *	chunks of data can be requested and serviced in smaller packets.
 *	The problem was running a program on the virtual drive, it would load
 *	up some large packets, then on the first record locking violation
 *	this large packet size would be propogated up and there would be
 *	a crash in chksum() because of the packet size.
 *	I *beleive* that in the ethernet case the packet size gets limited
 *	somewhere else and thus the packet size doesn't get humungus.
 *	TBD 
 *	The TBD above applies to the error returns above that also don't
 *	reset the packet size. Should the packet size be set upon entry
 *	to this routine? I don't know, I don't have time for further
 *	investigation right now.
 */

#ifdef MEM_COPY
struct output *
pciseq_read(vdescriptor, bytes, intoAddr)
int vdescriptor;
int bytes;
char *intoAddr;
#else
struct	output	*
pciseq_read(vdescriptor, bytes)
    int		vdescriptor;		/* PCI virtual file descriptor */
    int		bytes;			/* Number of bytes to read */
#endif	/* MEM_COPY */
{
    register	int
	status,				/* Return value from system calls */
	adescriptor;			/* Actual UNIX descriptor */

    long
	offset;				/* Current pointer position in file */

    int
	bytestoread;			/* Number of bytes to read now */

    register	struct	output
	*addr;				/* Pointer to response buffer */

#ifdef RLOCK  /* record locking */
long 		iolow;			/* For ioStart lock check */

struct vFile	*vfSlot;		/* Pointer to virtual file */
#endif  /* RLOCK */
#ifndef	MEM_COPY
	int	might_do_mtf;		/* Might to "more-to-follow", which
					** means splitting a big read into
					** several smaller ones, because of
					** limited packet size.
					*/ 
	char	*intoAddr;		/* Read into this buffer */
#endif	/* not MEM_COPY */
#ifdef	JANUS
	int	preset_read;		/* Bytes to read from preset file */
	static	char	*inbuf = NULL;
	static	int	inbufsize = 0;
	extern	char	*malloc();
#endif	/* JANUS */


    /* Swap-in actual UNIX file descriptor */
	addr = &out2;
	if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
	    addr->hdr.res =
		(adescriptor == NO_FDESC) ? FILDES_INVALID : ACCESS_DENIED;
	    addr->hdr.t_cnt = 0;		/* no text only a response */
	    return addr;
	}

#ifdef MEM_COPY
	bytestoread = bytes;
#else	/* not MEM_COPY */
	brequested = bytes;
	if (brequested > MAX_OUTPUT) {
		might_do_mtf = 1;
		bytestoread = MAX_OUTPUT;
	} else {
		might_do_mtf = 0;
		bytestoread = brequested;
	}
	intoAddr = addr->text;
#endif	/* not MEM_COPY */

#ifdef RLOCK  /* record locking */
	/* Get the handle */
	vfSlot = validFid(vdescriptor, (word) getpid());
	if (vfSlot == NULL) {
		addr->hdr.res = FILDES_INVALID;
		addr->hdr.t_cnt = 0;		/* no text only a response */
		return addr;
	}

	/* Verify the file is readable */
	if ((vfSlot->flags & FF_READ) == 0) {
		addr->hdr.res = ACCESS_DENIED;
		addr->hdr.t_cnt = 0;		/* no text only a response */
		return addr;
	}

	iolow = lseek(adescriptor, 0L, 1);  /* current file pointer loc */

	/* Check for lock conflicts and lock the lock table */
	if ((vfSlot->flags & FF_NOLOCK) == 0)  /* If file is lockable type... */
	if (ioStart((int) vfSlot->shareIndex, (long) vfSlot->dosPid, iolow,
 	   (unsigned long)bytestoread)
	    == -1) {
		addr->hdr.res = LOCK_VIOLATION;
		addr->hdr.t_cnt = 0;		/* no text only a response */
		return addr;
	}
#endif  /* RLOCK */

    /* Read the requested amount or 1K, whichever is less */
#ifdef	JANUS
	/* When reading JANUS preset file (stdin), read half requested amount,
	** because NLS translation could expand each byte into two.
	*/
	preset_read = bytestoread >> 1;
	if (!preset_read)
		preset_read = 1;
	if ((vfSlot->flags & VF_PRESET) && (inbufsize < preset_read)) {
		if (inbuf) free(inbuf);
		inbuf = malloc(preset_read);
		inbufsize = preset_read;
	}
#endif	/* JANUS */
	do {
#ifdef	JANUS
	    if ((vfSlot->flags & VF_PRESET) && inbuf) {
		/* Janus preset files are stdin, stdout and stderr.
		** (reads here will normally only be from stdin)
		** Do possible unix-to-dos character translation.
		** Because translation can double the number of
		** bytes, only read half of the requested amount.
		*/
		bytesread = read(adescriptor, inbuf, preset_read);
		if (bytesread != (unsigned)-1) {
			bytesread = janus_u2d(intoAddr, bytestoread,
				inbuf, bytesread, &(vfSlot->vf_jstate));
		}
#ifndef	MEM_COPY
		/* Because the translation can change the number of bytes
		** up or down, the later calculations with bytesread
		** and brequested can get really screwed up.  Here we
		** assume that nothing bad will happen if we satisfy the
		** read here, and not do the MTF (more-to-follow) stuff.
		** This is safe for two reasons: 1) JANUS and MEM_COPY
		** are always defined together, so we won't be doing
		** any MTF stuff anyway.  2) We are translating stdin,
		** so more reads on the DOS side will be done again
		** anyway.
		*/
		might_do_mtf = 0;
#endif	/* not MEM_COPY */
	    }
	    else
#endif	/* JANUS */
	    {
		bytesread = read(adescriptor, intoAddr, bytestoread);
	    }
	} while (bytesread == (unsigned)-1 && errno == EINTR);
	if (bytesread == (unsigned)-1) {
	    err_handler(&addr->hdr.res, request, NULL);
#ifdef RLOCK  /* record locking */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	    addr->hdr.t_cnt = 0;		/* no text only a response */
	    return addr;
	}

	offset = lseek(adescriptor, 0L, 1);
#ifndef	MEM_COPY
	/* See if fully satisfied the read, or will need to do MTF.
	** The read is not satisfied only in the case when
	** "might_do_mtf" is set, and got the full amount requested
	** by the read command.
	*/
	if (!might_do_mtf || bytesread < bytestoread)
#endif	/* not MEM_COPY */
	{
#ifdef RLOCK  /* record locking */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	    addr->hdr.res = SUCCESS;
	    addr->hdr.stat = NEW;
	    addr->hdr.fdsc = vdescriptor;
	    addr->hdr.b_cnt = bytesread;
#ifdef MEM_COPY
	    /* There is no text in a MEM_COPY.  b_cnt has size */
	    addr->hdr.t_cnt = 0;
#else	/* not MEM_COPY */
	    addr->hdr.t_cnt = bytesread;
#endif	/* not MEM_COPY */
	    addr->hdr.offset = offset;

		/* NOTE: a MEM_COPY read returns here. */
	    return addr;
	}
#ifndef	MEM_COPY
    /* Send first frame of multiple-frame transaction */
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW_MTF;
	addr->pre.select = BRIDGE;
	addr->hdr.req = request;
	addr->hdr.seq = brg_seqnum;
	addr->hdr.fdsc = vdescriptor;
	addr->hdr.b_cnt = NULL;
	addr->hdr.t_cnt = bytesread;
	addr->hdr.offset = offset;
	optr   = (char *)addr;

	outputframelength = xmtPacket((struct output *)optr, &ndata, swap_how);

	saved_address = addr = &out3;

    /* Read until the end of request or EOF */
	bytestoread = (brequested - bytesread >= MAX_OUTPUT)
					? MAX_OUTPUT : brequested - bytesread;
	do
	    status = read(adescriptor, addr->text, bytestoread);
	while (status == -1 && errno == EINTR);
	if (status < 0) {
	    err_handler(&addr->hdr.res, request, NULL);
#ifdef RLOCK  /* record locking */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	    addr->hdr.b_cnt = bytesread;
	    read_state = READ_LAST;
	    return NULL;
	}
	bytesread += status;
	offset = lseek(adescriptor, 0L, 1);
	if (status < bytestoread || bytesread == brequested) {
	    addr->hdr.stat = EXT;
	    addr->hdr.b_cnt = bytesread;
	    read_state = READ_LAST;
	} else {
	    addr->hdr.stat = EXT_MTF;
	    addr->hdr.b_cnt = NULL;
	    read_state = READ_MTF;
	}
	addr->hdr.res = SUCCESS;
	addr->hdr.fdsc = vdescriptor;
	addr->hdr.t_cnt = status;
	addr->hdr.offset = offset;

#ifdef RLOCK  /* record locking */
	if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	return NULL;
#endif	/* not MEM_COPY */
}



struct	output *
pci_ack_read(vdescriptor) 
    int	vdescriptor;
{
    register	int
	status,				/* Return value from system call */
	adescriptor;			/* Actual UNIX descriptor */
    
    long
	offset;				/* Current offset with file */

    int
	bytestoread;			/* Number of bytes to read now */

    register	struct	output
	*addr;				/* Pointer to response buffer */

#ifdef RLOCK  /* record locking */
long 		iolow;			/* For ioStart lock check */

struct vFile	*vfSlot;		/* Pointer to virtual file */
#endif  /* RLOCK */


    /* Is there a prior context? */
	if (read_state == NULL) {
	    addr = &out1;
	    addr->hdr.res = INVALID_FUNCTION;
	    addr->hdr.stat = NEW;
	    return addr;
	}

	if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
	    addr = &out1;
	    addr->hdr.res =
		(adescriptor == NO_FDESC) ? FILDES_INVALID : ACCESS_DENIED;
	    return addr;
	}

    /* Send frame already built */
	addr = saved_address;
	addr->hdr.seq = brg_seqnum;
	addr->pre.select = BRIDGE;
	addr->hdr.req = request;
	optr = (char *)addr;

	outputframelength = xmtPacket((struct output *)optr, &ndata, swap_how);

    /* Build next response in parallel frame */
	if (read_state == READ_LAST) {
	    read_state = NULL;
	    return NULL;
	}

    /* Select next output buffer */
	addr = (addr == &out2) ? &out3 : &out2;
	saved_address = addr;

	bytestoread = (brequested - bytesread >= MAX_DATA)
			? MAX_DATA : brequested - bytesread;

#ifdef RLOCK  /* record locking */
	/* Get the handle */
	vfSlot = validFid(vdescriptor, (word) getpid());
	if (vfSlot == NULL) {
		addr->hdr.res = FILDES_INVALID;
		read_state = READ_LAST;
		return NULL;
	}

	/* Verify the file is readable */
	if ((vfSlot->flags & FF_READ) == 0) {
		addr->hdr.res = ACCESS_DENIED;
		read_state = READ_LAST;
		return NULL;
	}

	iolow = lseek(adescriptor, 0L, 1);  /* current file pointer loc */

	/* Check for lock conflicts and lock the lock table */
	if ((vfSlot->flags & FF_NOLOCK) == 0)  /* If file is lockable type... */
	if (ioStart((int) vfSlot->shareIndex, (long) vfSlot->dosPid, iolow,
	    (unsigned long) bytestoread)
			== -1) {
		addr->hdr.res = LOCK_VIOLATION;
		read_state = READ_LAST;
		return NULL;
	}
#endif  /* RLOCK */


	do
	    status = read(adescriptor, addr->text, bytestoread);
	while (status == -1 && errno == EINTR);
	if (status < 0) {
	    err_handler(&addr->hdr.res, request, NULL);
#ifdef RLOCK  /* record locking */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	    addr->hdr.b_cnt = bytesread;
	    read_state = READ_LAST;
	    return NULL;
	}
	bytesread += status;
	offset = lseek(adescriptor, 0L, 1);
	if (status < bytestoread || bytesread == brequested) {
	    addr->hdr.stat = EXT;
	    addr->hdr.b_cnt = bytesread;
	    read_state = READ_LAST;
	} else {
	    addr->hdr.stat = EXT_MTF;
	    addr->hdr.b_cnt = NULL;
	    read_state = READ_MTF;
	}
	addr->hdr.res = SUCCESS;
	addr->hdr.fdsc = vdescriptor;
	addr->hdr.t_cnt = status;
	addr->hdr.offset = offset;

#ifdef RLOCK  /* record locking */
	if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	return NULL;
}
