/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_ICS_H
#define _SYS_ICS_H

/*	Copyright (c) 1986, 1987, 1988  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)mbus:uts/i386/sys/ics.h	1.3.1.2"

#ifndef _SYS_CRED_H
#include "sys/cred.h"
#endif

/*
 *	Multibus II is spec'ed for up to 21 slots
*/
#define ICS_MAX_SLOT 21

/*
 *	Interconnect space is 512 bytes long
*/
#define ICS_MAX_REG 511

/*
 *	Slot ID 31 is an alias for the current slot:
*/
#define ICS_MY_SLOT_ID			0x1f

/*
 *	Interconnect space register offsets for the header record.
 *	Every board has one of these at offset 0 in interconnect space.
*/
#define ICS_VendorIDL			0
#define ICS_VendorIDH			1
#define ICS_ProductCode			2		/* 10 char product "name" */
#define ICS_HardwareTestRev		16
#define ICS_ClassID				17
#define ICS_ProgramTableIndex	22
#define ICS_NMIEnable			23
#define ICS_GeneralStatus		24
#define ICS_GeneralControl		25
#define ICS_BistSupportLevel	26
#define ICS_BistDataIn			27
#define ICS_BistDataOut			28
#define ICS_BistSlaveStatus		29
#define ICS_BistMasterStatus	30
#define ICS_BistTestID			31

/*
 *	Here are some defines related to ICS_ProgramTableIndex:
*/
#define ICS_FW_INDEX	0x00

/*
 *	Here are some defines related to ICS_NMIEnable:
*/
#define ICS_ENABLE_NMI	0x80
#define ICS_DEBUG_NMI	0x02
#define ICS_DEBUG_MASK	0x02
#define ICS_SW_NMI		0x04
#define ICS_SWNMI_MASK	0x04

/*
 *	From Appendix B of the Multibus II Interconnect Interface Spec:
 *	(I put them in decimal because that is what the manual used).
*/
#define ICS_EXTENDED_REC_TYPE	0
#define ICS_MEMORY_REC_TYPE		1
#define ICS_PSB_MEMORY_REC_TYPE	2
#define ICS_LBX_MEMORY_REC_TYPE	3
#define ICS_PARITY_CONTROL_TYPE	4
#define ICS_CACHE_MEM_REC_TYPE	5
#define ICS_PSB_CONTROL_TYPE	6
#define ICS_LBX_PRIMARY_TYPE	7
#define ICS_CSM_RECORD_TYPE		8
#define ICS_DATE_TIME_REC_TYPE	9
#define ICS_RESERVED_TYPE		10
#define ICS_PROTECTION_REC_TYPE	11
#define ICS_PSB_WINDOW_REC_TYPE	12
#define ICS_SERIAL_COM_REC_TYPE	13
#define ICS_PERIPH_COM_REC_TYPE	14
#define ICS_FW_COM_REG			15
#define ICS_HOST_ID_TYPE		16
#define ICS_LOCAL_MEM_REC_TYPE	17
#define ICS_PROC_PROTECT_TYPE	18
#define ICS_LOCAL_PROC_REC_TYPE	19
#define ICS_MEMORY_SPACE_TYPE	128
#define ICS_IO_SPACE_TYPE		129
#define ICS_MESSAGE_SPACE_TYPE	130
#define ICS_INITIALIZATION_TYPE	131
#define	ICS_BOARD_SPECIFIC_REC	254
#define ICS_EOT_TYPE			255

/*
 *	All interconnect records (except the header record) have
 *	a record type, a length, then data:
*/
#define ICS_TYPE_OFFSET		0
#define ICS_LENGTH_OFFSET	1
#define ICS_DATA_OFFSET		2

/*
 *	Defines related to ICS_PSB_CONTROL_TYPE
*/
#define ICS_SLOT_ID_OFFSET		(ICS_DATA_OFFSET+0)

/*
 *	Defines related to ICS_PARITY_CONTROL_TYPE
*/
#define ICS_PARITY_OFFSET	(ICS_DATA_OFFSET+0)
#define ICS_PARITY_MASK		0x80

/*
 *	Defines related to ICS_LOCAL_PROC_REC_TYPE
*/
#define ICS_RESET_OFFSET	(ICS_DATA_OFFSET+0)
#define ICS_RESET_MASK		0x01

