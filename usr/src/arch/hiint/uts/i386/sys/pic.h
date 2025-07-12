/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_PIC_H
#define _SYS_PIC_H

#ident	"@(#)hiint:uts/i386/sys/pic.h	1.1"

/* Definitions for 8259 Programmable Interrupt Controller */

#define PIC_NEEDICW4    0x01            /* ICW4 needed */
#define PIC_ICW1BASE    0x10            /* base for ICW1 */
#define PIC_86MODE      0x01            /* MCS 86 mode */
#define PIC_AUTOEOI     0x02            /* do auto eoi's */
#define PIC_SLAVEBUF    0x08            /* put slave in buffered mode */
#define PIC_MASTERBUF   0x0C            /* put master in bnuffered mode */
#define PIC_SPFMODE     0x10            /* special fully nested mode */
#define PIC_READISR     0x0B            /* Read the ISR */
#define PIC_NSEOI       0x20            /* Non-specific EOI command */

#define PIC_VECTBASE    0x40            /* Vectors for external interrupts */
					/* start at 64.                    */
#endif	/* _SYS_PIC_H */
