/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _SYS_SPCSS_H
#define _SYS_SPCSS_H

/*	Copyright (c) 1990  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)hiint:uts/i386/sys/spcscsi.h	1.1"

/* Ideally these should be in uts/i386/sys/scsi.h */

/*
 * SCSI Message Types
 */
#define SS_CMD_COMP_MSG				0X00
#define SS_EXTENDED_MSG				0X01
#define SS_SAVE_DATA_PTRS_MSG		0X02
#define SS_RESTORE_PTRS_MSG			0X03
#define SS_DISCONN_MSG				0X04
#define SS_INIT_DET_ERR_MSG			0X05
#define SS_ABORT_MSG				0X06
#define SS_REJECT_MSG				0X07
#define SS_NO_OP_MSG				0X08
#define SS_MSG_PARITY_ERR_MSG		0X09
#define SS_LINK_CMD_COMPLETE_MSG	0X0A
#define SS_LINK_CMDCOMP_FLAG_MSG	0X0B
#define SS_BUS_DEVICE_RESET_MSG		0X0C
#define SS_ABORT_TAG_MSG			0X0D
#define SS_CLEAR_QUEUE_MSG			0X0E
#define SS_INITIATE_RECOVERY_MSG	0X0F
#define SS_SIMPLE_QUE_TAG_MSG		0X20
#define SS_HEADOF_QUE_TAG_MSG		0X21
#define SS_ORDERED_QUE_TAG_MSG		0X22
#define SS_IGNORE_WIDE_RES_MSG		0X23
#define SS_IDENTIFY_MSG				0X80
#define SS_DISCONN_IDENTIFY_MSG		0XC0


/*
 * SCSI Extended Message Types
 */
#define SS_MOD_DATA_PTRS_EMSG		0X00
#define SS_SYNC_XFER_REQ_EMSG		0X01
#define SS_WIDE_XFER_REQ_EMSG		0X03


/*
 * SCSI Status Values
 */
#define S_CMDTERM					0X22	/* command terminated */
#define S_QUEFULL					0X28	/* queue full */


/*
 * SCSI Op-codes for Group 0 Command
 */
#define SS_COPY						0X18	/* copy */
#define SS_REC_DIAG_RESULTS			0X1C	/* receive diagnostics results */
#define SS_PREFETCH					0X34	/* enable pre-fetch */
#define SS_LOCK_UNLOCK_CACHE		0X36	/* lock-unlock cache */
#define SS_COMPARE					0X39	/* compare */
#define SS_COPY_VERIFY				0X3A	/* copy and verify */
#define SS_READ_LONG				0X3E	/* read long */
#define SS_CHANGE_DEFINITION		0X40	/* change definition */
#define SS_LOG_SELECT				0X4C	/* log select */
#define SS_LOG_SENSE				0X4D	/* log sense */
#define SM_MSELECT					0X55	/* mode select extended (10) */
#define SM_MSENSE					0X5A	/* mode select extended (10) */


/*
 * SCSI Op-codes for Direct/Sequential-Access devices
 */
#define SS_FORMAT_UNIT				0X04	/* format unit */
#define	SM_SEEK						0X2B	/* seek extended (10) */
#define	SS_WRITE_VERIFY				0X2E	/* write and verify */
#define	SS_VERIFY					0X2F	/* verify */
#define	SS_SEARCHHI					0X30	/* search data high */
#define	SS_SEARCHEQ					0X31	/* search data equal */
#define	SS_SEARCHLI					0X32	/* search data low */
#define	SS_SYNC_CACHE				0X35	/* synchronize cache */
#define	SS_WRITE_LONG				0X3F	/* write long */
#define	SS_WRITE_SAME				0X41	/* write same */


/*
 * SCSI COPY command function codes
 */
#define SC_DTOSEQ					0X00	/* direct access to sequential */
#define SC_SEQTOWD					0X01	/* sequential to write direct acc */
#define SC_DTOWD					0X02	/* direct access to writ direct */
#define SC_SEQTOSEQ					0X03	/* sequential to sequential */
#define SC_SEQIMG					0X04	/* sequential access image copy */


/*
 * Peripheral device type
 */
#define ID_SCANNER					0x06	/* scanner device */
#define ID_OPTICAL_MEM				0x07	/* optical memory device */
#define ID_MEDCHANGER				0x08	/* medium changer device */
#define ID_COMM						0x09	/* communication device */


/*
 * Interface Identifier codes
 */
#define II_SCSI						0x00	/* SCSI reference std X3.131 */
#define II_SMI						0x01	/* SMI reference std X3.91M-1987 */
#define II_ESDI						0x02	/* ESDI reference std X3.170 */
#define II_IPI2						0x03	/* X3.130-1986; X3T9.3/87-002 */
#define II_IPI3						0x04	/* X3.132-1987; X3.147-1988 */

/*
 * SCSI Direct-Access log parameter page codes
 */
#define	SP_LOG_PAGES				0X00	/* supported log pages */
#define	SP_BUF_ORUR					0X01	/* buffer over-run/under-run */
#define	SP_ECP_WRITE				0X02	/* error counter page write */
#define	SP_ECP_READ					0X03	/* error counter page read */
#define	SP_ECP_RDREV				0X04	/* error counter page rd reverse */
#define	SP_ECP_VER					0X05	/* error counter page verify */
#define	SP_NON_MEP					0X06	/* non-medium error page */
#define	SP_LAST_EEP					0X07	/* last n error events page */


/*
 * SCSI Direct-Access medium type codes
 */