#define ICS_COLD_RESET		0x01
#define ICS_WARM_RESET		0x02
#define ICS_PFRR_RESET		0x03 	/* power-fail/recovery reset */
#define ICS_PLIR_RESET		0x04	/* protected/live-insertion  */
#define ICS_LOCAL_RESET		0x07	/* single-agent reset */
/*
 *	I have no idea what these are:
*/
#if 0
#define ICS_PC16_STATUS_OFFSET 3
#define ICS_PC16_CONFIG_OFFSET 6
#define ICS_PSB_SLOT_ID_REG 0x2D
#endif

#define ICS_LO(x)	((x)&0xFF)
#define ICS_HI(x)	(((x)>>8)&0xFF)

#define NAMESZ 11
typedef struct slot {
	unsigned short s_hostid;
	unsigned char  s_msgid;
	unsigned char  s_pcode[NAMESZ];
} ics_slot_t;

struct ics_struct {
	unsigned short slot_id;	/* slot id of the target board */
	unsigned short reg_id;	/* register to be read */
	unsigned short count;	/* number of registers to read */
	unsigned char buffer[1];		/* data buffer */
};

struct ics_rw_struct {
	unsigned short slot_id;	/* slot id of the target board */
	unsigned short reg_id;	/* register to be read */
	unsigned short count;	/* number of registers to read */
	unsigned char *buffer;		/* data buffer */
};

/* table to reconfigure UNIX for multiple cpu systems */

minor_t	rootdev_minor;
minor_t	swapdev_minor;
minor_t	pipedev_minor;
minor_t	dumpdev_minor;

#define DEVVAL		128
#define CPUNUMSIZE	128
#define UNX_DEV		"unx_dev"
#define UNX_CPUNUM	"unx_cpunum"

struct ics_bdev {
	major_t	major;
	int	base;
};

/*
 *	These are user mode ioctl commands:
*/
#define ICS_WRITE_ICS 	4
#define ICS_READ_ICS 	5
#define ICS_FIND_REC	10
#define ICS_MY_SLOTID	11
#define ICS_MY_CPUNUM	12

/* defintion of ioctl records passed from user */
struct ics_rdwr_args{
	unsigned short	ics_slot;	/* Slot number */
	unsigned short	ics_reg;	/* Register number */
	caddr_t			ics_buf;	/* Buffer to read from/write to */
	int				ics_count;	/* Number of bytes to read/write */
};

struct ics_frec_args{
	unsigned short	ics_rval;	/* Value returned */
	unsigned short	ics_slot;	/* Slot number */
	unsigned short	ics_recid;	/* Record ID to search for */
};

 

#ifdef _KERNEL
#if defined __STDC__
/*
 * Since the prototype definitions are only allowed word parameters, all
 * shorts and char's have been changed to int in the prototypes only
 */
extern int 	ics_cpunum();
extern unsigned char ics_myslotid();
extern int 	icsinit();
extern int 	icsopen (dev_t *, int, int, struct cred *);
extern int 	icsclose (dev_t *, int, int, struct cred *);
extern int 	ics_rdwr(int, struct ics_rw_struct *);
extern int 	ics_rw (int, struct ics_struct *);
extern int 	ics_hostid();
extern int 	icsioctl(dev_t, int, caddr_t, int, struct cred *, int *);
extern int 	ics_agent_cmp(char *[], unsigned int);
extern int 	ics_autoconfig();
extern int 	ics_find_rec(unsigned int, unsigned int);
extern int 	ics_read(unsigned int, unsigned int);
extern int 	ics_write(unsigned int, unsigned int, unsigned int);
#else
extern int 	ics_cpunum();
extern unsigned char ics_myslotid();
extern int 	icsinit();
extern int 	icsopen ();
extern int 	icsclose ();
extern int 	ics_rdwr();
extern int 	ics_rw ();
extern int 	ics_hostid();
extern int 	ics_find_rec();
extern int 	ics_read();
extern int	ics_write();
extern int 	icsioctl();
extern int 	ics_agent_cmp();
extern int 	ics_autoconfig();
#endif
#endif /* _KERNEL */

#endif	/* _SYS_ICS_H */
