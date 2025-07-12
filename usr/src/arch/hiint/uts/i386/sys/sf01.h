/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990 Intel Corp.						*/
/*	  All Rights Reserved  								*/

#ident	"@(#)hiint:uts/i386/sys/sf01.h	1.1"
/* note: tabstop = 4 */

#include "sys/sdi_edt.h"

/* defines from fd.h */
/*
 * the floppy disk minor device number is interpreted as follows:
 * bits:
 *		 7  4 3 21 0
 * 		+----+-+--+-+
 * 		|fmt |s|pt|u|
 * 		+----+-+--+-+
 * codes:
 *		u   - unit no. (0 or 1)
 *		pt  - partition no. (0 - 3)
 *		s   - single/double sided (1 = single)
 *		fmt - format code, no. of bytes per sector/ no. of sectors per trk.
 */

#define	PARTITION(x)	((getminor(x) & 0x06) >> 1)
#define UNIT(x)			(getminor(x) & 0x01)
#define	FRMT(x)			((getminor(x) & 0xf0) >> 4)
#define	SIDES(x)		((getminor(x) & 0x08) ? 1 : 2)

#define OPENED  0x01
#define OPENING 0x02
#define CLOSING 0x08
#define EXCLUSV 0x10

/* sf01 defines and declarations */

#define SD_CHAR			('D' << 8)
#define SF01ARRAYSZ(x)	(sizeof(x) / sizeof(x[0]))

/* 
 * Job data structure for each active job.
 */

struct job{
	struct	job	*j_forw;	/* Next job in the work queue 		*/
	struct	job	*j_back;	/* Previous job in the work queue	*/
	struct	sb	*j_cont;	/* SCB for this job 				*/
	struct	job	*j_mate;	/* Other job when duplexed write	*/
	void	(*j_done)();	/* Function to call when done 		*/
	buf_t	*j_bp;			/* Pointer to buffer header			*/
	struct	disk	*j_dk;	/* Physical device to be accessed	*/
	union	sc{				/* SCSI command block 				*/
		struct scs cs;
		struct scm cm;
	} j_cmd;		
};

/*
 * Defines for Mode sense data command.
 */

#define FLEX_DISK_PAGE	5	/* scsi page 5 is floppy disk parameter page */
#define	SENSE_PLH_SZ	4	/* Length of page header		*/
#define FPGSZ 			44	/* Length of header (4) +
							 * block descriptor (8) +
							 * flex disk page parameter area (32)
							 */

/*
 *  Define the Mode Sense Parameter List Header format.
 */
typedef struct sense_plh {
	unchar 	plh_len;		/* Data Length				*/
	unchar 	plh_type;		/* Medium Type				*/
	uint  	plh_res : 7;	/* Reserved					*/
	uint  	plh_wp : 1;		/* Write Protect			*/
	unchar 	plh_bdl;		/* Block Descriptor Length	*/
} SENSE_PLH_T;

/*
 *  Define the Mode Sense Block descriptor format
 */
typedef struct sense_blkd {
	unchar 	blkd_den;			/* Density Code			*/
	uint 	blkd_nblks : 24;	/* Number of blocks		*/
	unchar 	blkd_res; 			/* Reserved				*/
	uint  	blkd_blen : 24;		/* Block Length			*/
} SENSE_BLKD_T;

/*  
 * Define the Flexible Disk Drive Geometry Parameter Page format.
 */