#define	SF_DEFAULT				0X00	/* default medium type */
#define	SF_FDSS_UNSP			0X01	/* SS Flexible disk - unspecified */
#define	SF_FDDS_UNSP			0X02	/* DS Flexible disk - unspecified */
#define SF_8INSSSD				0X05	/* 8 in   	- SS/SD			*/
#define SF_8INDSSD				0X06	/*        	- DS/SD			*/
#define SF_8INSSDD				0X09	/*	      	- SS/DD			*/
#define SF_8INDSDD				0X0A	/*	      	- DS/DD			*/
#define SF_5INSSSD				0X0D	/* 5.25 in	- SS/SD			*/
#define SF_5INDSDD				0X12	/*		  	- DS/DD			*/
#define SF_5INDSDD96			0X16	/*		  	- DS/DD 96 tpi	*/
#define SF_5INDSQD				0X1A	/*		  	- DS/QD 96 tpi	*/
#define ST_3INDSDD				0X1E	/* 3.5 in	- DS/DD 135 tpi	*/


/*
 * SCSI Direct-Access/Sequential access mode page codes
 */
#define	SM_VENSP				0X00	/* vendor specific -no page format */
#define	SM_RWERP				0X01	/* read-write error recovery page */
#define	SM_DIS_RECP				0X02	/* disconnect-reconnect page */
#define	SM_FMTDP				0X03	/* format device page */
#define	SM_RDGP					0X04	/* rigid disk geometry page */
#define	SM_FLEXDP				0X05	/* flexible disk page */
#define	SM_VERP					0X07	/* verify error recovery page */
#define	SM_CACHEP				0X08	/* cacheing page */
#define	SM_PDP					0X09	/* peripheral device page */
#define	SM_CONTMP				0X0A	/* control mode page */
#define	SM_DCP					0X10	/* device configuration page */
#define	SM_MPP1					0X11	/* medium partition page 1 */
#define	SM_MPP2					0X12	/* medium partition page 2 */
#define	SM_MPP3					0X13	/* medium partition page 3 */
#define	SM_MPP4					0X14	/* medium partition page 4 */
#define	SM_MTSP					0X0B	/* medium types supported page */
#define	SM_NPP					0X0C	/* notch and partition page */
#define	SM_RALLP				0X3F	/* ret all pages;valid for MODE SENSE */


/*
 * SCSI Sequential-Access command codes
 */
#define	SS_SPACEB				0X00	/* space in blocks */
#define	SS_SPACEFM				0X01	/* space in filemarks */
#define	SS_SPACE_SFM			0X02	/* space in sequential filemarks */
#define	SS_SPACEEOD				0X03	/* space in end-of-data */
#define	SS_SPACE_SETM			0X04	/* space in set-marks */
#define	SS_SPACE_SEQSM			0X05	/* space in sequential set-marks */


/*
 * Misc Buffer Lengths
 */
#define SS_CMD_LEN					6
#define SS_ECMD_LEN					10
#define SS_MAX_CMD_LEN				12
#define SS_MSG_LEN					1
#define SS_EXT_MSG_HDR_LEN			2
#define SS_STATUS_LEN				1

#define SS_SENSE_DATA_LEN			13
#define SS_READ_CAP_LEN				8
#define SS_DEFECT_LIST_HDR_LEN		4
#define SS_EXT_MSG_BUF_LEN			16
#define SS_SYNC_XFER_REQ_MSG_LEN	3

#define SS_PG_3_LEN					24
#define SS_PG_4_LEN					20
#define SS_PG_5_LEN					32
#define SS_MODE_SENSE_HDR_LEN		4
#define SS_BLOCK_DESC_LEN			8
#define SS_MSH_BD_LEN				12	/* 8 + 4 */
	
/*
 * Mode Sense + Mode Select Bit Encodings
 */
#define SS_PG_CTL_CURR				0X00
#define SS_PG_CTL_CHG				0X40
#define SS_PG_CTL_DEF				0X80
#define SS_PG_CTL_SAVED				0XC0

#define SS_PG_FMT					0X10
#define SS_SAVE_MODE				0X01
	
#define SS_PG_5_TRUE_RDY			0X80
#define SS_PG_5_SSN					0X40
#define SS_PG_5_MO					0X20
	
#define SS_PG_5_PIN_34_RDY 			0X10
#define SS_PG_5_PIN_2_SPEED			0X03
#define SS_PG_5_PIN_2_HI_DEN		0X05
	
	
/*
 * SASD Command Parameter Bit Encodings
 */
#define SS_SASD_READ_FIXED		0X01	/* Read/Write: Fixed-length flag	*/
#define SS_SASD_LOAD_RETENSION	0X02	/* Load/Unload: Retension flag		*/ 
#define SS_SASD_LOAD_LOAD		0X01	/* Load/Unload: Load/Unload flag	*/ 
#define SS_SASD_ERASE_LONG		0X01	/* Erase: Long flag					*/


/*
 * Device Types in INQUIRY
 */
#define SS_RMB_MASK					0X80
	
/*
 * SASD Space Units
 */
#define SS_SPACE_FMS				0X01
#define SS_SPACE_EOD				0X03
	
/*
 * Start/Stop Cmd
 */
#define SS_START					0X01
#define SS_STOP						0X00

/*
 * Read Defect List and Format
 */
#define SS_PLIST					0X10
#define SS_GLIST					0X08
#define SS_BLK_FMT					0X00
#define SS_BFIP_FMT					0X04
#define SS_PSEC_FMT					0X05

#define SS_FMT_DATA					0X10
#define SS_CMP_LST					0X08
#define SS_FOV						0X80
#define SS_DPRY						0X40
#define SS_DCRT						0X20
#define SS_STPF						0X10

/*
 * Extended Sense Keys
 */
#define SS_EXTEOM					0X40
#define SS_EXTFM					0X80

#endif	/* _SYS_SPCSCSI_H */
