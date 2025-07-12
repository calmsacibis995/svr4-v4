/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Copyright (c) 1990  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)hiint:uts/i386/master.d/f87033/space.c	1.1"

#include "sys/types.h"
#include "sys/proc.h"
#include "sys/cred.h"
#include "sys/dma.h"
#include "sys/sdi_edt.h"
#include "sys/sdi.h"
#include "sys/scsi.h"
#include "sys/spcscsi.h"
#include "sys/spc.h"
#include "sys/f87033.h"
#include "sys/conf.h"
#include "config.h"

int	scsidevflag = D_NEW | D_DMA;			/* SVR4 Driver Flags */

#define SCSI_BLKS 		(100 * SCSI_CNTLS)		/* Total # of request blks	*/
#define SCSI_EDT_SZ		(MAX_TCS * SCSI_CNTLS)	/* Total # of EDT entries	*/

/*
 * SDI defined data structures 
 */
long			sdi_hacnt = SCSI_CNTLS;		/* Total # of SCSI controllers	*/
long			sdi_started = FALSE;		/* SDI initialization flag		*/
struct ver_no	sdi_ver;					/* Host Adaptor/SDI version #	*/

/*
 * Constants defined at kernel build time
 */
const major_t	sc_cmajor = SCSI_CMAJOR_0;	/* Char Major # 				*/
const uint_t	sc_edtcnt = SCSI_EDT_SZ;	/* Total # of EDT entries		*/
const uint_t	sc_sbcnt = SCSI_BLKS;		/* Total # of Request Blocks	*/
const uint_t	sc_hiwat = 8;				/* LU queue high water mark		*/
const uint_t	sc_lowat = 2;				/* LU queue low water mark		*/

/*
 * Data structure storage allocation
 */
struct scsi_ha		sc_ha[SCSI_CNTLS];		/* Host Adaptor information		*/
struct scsi_edt		sc_edt[SCSI_EDT_SZ];	/* EDT - Equipted Device Table	*/
struct srb			sc_sbtab[SCSI_BLKS];	/* SCSI (Request) Block pool	*/




/*
 * Single-ended SCSI Bus SPC Configuration
 */
struct spc_conf	spc_se_conf = {
	CFG_NO_FLAGS,					/* SCSI Bus configuration flags			*/
	SE_SCSI_ID_SHIFT,				/* SCSI ID bit pos, base-0, brd_scsi_id */
	SCSI_0_VECT,					/* Interrupt vector						*/
	SCSI_0_SIOA,					/* Starting I/O Address					*/
	SCSI_0_EIOA,					/* Ending I/O Address					*/
	SCSI_CHAN,						/* DMA Channel							*/

	SCSI_0_SIOA + 0x00,				/* SPC: SCSI ID							*/
	SCSI_0_SIOA + 0x02,				/* SPC: Control Register				*/
	SCSI_0_SIOA + 0x04,				/* SPC: Command Register				*/
	SCSI_0_SIOA + 0x06,				/* SPC: Transfer Mode (TMOD) Register	*/
	SCSI_0_SIOA + 0x08,				/* SPC: Interrupt Status Register		*/
	SCSI_0_SIOA + 0x0A,		/* Read	 - SPC: Phase Sense Register			*/
	SCSI_0_SIOA + 0x0A,		/* Write - SPC: Diagnostic Control Register		*/
	SCSI_0_SIOA + 0x0C,				/* SPC: Status Register					*/
	SCSI_0_SIOA + 0x0E,				/* SPC: Error Status Register			*/
	SCSI_0_SIOA + 0x10,				/* SPC: Phase Control Register			*/
	SCSI_0_SIOA + 0x12,				/* SPC: Modified Byte Control Register	*/
	SCSI_0_SIOA + 0x14,				/* SPC: Data Buffer Register			*/
	SCSI_0_SIOA + 0x16,				/* SPC: Temporary Register				*/
	SCSI_0_SIOA + 0x18,				/* SPC: Transfer Counter - High Byte	*/
	SCSI_0_SIOA + 0x1A,				/* SPC: Transfer Counter - Middle Byte	*/
	SCSI_0_SIOA + 0x1C,				/* SPC: Transfer Counter - Low Byte		*/
	SCSI_0_SIOA + 0x1E,				/* SPC: External Buffer Register		*/

