/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)hiint:uts/i386/master.d/pic/space.c	1.1"

#include "sys/ipl.h"
#include "sys/pic.h"
#include "config.h"

/* Configurable data for Programmable Interupt Controllers */
#ifdef AT386            /* AT386 board */
unsigned char picbuffered = 0;			/* PICs in unbuffered mode */
#else
unsigned char picbuffered = 1;			/* PICs in buffered mode */
#endif /* AT386 */
unsigned char i82380 = 0;				/* 1 iff i82380 chip used */

/* Initialized data for Programmable Interupt Controllers */
int npic = PIC_UNITS;					/* number of pics configured */
unsigned short cmdport[PIC_UNITS] = {	/* command port addrs for pics */
	PIC_0_SIOA,							/* Use SIOA field of sdev */
	PIC_1_SIOA,
#ifdef PIC_2_SIOA
	PIC_2_SIOA,
#endif
};
unsigned short imrport[PIC_UNITS] = {	/* intr mask port addrs for pics */
	PIC_0_EIOA,							/* Use EIOA field of sdev */
	PIC_1_EIOA,
#ifdef PIC_2_EIOA
	PIC_2_EIOA,
#endif
};
unsigned char masterline[PIC_UNITS] = {	/* line on master connected to */
	0,									/* Use VECT field of sdev */
	PIC_1_VECT,
#ifdef PIC_2_VECT
	PIC_2_VECT,
#endif
};

/* Uninitialized data for Programmable Interupt Controllers */
unsigned char curmask[PIC_UNITS];		/* current pic masks */
unsigned char iplmask[IPLHI*PIC_UNITS];	/* pic masks for priority levels */
