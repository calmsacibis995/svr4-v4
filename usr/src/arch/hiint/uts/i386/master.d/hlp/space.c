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

#ident	"@(#)hiint:uts/i386/master.d/hlp/space.c	1.1"
static char	prog_copyright[] = "Copyright 1990 Intel Corp. xxxxxx";
static char	prog_version[] = "@@(#) $File: space.c $ $Version: 1.1 $ $Date: 90/11/20 15:11:54 $ $State: unit-tested $";

/*
 * space.c
 *	Line-printer specific configuration.
*/
#include <sys/types.h>
#include <sys/cred.h>
#include <sys/ddi.h>
#include <sys/hlp.h>
#include "config.h"

/*
 * This table gives the interrupt level and  I/O addresses
 * for the line-printer. 
 */

struct	lp_cfg lp_hiint_cfg = {
/*  Level, 	 	port A, 	port B, 	  port C, 		Control */
	HLP_0_VECT, HLP_0_SIOA, HLP_0_SIOA+2, HLP_0_SIOA+4, HLP_0_SIOA+6
};