typedef struct fdpage {
	int		pg_pc		: 6;	/* Page Code				*/
	int		pg_res1		: 2;	/* Reserved			 		*/
	unchar	pg_len;				/* Page Length				*/
	int		pg_trnrate	: 16;	/* Transfer Rate			*/
	unchar	pg_head;			/* Number of Heads			*/
	unchar	pg_nsects;			/* Sectors per track		*/
	int		pg_secsiz	: 16;	/* Bytes per sector			*/
	int		pg_ncyls	: 16;	/* Number of cylinders		*/
	int		pg_wrpcompu	: 16;	/* Starting Cylinder - Write Precompensation */
	int		pg_redwrcur	: 16;	/* Starting Cylinder - Reduced Write Current */
	int		pg_drstep	: 16;	/* Drive Step Rate			*/
	unchar	pg_drsteppw;		/* Drive Step Pulse Width	*/
	int		pg_hdsetdel	: 16;	/* Head Settle Delay		*/
	unchar	pg_motondel;		/* Motor on delay		 	*/
	unchar	pg_motoffdel;		/* Motor off delay		 	*/
	int		pg_res2		: 5;	/* Reserved			 		*/
	int		pg_mo		: 1;	/* Motor on			 		*/
	int		pg_ssn		: 1;	/* Start sector number		*/
	int		pg_trdy		: 1;	/* True ready signal		*/
	int		pg_spc		: 4;	/* Step pulse per cylinder	*/
	int		pg_res3		: 4;	/* Reserved					*/
	unchar	pg_wrcomp;			/* Write compensation		*/
	unchar	pg_hdloaddel;		/* Head Load Delay		 	*/
	unchar	pg_hduloaddel;		/* Head Unload Delay		*/
	int		pg_pin2		: 4;	/* Pin 2					*/
	int		pg_pin34	: 4;	/* Pin 34					*/
	int		pg_pin1		: 4;	/* Pin 1					*/
	int		pg_pin4		: 4;	/* Pin 4					*/
	int		pg_medrot	: 16;	/* Medium rotation rate		*/
	int		pg_res4		: 16;	/* Reserved					*/
} FDPAGE_T;

/* data needed to keep track of floppy state */
struct sf01fdstate {
	unsigned char	fd_dindex;	/* drive data tbl index */
	char			fd_status;
	dev_t			fd_device;
	unsigned char	fd_fmt;		/* format */
	unsigned char	fd_part;	/* partition */
	unsigned short	fd_nkernb;	/* number of DEV_BSIZE blocks */
};

/*
 * The disk structure holds the job queue for each floppy drive
 * and the Request Sense data for the last error.
 */

struct disk {
	struct job *dk_forw;		/* 1st entry on work queue 		*/
	struct job *dk_back;		/* Last entry on work queue 	*/
	struct job *dk_next;		/* Next entry for HAD 			*/
	long dk_state;				/* State of this disk 			*/
	long dk_count;				/* Number of jobs on work que 	*/
	long dk_outcnt;				/* Jobs in HAD for this disk 	*/
	long dk_error;				/* Number of errors detected 	*/
	long dk_jberr;				/* Errors for this job 			*/
	long dk_sendid;				/* Timeout id for sf01send 		*/
	time_t dk_scb_time;			/* Time for SCSI cmd to complete*/
	time_t dk_start;			/* When last trasfer started	*/
	struct scsi_ad dk_addr;		/* Major/Minor number of device */
	struct sf01fdstate fd;		/* Floppy state					*/
	struct sb *dk_fltreq;		/* SCSI block for request sense */
	struct sb *dk_fltres;		/* SCSI block for reserve job 	*/
	struct sb *dk_fltrel;		/* SCSI block for release job -- fixes PR2131 */
	struct sb *dk_fltsus;		/* SCSI block for suspend job 	*/
	struct disk *dk_fltnext;	/* Next disk in RESUME list 	*/
	long dk_rescnt;				/* Number of RESUME Bus Resets 	*/
	long dk_spcount;			/* Retry count for special jobs */
	struct scs dk_fltcmd;		/* Request Sense/Reserve command*/
	struct scs dk_blkcmd;		/* Reassign/Read/Write command	*/
	struct sense dk_sense;		/* Request Sense data 			*/
	char dk_ms_data[FPGSZ];		/* Mode Sense or Select data 	*/
};

/*** Defines needed for floppy manipulation ***/
/* maximum number of drives on line at one time */
#define DRV_MAXNUM		0x2

/* State flags for the disk */
#define DKSUSP	    	0X0001		/* The HAD susupended the que	*/
#define DKSEND	    	0X0004		/* sf01send has requested timeout*/
#define DKINIT	    	0X0010		/* The disk has been initialized.*/
#define	DKFLT       	0X0200		/* Disk is recovering from a fault*/
#define	DKRESERVE   	0X0400		/* Disk is currently reserved 	*/
#define	DKRESDEVICE 	0X0800		/* Disk has been reserved 	*/
#define	DKONRESQ    	0X1000		/* Disk on the Resume Queue 	*/
#define	DKPENDRES   	0X2000		/* Disk has a pending Resume 	*/
#define DKCONFLICT  	0X4000		/* Reservation Confict on Open 	*/

