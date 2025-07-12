/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1986, 1987, 1989  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)mbus:uts/i386/master.d/ics/space.c	1.3.1.3"

#include "sys/types.h"
#include "sys/param.h"
#include "sys/ics.h"
#include "config.h"

/* slot map of boards in the system */
#define ICS_MAX_SLOT 21
ics_slot_t ics_slotmap[ICS_MAX_SLOT];


/* list of supported cpu boards */

char *ics_cpu_cfglist[] = {
	"386/100", 
	"386/116", 
	"386/120", 
	"386/133", 
	"486/125DU", 
	"486/125", 
	"486/133SE", 
	 0
};

/*
 *	IO address for ics got from sdev
 */
unsigned long ics_low_addr= ICS_0_SIOA;
unsigned long ics_hi_addr= ICS_0_SIOA + 0x4;
unsigned long ics_data_addr= ICS_0_SIOA + 0xc;

/* 
 * For init 6 functionality specify either single agent or bus wide reset. 
 * Legal values for ics_reset_type are ICS_LOCAL_RESET or ICS_WARM_RESET
 */
int	ics_reset_type	= ICS_LOCAL_RESET;

#define MAX_CPU 4
long ics_max_numcpu = MAX_CPU;

/*
 * these fields are static well known minor devices for root swap pipe 
 * and dump devices. If the user wishes to change the device nos for them
 * they have to do it here.
 */
extern minor_t rootdev_minor = 1;
extern minor_t swapdev_minor = 2;
extern minor_t pipedev_minor = 1;
extern minor_t dumpdev_minor = 2;

/* these values should be specified in /stand/config */

struct ics_bdev ics_bdev[MAX_CPU] = {
/*	major	base	*/	
	 0,	0,
	 0,	16,
	 0,	32,
	 0,	48,
};
