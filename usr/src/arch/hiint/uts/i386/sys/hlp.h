/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied under the terms of a license agreement
 *	or nondisclosure agreement with Intel Corporation and may not be
 *	copied nor disclosed except in accordance with the terms of that
 *	agreement.
 *
 *	Copyright 1990  Intel Corporation
*/

#ident	"@(#)hiint:uts/i386/sys/hlp.h	1.1"

#ifndef	_SYS_HLP_H
#define _SYS_HLP_H

/*
 *	Hardware constants
*/
	/*
	 * Initialize 8255
	 * Control word = 0x82
	 * 
	 *	---------------------------------
	 *	|   |   |   |   |   |   |   |   |
	 *  | 1 | 0 | 0 | 0 | 0 | 0 | 1 | 0 |
	 *	|   |   |   |   |   |   |   |   |
	 *	---------------------------------
	 *    |   |   |   |   |   |   |   |
	 *    |   |   |   |   |   |   |   |--------> Port C (Lower) Output
	 *    |   |   |   |   |   |   |------------> Port B         Input 
	 *    |   |   |   |   |   |----------------> Mode 0
	 *    |   |   |   |   |--------------------> Port C (Upper) Output
	 *    |   |   |   |------------------------> Port A         Output
	 *    |   |---|----------------------------> Mode 0
	 *    |------------------------------------> Mode Flag      Active
	 */
#define LP_TEST_PATTERN		0xaa	/* test pattern read back by probe */
#define LP_CONTROL_WORD		0x82	/* control word for 8255 */
#define LP_INIT_PRINTER		0xfb	/* To reset the printer. */
#define LP_ONSTROBE		0xfe	/* Turn data strobe signal ON */
#define LP_OFFSTROBE		0xff	/* Turn data strobe signal OFF */
#define LP_INT_CLEAR		0x7f	/* Clear Printer interrupt */
#define LP_NO_ERROR			0x40	/* Printer port in Error condition. */	
#define LP_BUSY				0x80	/* Printer port BUSY */
#define LP_ACK				0x10	/* Printer ACK line */
#define ONSTROBE			0x0		/* Turn data strobe signal on */
#define OFFSTROBE			0x1		/* Turn data strobe signal off */
#define LP_MINOR			5
/*
 *	Device Structures
*/
struct lp_cfg {
		int p_level;	/* intr level */
		int p_porta;	/* 8255 port a -> data out */
		int p_portb;	/* 8255 port b -> status in */
		int p_portc;	/* 8255 port c -> strobe and interrupt f/f */
		int control;	/* 8255 control port */
};

/*
 * HW dependent macros
 */
#define	LP_DATAPORT(cfg)	((cfg)->p_porta)
#define	LP_INITIALISE_CHIP(cfg)	outb((cfg)->control, LP_CONTROL_WORD)
#define	LP_INITIALISE_PRINTER(cfg) \
	{ \
	outb((cfg)->p_portc, LP_INIT_PRINTER); \
	drv_usecwait (50); \
	outb((cfg)->p_portc, 0xff); \
	}
#define	LP_INTERRUPT_CLEAR(cfg) \
	{ \
	outb((cfg)->p_portc, LP_INT_CLEAR); \
	drv_usecwait (50); \
	outb((cfg)->p_portc, 0xff); \
	}
#define	LP_OUTPUT_CHAR(cfg, c) 	outb ((cfg)->p_porta, c)
#define	LP_TURNON_STROBE(cfg)	outb ((cfg)->p_portc, LP_ONSTROBE)
#define	LP_TURNOFF_STROBE(cfg)	outb ((cfg)->p_portc, LP_OFFSTROBE)
#define	LP_PRINTER_STATUS(cfg)	inb  ((cfg)->p_portb)
#define	LP_PRINTER_READY(stat) \
		(((stat) & LP_NO_ERROR) && !((stat) & LP_BUSY) && ((stat) & LP_ACK))
#endif	/* _SYS_HLP_H */
