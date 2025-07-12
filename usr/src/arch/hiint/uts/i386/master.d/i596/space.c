/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)hiint:uts/i386/master.d/i596/space.c	1.1"

/* i596 space.c */

#include "sys/i596.h"
#include "config.h"

/*
 *	INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied under the terms of a license
 *	agreement or nondisclosure agreement with Intel Corpo-
 *	ration and may not be copied or disclosed except in
 *	accordance with the terms of that agreement.
 *	Copyright 1987, 1988, 1990 Intel Corporation
 */

#define	I596_N_TBD		25
#define	I596_N_FD		50
#define	I596_N_RBD		50
#define	I596_RCVBUFSIZ		256
#define	I596_N_LSAP		32

unsigned char pc586default_add[6] = { 0x00, 0xAA, 0x00, 0x00, 0x0B, 0xAD };
/*
 * INETSTATS must be defined as 1 to get interface statistics using
 * the netstat(1) command.
 */
#define	INETSTATS	1

/*
 *  Ethernet interfaces are given logical names in the strcf(4) file and
 *  then used by the ifconfig(1M) and netstat(1) commands.  "pc586_ifname"
 *  is used to inform the driver what the interface has been named.  The
 *  driver only needs to know this if INETSTATS is set to 1 to get network
 *  statistics displayed with the netstat(1) command.
 *
 *  By default the PC586 interface name is "emd" with the board number added
 *  as a suffix (i.e emd0, emd1, emd2).  It may be necessary to change the
 *  name if it conflicts with another Ethernet driver.
 */
char	*pc586_ifname = "emd";	/* unit number of interface will match board */

/*
 * The following are all data declarations for this driver.
 * DO NOT TOUCH THEM.
 */
int	pc586inetstats = INETSTATS;
struct586_t pc586struct[1];

int i596_interrupt_level		= PC586_0_VECT;
int i596_major_dev			= PC586_CMAJOR_0;

unsigned short	i596_n_tbd		= I596_N_TBD;
unsigned short	i596_n_fd		= I596_N_FD;
unsigned short	i596_n_rbd		= I596_N_RBD;
unsigned short	i596_rcvbufsiz		= I596_RCVBUFSIZ;
unsigned short	i596_n_lsap		= I596_N_LSAP;

unsigned short	i596_chan_attn_addr	= PC586_0_SIOA + 0x02;
unsigned short	i596_port_addr		= PC586_0_SIOA + 0x04;
unsigned short	i596_reset_addr		= PC586_0_SIOA + 0x0F;
unsigned short	i596_station_addr	= PC586_0_SIOA + 0x10;
