/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pc586:ID/Space.c	1.3.2.2"

/* imx586 space.c */

#include "sys/imx586.h"
#include "config.h"

/*
 *	INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied under the terms of a license
 *	agreement or nondisclosure agreement with Intel Corpo-
 *	ration and may not be copied or disclosed except in
 *	accordance with the terms of that agreement.
 *	Copyright 1987, 1988 Intel Corporation
 */


/*  First, decide how many boards you wish to install in one PC AT.
    (typically only one). Then set N_BOARDS equal to this value.
 
    Second, define all 4 interrupt levels and all 4*2 imx586 address ranges.
    Any unused addresses and levels MUST be set to -1. Refer to
    IMX586 User's Manual for board jumpering.

    NOTE: when static ram address is jumpered above one megabyte,
    set board_XXX_static_ram to the jumpered address plus 0x20000
    For example, if static ram is jumpered to 0xf00000 then
    board_XXX_static_ram is set to 0xf20000. (For those too lazy to
    read.)  If static ram is jumpered
    to 0xc8000 then set board_XXX_static_ram to 0xc8000.
*/

#ifdef IMX586_0
#define N_BOARDS		1
int imx586_0_cmd_prom		= IMX586_0_SCMA;
#if (IMX586_0_SCMA >= 0xF00000)
int imx586_0_static_ram		= IMX586_0_SCMA + 0x20000;
#else
int imx586_0_static_ram		= IMX586_0_SCMA + 0;
#endif
int imx586_0_interrupt_level	= IMX586_0_VECT;
int imx586_0_major_dev		= IMX586_CMAJOR_0;
#else
int imx586_0_cmd_prom		= {-1};
int imx586_0_static_ram		= {-1};
int imx586_0_interrupt_level	= {-1};
int imx586_0_major_dev		= {-1};
#endif

#ifdef IMX586_1
#undef N_BOARDS
#define N_BOARDS		2
int imx586_1_cmd_prom		= IMX586_1_SCMA;
#if (IMX586_1_SCMA >= 0xF00000)
int imx586_1_static_ram		= IMX586_1_SCMA + 0x20000;
#else
int imx586_1_static_ram		= IMX586_1_SCMA + 0;
#endif
int imx586_1_interrupt_level	= IMX586_1_VECT;
int imx586_1_major_dev		= IMX586_CMAJOR_1;
#else
int imx586_1_cmd_prom		= {-1};
int imx586_1_static_ram		= {-1};
int imx586_1_interrupt_level	= {-1};
int imx586_1_major_dev		= {-1};
#endif

#ifdef IMX586_2
#undef N_BOARDS
#define N_BOARDS		3
int imx586_2_cmd_prom		= IMX586_2_SCMA;
#if (IMX586_2_SCMA >= 0xF00000)
int imx586_2_static_ram		= IMX586_2_SCMA + 0x20000;
#else
int imx586_2_static_ram		= IMX586_2_SCMA + 0;
#endif
int imx586_2_interrupt_level	= IMX586_2_VECT;
int imx586_2_major_dev		= IMX586_CMAJOR_2;
#else
int imx586_2_cmd_prom		= {-1};
int imx586_2_static_ram		= {-1};
int imx586_2_interrupt_level	= {-1};
int imx586_2_major_dev		= {-1};
#endif

#ifdef IMX586_3
#undef N_BOARDS
#define N_BOARDS		4
int imx586_3_cmd_prom		= IMX586_3_SCMA;
#if (IMX586_3_SCMA >= 0xF00000)
int imx586_3_static_ram		= IMX586_3_SCMA + 0x20000;
#else
int imx586_3_static_ram		= IMX586_3_SCMA + 0;
#endif
int imx586_3_interrupt_level	= IMX586_3_VECT;
int imx586_3_major_dev		= IMX586_CMAJOR_3;
#else
int imx586_3_cmd_prom		= {-1};
int imx586_3_static_ram		= {-1};
int imx586_3_interrupt_level	= {-1};
int imx586_3_major_dev		= {-1};
#endif

/* The following is a work around for pre-production imx586 boards that
   contain invalid ethernet address proms. This patch will only work in
   PC AT's that have at most one imx586 board installed. Alter the value
   of default_add[] to obtain the desired ethernet address
*/

unsigned char imx586default_add[6]	= { 0x38, 0x00, 0x25, 0x00, 0x08, 0x00 };

/*
 * INETSTATS must be defined as 1 to get interface statistics using
 * the netstat(1) command.
 */
#define	INETSTATS	1

/*
 *  Ethernet interfaces are given logical names in the strcf(4) file and
 *  then used by the ifconfig(1M) and netstat(1) commands.  "imx586_ifname"
 *  is used to inform the driver what the interface has been named.  The
 *  driver only needs to know this if INETSTATS is set to 1 to get network
 *  statistics displayed with the netstat(1) command.
 *
 *  By default the IMX586 interface name is "emd" with the board number added
 *  as a suffix (i.e emd0, emd1, emd2).  It may be necessary to change the
 *  name if it conflicts with another Ethernet driver.
 */
char	*imx586_ifname = "emd";	/* unit number of interface will match board */
								/* number (i.e emd0, emd1, emd2) */

/*
 * The following are all data declarations for this driver.
 * DO NOT TOUCH THEM.
 */
int	imx586_boards   = {N_BOARDS};
int	imx586inetstats = INETSTATS;
struct586_t imx586struct[N_BOARDS];