struct resume{
	struct disk *res_head;		/* Next disk to use the RESUME SB*/
	struct disk *res_tail;		/* Last disk to use the RESUME SB*/
};

struct free_jobs{			/* List of free job structures	*/
	int fj_state;			/* -1 if waiting for jobs 	*/
	int fj_count;			/* Number free jobs 		*/
	struct job *fj_ptr;		/* Pointer to free list 	*/
};

/* fj_state values. */
#define FJEMPTY -1			/* The free list was empty 	*/
#define FJOK 0				/* The free list is ok 		*/

/* Floppy defines:			*/
/* Track format supported	*/
/* CAUTION: changing the order of this array or adding to it means
 * Sf01_parts and Sf01_drives need to change in space.c.
 */
#define FMT_5H			0
#define FMT_5D9	  		1
#define FMT_5D8	  		2
#define FMT_5D4	  		3
#define FMT_5D16  		4
#define FMT_5Q	  		5
#define FMT_3D	  		6
#define FMT_3H	  		7
#define FMT_AUTO  		8
#define FMT_MAX	  		8	/* largest legal value for a format		*/
#define FMT_MAXNUM		8	/* number of actual formats supported	*/
#define FMT_UNDEFINED	-1

/* Defines for partitions */
#define PART_MAX		3	/* largest legal value for a partition	*/
#define PART_MAXNUM		4	/* number of partitions supported		*/
#define PART_UNDEFINED	-1

#define SPDLOW			250
#define SPDMED			300
#define SPDHIGH			500


/* maps a drive inquiry string to its index in the drive parameter array */
struct drive_index {
	unsigned char   tc_inquiry[VID_LEN + PID_LEN];	/* TC inquiry data	*/
	unsigned char	index;							/* drive data index	*/
};

/* drive parameters for a single format */
struct drive_data {
	uint			sshift;			/* shift to convert bytes to sectors */
	unsigned char	mediumtype;		/* medium type code */
	FDPAGE_T		page_params;	/* SCSI flex disk page params */
};

/* partition data for a single format */
struct part_data {
	int     startsect;				/* sector no. where partition starts */
	int     numsects;				/* number of sectors in partition */
};

/* Space file declarations. */
extern struct tc_data Sf01_data[];	/* Array of tc device info	*/

extern struct drive_index Sf01_drive_index[];
					/* Maps drive types to formats */
extern int Sf01_drive_index_size;
					/* Number of elements in Sf01_drive_index */

extern struct part_data Sf01_parts[FMT_MAXNUM][PART_MAXNUM];
					/* Partition info */
extern struct drive_data Sf01_drives[][FMT_MAXNUM];
					/* Info needed to operate drives */

/* Additional device info	*/
extern struct drv_majors	Sf01_majors[];	/* Array of major number info	*/
extern int					Sf01_datasz;	/* Number of supported TC's    	*/
extern struct disk			*Sf01_dp;		/* Array of disk structures 	*/
extern struct job			*Sf01_jp;		/* Array of job structures 		*/
extern int 					Sf01_jobs;		/* Number of job structures 	*/
extern int					Sf01_cmajors;	/* Number of c major numbers	*/
extern char					Sf01_debug[];	/* Array of debug levels		*/

extern int					sf01_diskcnt;	/* Number of disks defined 		*/
extern int					sf01_tccnt;		/* Number of controllers defined*/
extern int					sf01_jobcnt;	/* Number of allocated jobs 	*/

/* Number of TC'c supported.	*/
#define	DATASZ	sizeof(Sf01_data)/sizeof(struct tc_data)

/* Minor number macros. */
#define DKMJINS(x)	(sf01instbl[getmajor(x)] * 16)

#define DKNOTCS		-1			/* No TC's configured in system	*/
#define DKNOMAJ		-1			/* Unsupported major number		*/

#define JTIME		10000		/* Ten seconds for a job 		*/
#define LATER		20000		/* How much later when retrying */
#define FORMAT_TIME	240000		/* Four minutes for a format	*/

#define SF01_RETRYCNT	2					/* Retry count 				*/
#define SF01_RST_ERR	(SF01_RETRYCNT-1)	/* Reset error retry count 	*/
#define SF01_MAXSIZE	0xF000				/* Maximum job size 		*/

#define SF01_DEBUGFLG	73		 /* Turn debugs on/off 		*/
