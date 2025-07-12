/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Module: WD8003
 * Project: System V ViaNet
 *
 *		Copyright (c) 1987 by Western Digital Corporation.
 *		All rights reserved.  Contains confidential information and
 *		trade secrets proprietary to
 *			Western Digital Corporation
 *			2445 McCabe Way
 *			Irvine, California 92714
 */

#ident	"@(#)wd:sys/wdhdw.h	1.1"

/* Hardware definitions for the WD8003 and 8390 LAN controller		*/

/* WD8003 Definitions and Statistics					*/

#define TX_BUF_LEN	(6*256)
#define	SEG_LEN		(1024 * 8)

/* WD8003 Commands							*/

#define SFTRST	0x80			/* software reset command	*/
#define MEMENA	0x40			/* memory enable switch		*/
 
/* WD8003 register locations						*/

#define	UBA_STA		8
#define	WD_BYTE		0xE

/* 8390 Registers: Page 1						*/
/* NOTE: All addresses are offsets from the command register (cmd_reg)	*/

#define	PSTART	0x1
#define	PSTOP	0x2
#define	BNRY	0x3
#define	TPSR	0x4
#define	TBCR0	0x5
#define	TBCR1	0x6
#define	ISR	0x7
#define RBCR0	0xA
#define RBCR1	0xB
#define	RCR	0xC
#define	TCR	0xD
#define	DCR	0xE
#define	IMR	0xF
#define	RSR	0xC
#define	CNTR0	0xD
#define CNTR1	0xE
#define	CNTR2	0xF

/* 8390 Registers: Page 2						*/
/* NOTE: All addresses are offsets from the command register (cmd_reg)	*/

#define	PAR0	0x1
#define	CURR	0x7
#define MAR0	0x8

/* 8390 Commands							*/

#define	PAGE_0	0x00
#define	PAGE_1	0x40
#define	PAGE_2	0x80
#define	PAGE_3	0xC0

#define	PG_MSK	0x3F			/* Used to zero the page select
					   bits in the command register */

#define	STA	0x2			/* Start 8390			*/
#define STP	0x1			/* Stop 8390			*/
#define	TXP	0x4			/* Transmit Packet		*/
#define	ABR	0x20			/* Value for Remote DMA CMD	*/

/* 8390 ISR conditions							*/

#define	PRX	0x1
#define	PTX	0x2
#define	RXE	0x4
#define	TXE	0x8
#define	OVW	0x10
#define	CNT	0x20

/* 8390 IMR bit definitions						*/

#define	PRXE	0x1
#define	PTXE	0x2
#define	RXEE	0x4
#define	TXEE	0x8
#define	OVWE	0x10
#define	CNTE	0x20
#define	RDCE	0x40

/* 8390 DCR bit definitions						*/

#define	WTS	0x1
#define	BOS	0x2
#define	LAS	0x4
#define	BMS	0x8
#define	FT0	0x20
#define	FT1	0x40


/* 8390 TCR bit definitions						*/

#define	CRC	0x1
#define	LB0_1	0x2
#define	ATD	0x8
#define	OFST	0x10


/* RCR bit definitions							*/

#define	SEP	0x1
#define	AR	0x2
#define	AB	0x4
#define	AM	0x8
#define	PRO	0x10
#define	MON	0x20

/* TSR bit definitions							*/
#define TSR_COL 0x4
#define TSR_ABT 0x8

/* 8390 Register initialization values					*/

#define	INIT_IMR	PRXE + PTXE + RXEE + TXEE
#define INIT_DCR	BMS + FT1
#define	INIT_TCR	0
#define	INIT_RCR	AB + AM
#define	RCRMON		MON

/* Misc. Commands & Values						*/

#define	CLR_INT		0xFF		/* Used to clear the ISR */
#define	NO_INT		0		/* no interrupts conditions */
#define ADDR_LEN	6
#define NETPRI		PZERO+3

/* PS2 specific defines */
/* Defines for PS/2 Microchannel POS ports */

#define SYS_ENAB	0x94		/* System board enable / setup */
#define ADAP_ENAB	0x96		/* Adaptor board enable / setup */
#define POS_0		0x100		/* POS reg 0 - adaptor ID lsb */
#define POS_1		0x101		/* POS reg 1 - adaptor ID msb */
#define POS_2		0x102		/* Option Select Data byte 1 */
#define POS_3		0x103		/* Option Select Data byte 2 */
#define POS_4		0x104		/* Option Select Data byte 3 */
#define POS_5		0x105		/* Option Select Data byte 4 */
#define POS_6		0x106		/* Subaddress extension lsb */
#define POS_7		0x107		/* Subaddress extension msb */

/* Defines for Adaptor Board ID's for Microchannel */

#define WD_ID	0xABCD			/* generic id for WD test */
#define ETI_ID	0x6FC0			/* 8003et/a ID */
#define STA_ID	0x6FC1			/* 8003st/a ID */
#define WA_ID	0x6FC2			/* 8003w/a ID */

#define NUM_ID	4

#define PS2_RAMSZ	16384


