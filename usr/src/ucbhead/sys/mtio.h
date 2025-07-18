/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ucbhead:sys/mtio.h	1.1.3.1"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * Structures and definitions for mag tape io control commands
 */

/* structure for MTIOCTOP - mag tape op command */
struct	mtop	{
	short	mt_op;		/* operations defined below */
	daddr_t	mt_count;	/* how many of them */
};

/* operations */
#define MTWEOF		0	/* write an end-of-file record */
#define MTFSF		1	/* forward space file */
#define MTBSF		2	/* backward space file */
#define MTFSR		3	/* forward space record */
#define MTBSR		4	/* backward space record */
#define MTREW		5	/* rewind */
#define MTOFFL		6	/* rewind and put the drive offline */
#define MTNOP		7	/* no operation, sets status only */
#define MTRETEN		8	/* retension the tape */
#define MTERASE		9	/* erase the entire tape */
#define MTEOM		10	/* position to end of media (SCSI only) */
#define MTBSFM		11	/* backspace filemark */


/* structure for MTIOCGET - mag tape get status command */
struct	mtget	{
	short	mt_type;	/* type of magtape device */
/* the following two registers are grossly device dependent */
	short	mt_dsreg;	/* ``drive status'' register */
	short	mt_erreg;	/* ``error'' register */
/* optional error info. */
	daddr_t	mt_resid;	/* residual count */
	daddr_t	mt_fileno;	/* file number of current position */
	daddr_t	mt_blkno;	/* block number of current position */
};

/*
 * Constants for mt_type byte
 */
#define	MT_ISTS		0x01		/* vax: unibus ts-11 */
#define	MT_ISHT		0x02		/* vax: massbus tu77, etc */
#define	MT_ISTM		0x03		/* vax: unibus tm-11 */
#define	MT_ISMT		0x04		/* vax: massbus tu78 */
#define	MT_ISUT		0x05		/* vax: unibus gcr */
#define	MT_ISCPC	0x06		/* sun: multibus cpc */
#define	MT_ISAR		0x07		/* sun: multibus archive */
#define	MT_ISSC		0x08		/* sun: SCSI archive */
#define	MT_ISXY		0x09		/* sun: Xylogics 472 */
#define	MT_ISSYSGEN11	0x10		/* sun: SCSI Sysgen, QIC-11 only */
#define	MT_ISSYSGEN	0x11		/* sun: SCSI Sysgen QIC-24/11 */
#define	MT_ISDEFAULT	0x12		/* sun: SCSI default CCS */
#define	MT_ISCCS3	0x13		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISMT02	0x14		/* sun: SCSI Emulex MT02 */
#define	MT_ISVIPER1	0x15		/* sun: SCSI Archive QIC-150 Viper */
#define	MT_ISWANGTEK1	0x16		/* sun: SCSI Wangtek QIC-150 */
#define	MT_ISCCS7	0x17		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS8	0x18		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS9	0x19		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS11	0x1a		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS12	0x1b		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS13	0x1c		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS14	0x1d		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS15	0x1e		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS16	0x1f		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCDC	0x20		/* sun: SCSI CDC 1/2" cartridge */
#define	MT_ISFUJI	0x21		/* sun: SCSI Fujitsu 1/2" cartridge */
#define	MT_ISKENNEDY	0x22		/* sun: SCSI Kennedy 1/2" reel */
#define	MT_ISHP		0x23		/* sun: SCSI HP 1/2" reel */
#define	MT_ISCCS21	0x24		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS22	0x25		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS23	0x26		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS24	0x27		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISEXABYTE	0x28		/* sun: SCSI Exabyte 8mm cartridge */
#define	MT_ISCCS26	0x29		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS27	0x2a		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS28	0x2b		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS29	0x2c		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS30	0x2d		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS31	0x2e		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS32	0x2f		/* sun: SCSI generic (unknown) CCS */

/*
 * Device table structure and data for looking tape name from
 * tape id number.  Used by mt.c.
 */
struct mt_tape_info {
	short	t_type;		/* type of magtape device */
	char	*t_name;	/* printing name */
	char	*t_dsbits;	/* "drive status" register */
	char	*t_erbits;	/* "error" register */
};
#define MT_TAPE_INFO  {\
{ MT_ISCPC,		"TapeMaster",		TMS_BITS, 0 }, \
{ MT_ISXY,		"Xylogics 472",		XTS_BITS, 0 }, \
{ MT_ISAR,		"Archive",		ARCH_CTRL_BITS,	ARCH_BITS }, \
{ MT_ISSYSGEN11,	"Sysgen QIC-11",	0, 0 }, \
{ MT_ISSYSGEN,		"Sysgen",		0, 0 }, \
{ MT_ISMT02,		"Emulex MT-02",		0, 0 }, \
{ MT_ISVIPER1,		"Archive QIC-150",	0, 0 }, \
{ MT_ISWANGTEK1,	"Wangtek QIC-150",	0, 0 }, \
{ MT_ISCDC,		"CDC",			0, 0 }, \
{ MT_ISKENNEDY,		"Kennedy",		0, 0 }, \
{ MT_ISHP,		"HP-88780",		0, 0 }, \
{ MT_ISEXABYTE,		"Exabyte",		0, 0 }, \
{ 0 } \
}


/*
 * Constants for mt_type byte
 */

/*
 * Check if mt_type is one of the SCSI tape devices.
 */
#define MT_TYPE_SCSI(mt_type) \
	((mt_type >= MT_ISSYSGEN11)  &&  (mt_type <= MT_ISCCS32))

/*
 * Older 1/4-inch cartridge tapes devices.
 * A blocking factor of 126 is recommended for compatibility.
 * A larger blocking factor may be used for improved streaming
 * performance.
 */
#define MT_TYPE_OLD_CARTRIDGE(mt_type) \
	((mt_type == MT_ISSYSGEN11)  ||  (mt_type == MT_ISSYSGEN)  || \
	 (mt_type == MT_ISAR))

/*
 * Current 1/4-inch cartridge tape devices.
 * A blocking factor of 40 (to 60) is recommended for
 * optimal streaming performance.
 */
#define MT_TYPE_NEW_CARTRIDGE(mt_type) \
	(mt_type >= MT_ISDEFAULT  &&  mt_type <= MT_ISCCS16) 

/*
 * All 1/4-inch cartridge tape devices.
 * A blocking factor of 126 is recommended for compatibility
 * (during writes).  See above for specific recommendations for
 * reading.
 */
#define MT_TYPE_CARTRIDGE(mt_type) \
	((mt_type >= MT_ISSYSGEN11  &&  mt_type <= MT_ISCCS16)  || \
	 (mt_type == MT_ISAR))

/*
 * All 1/2-inch reel tape devices.
 * A blocking factor of 20 is recommended for compatibility.
 */
#define MT_TYPE_REEL(mt_type) \
	((mt_type >= MT_ISCDC  &&  mt_type <= MT_ISCCS32)  || \
	 (mt_type >= MT_ISTS   &&  mt_type <= MT_ISCPC)    || \
	 (mt_type == MT_ISXY))

/* mag tape io control commands */
#define	MTIOC		('m'<<8)
#define	MTIOCTOP	(MTIOC|1)		/* do a mag tape op */
#define	MTIOCGET	(MTIOC|2)		/* get tape status */

#ifndef KERNEL
#define	DEFTAPE	"/dev/rmt/c0s0"
#endif