	SCSI_0_SIOA + 0x20,		/* Read  - Board: SCSI ID jumpers				*/
	SCSI_0_SIOA + 0x20,		/* Write - Board: Inhibit SCSI interrupts		*/
	SCSI_0_SIOA + 0x22,				/* Board: Force a SCSI interrupt		*/
	SCSI_0_SIOA + 0x24,				/* Board: SCSI/DMA FIFO xfer direction	*/
	SCSI_0_SIOA + 0x26,				/* Board: SCSI/DMA FIFO xfer width		*/ 
	SCSI_0_SIOA + 0x28,				/* Board: SCSI/DMA FIFO xfer mode 		*/
	SCSI_0_SIOA + 0x2A,				/* Board: Reset SCSI/DMA FIFO			*/
	SCSI_0_SIOA + 0x2C,				/* Board: Reserved						*/
	SCSI_0_SIOA + 0x2E,				/* Board: Reserved						*/

	CFG_SANITY						/* Configuration sanity word			*/
};



#ifdef SCSI_1
/*
 * Differential SCSI Bus SPC Configuration 
 */
struct spc_conf	spc_df_conf = {
	CFG_RESET_BUS,					/* SCSI Bus configuration flags			*/
	DF_SCSI_ID_SHIFT,				/* SCSI ID bit pos, base-0, brd_scsi_id */
	SCSI_1_VECT,					/* Interrupt vector						*/
	SCSI_1_SIOA,					/* Starting I/O Address					*/
	SCSI_1_EIOA,					/* Ending I/O Address					*/
	SCSI_CHAN + 1,					/* DMA Channel							*/
	/*
	 * Note: DMA Chan kludge is required since 'mdevice' allows
	 * the user to specifiy only one DMA channel
	 */

	SCSI_1_SIOA + 0x10,				/* SPC: SCSI ID							*/
	SCSI_1_SIOA + 0x12,				/* SPC: Control Register				*/
	SCSI_1_SIOA + 0x14,				/* SPC: Command Register				*/
	SCSI_1_SIOA + 0x16,				/* SPC: Transfer Mode (TMOD) Register	*/
	SCSI_1_SIOA + 0x18,				/* SPC: Interrupt Status Register		*/
	SCSI_1_SIOA + 0x1A,		/* Read	 - SPC: Phase Sense Register			*/
	SCSI_1_SIOA + 0x1A,		/* Write - SPC: Diagnostic Control Register		*/
	SCSI_1_SIOA + 0x1C,				/* SPC: Status Register					*/
	SCSI_1_SIOA + 0x1E,				/* SPC: Error Status Register			*/
	SCSI_1_SIOA + 0x00,				/* SPC: Phase Control Register			*/
	SCSI_1_SIOA + 0x02,				/* SPC: Modified Byte Control Register	*/
	SCSI_1_SIOA + 0x04,				/* SPC: Data Buffer Register			*/
	SCSI_1_SIOA + 0x06,				/* SPC: Temporary Register				*/
	SCSI_1_SIOA + 0x08,				/* SPC: Transfer Counter - High Byte	*/
	SCSI_1_SIOA + 0x0A,				/* SPC: Transfer Counter - Middle Byte	*/
	SCSI_1_SIOA + 0x0C,				/* SPC: Transfer Counter - Low Byte		*/
	SCSI_1_SIOA + 0x0E,				/* SPC: External Buffer Register		*/

	SCSI_1_SIOA + 0x20,		/* Read  - Board: SCSI ID jumpers				*/
	SCSI_1_SIOA + 0x20,		/* Write - Board: Inhibit SCSI interrupts		*/
	SCSI_1_SIOA + 0x22,				/* Board: Force a SCSI interrupt		*/
	SCSI_1_SIOA + 0x24,				/* Board: SCSI/DMA FIFO xfer direction	*/
	SCSI_1_SIOA + 0x26,				/* Board: SCSI/DMA FIFO xfer width		*/ 
	SCSI_1_SIOA + 0x28,				/* Board: SCSI/DMA FIFO xfer mode 		*/
	SCSI_1_SIOA + 0x2A,				/* Board: Reset SCSI/DMA FIFO			*/
	SCSI_1_SIOA + 0x2C,				/* Board: Reserved						*/
	SCSI_1_SIOA + 0x2E,				/* Board: Reserved						*/

	CFG_SANITY						/* Configuration sanity word			*/
};
#endif



/*
 * Host Adaptor Configuration Data
 */
const struct spc_conf	*sc_conf[SCSI_CNTLS] = {
		&spc_se_conf

#ifdef SCSI_1
	,
	/* Host Adaptor 1 */
		&spc_df_conf
#endif
};
