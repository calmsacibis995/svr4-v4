/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_SPC_H
#define _SYS_SPC_H

/*	Copyright (c) 1990  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)hiint:uts/i386/sys/spc.h	1.1"


/* 386/258 SPC Manifest Constants */

/*
 * ICS Parameters
 */
#define	RESET_FLG_OFFSET	15		/* Reset Reg offset in ICS F/W Comm Rec	*/

#define	SE_SCSI_ID_SHIFT	0x00		/* Bit pos of Single-Ended SCSI ID	*/
#define	DF_SCSI_ID_SHIFT	0x04		/* Bit pos of Differential SCSI ID	*/
#define	SCSI_ID_MASK		0x07		/* Bit mask of relavent SCSI ID bits */

/*
 * SPC Register Bit Definitions:
 * Refer to SPC Hardware Reference Manual for details
 */
#define SCTL_RESET				0x080
#define SCTL_DATA_RESET			0x040
#define SCTL_ARBIT_ENABLE 		0x010
#define SCTL_PARITY_ENABLE		0x008
#define SCTL_SELECT_ENABLE		0x004
#define SCTL_RESELECT_ENABLE	0x002
#define SCTL_INT_ENABLE			0x001

#define SCMD_BUS_RELEASE		0x000
#define SCMD_SELECT				0x020
#define SCMD_RESET_ATN			0x040
#define SCMD_SET_ATN			0x060
#define SCMD_XFER   			0x080
#define SCMD_XFER_PAUSE			0x0A0
#define SCMD_RESET_ACK			0x0C0
#define SCMD_SET_ACK			0x0E0
#define SCMD_PAD				0x001

#define SCMD_RST_OUT			0x010
#define SCMD_INTERCEPT_XFER		0x008
#define SCMD_PROG_XFER			0x004
#define SCMD_TERM_MODE			0x001

#define INTS_SELECTED			0x080
#define INTS_RESELECTED			0x040
#define INTS_DISCONNECTED		0x020
#define INTS_CMD_COMP			0x010
#define INTS_SERVICE_REQ		0x008
#define INTS_TIMEOUT			0x004
#define INTS_HARD_ERROR			0x002
#define INTS_RESET				0x001

#define PSNS_REQ				0x080
#define PSNS_ACK				0x040
#define PSNS_ATN				0x020
#define PSNS_SEL				0x010
#define PSNS_BSY				0x008
#define PSNS_MSG				0x004
#define PSNS_CD					0x002
#define PSNS_IO					0x001

#define PSNS_XFER_PHASE_MSK		0x007
#define PSNS_DATA_OUT_PHASE		0x000
#define PSNS_DATA_IN_PHASE		0x001
#define PSNS_CMD_PHASE			0x002
#define PSNS_STATUS_PHASE		0x003
#define PSNS_MSG_OUT_PHASE		0x006
#define PSNS_MSG_IN_PHASE		0x007

#define PCTL_BUSFREE_INT_ENABLE	0x080
#define PCTL_MSG				0x004
#define PCTL_CD					0x002
#define PCTL_IO					0x001

#define PCTL_DATA_OUT_PHASE		0x000
#define PCTL_DATA_IN_PHASE		0x001
#define PCTL_CMD_PHASE			0x002
#define PCTL_STATUS_PHASE		0x003
#define PCTL_MSG_OUT_PHASE		0x006
#define PCTL_MSG_IN_PHASE		0x007

#define SSTS_INIT_CONN			0x080
#define SSTS_TARG_CONN			0x040
#define SSTS_SPC_BUSY			0x020
#define SSTS_XFER_IN_PROG		0x010
#define SSTS_SCSI_RESET			0x008
#define SSTS_TC_ZERO			0x004
#define SSTS_DREG_FULL			0x002
#define SSTS_DREG_EMPTY			0x001

/*
 * XXX - Should add 'reset' flag and 'target' flag
 */
#define SSTS_BUSY	(SSTS_INIT_CONN | SSTS_SPC_BUSY | SSTS_XFER_IN_PROG)

/*
 * Timeout values
 */
#define SELECT_TIMEOUT		0x0F42	/* Select timeout (~250 ms)				*/
#define SELECT_TWAIT		0x0004	/* Bus Free->Arb/Select delay (~1250 ns)*/

#define SPC_CMD_DELAY			(void) spc_cmd_delay_proc()
#define SPC_ARB_DELAY			(void) spc_arb_delay_proc()

#define SPC_POLL_DELAY_COUNT	10
#define SPC_POLL_COUNT_LIMIT	0x1000000	/* about 30 s */

/*
 * Synchronous Transfer Parameters
 */
#define SPC_XFER_PERIOD				62
#define SPC_REQ_ACK_OFFSET			8

#define SPC_5M_SYNC_XFER_1			50
#define SPC_5M_SYNC_XFER_TMOD_1		0x80

#define SPC_5M_SYNC_XFER_2			75
#define SPC_5M_SYNC_XFER_TMOD_2		0x84

#define SPC_5M_SYNC_XFER_3			100
#define SPC_5M_SYNC_XFER_TMOD_3		0x88

#define SPC_5M_SYNC_XFER_4			125
#define SPC_5M_SYNC_XFER_TMOD_4		0x8C

#endif	/* _SYS_SPC_H */
