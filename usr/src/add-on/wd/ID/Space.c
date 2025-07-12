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
 *		Copyright (c) 1987, 1988 by Western Digital Corporation.
 *		All rights reserved.  Contains confidential information and
 *		trade secrets proprietary to
 *			Western Digital Corporation
 *			2445 McCabe Way
 *			Irvine, California 92714
 */

#ident	"@(#)wd:ID/Space.c	1.1.3.1"

/*
 * Configuration file for WD8003S (Starlan) / WD8003E (Ethernet).
 * All user configurable options are here (automatically set on PS-2).
 */

#include "sys/types.h"
#include "sys/stream.h"
#include "sys/wd.h"
#include "sys/socket.h"
#include "net/if.h"

#define WDDEBUG		0		/* trace transmit attempts */

#include "config.h"

#define MAXMULTI	16		/* Number of multicast addrs/board */

struct wddev  wddevs[ (WD_UNITS / WD_CNTLS) * WD_CNTLS ];
struct wdstat wdstats[ WD_CNTLS ];
int    wd_boardcnt = WD_CNTLS;

struct wdmaddr wdmultiaddrs[ WD_CNTLS * MAXMULTI ];/* multicast addr storage */
int    wd_multisize = MAXMULTI;		/* # of multicast addrs/board */

int    wd_debug = WDDEBUG;		/* can be enabled dynamically */
int    wd_inetstats = 1;		/* keep inet interface stats */

 /*
  * "wd_ifname" is used to name the internet statistics structure.  It should
  *  match the interface prefix specified in the strcf(4) file and ifconfig(1M)
  *  command used in rc.inet.
  */
char	*wd_ifname = "emd";	/* unit number of interface will match board */
				/* number (i.e emd0, emd1, emd2) */

struct wdparam wdparams[ WD_CNTLS ] = {
#ifdef	WD_0
    {
	0,				/* board index */
	WD_0_VECT,			/* interrupt level */
	WD_0_SIOA,			/* I/O port for device */
	(caddr_t)WD_0_SCMA,		/* address of board's memory */
	(WD_0_ECMA-WD_0_SCMA)+1,	/* memory size */
	0,				/* pointer to mapped memory */
	0,				/* board type */
	0,				/* board present flag */
	0,				/* board status */
	0,				/* number of streams open */
	WD_CMAJOR_0,			/* major device number */
	(WD_UNITS / WD_CNTLS),		/* number of minor devices allowed */
    },
#endif
#ifdef	WD_1
    {
	0,				/* board index */
	WD_1_VECT,			/* interrupt level */
	WD_1_SIOA,			/* I/O port for device */
	(caddr_t)WD_1_SCMA,		/* address of board's memory */
	(WD_1_ECMA-WD_1_SCMA)+1,	/* memory size */
	0,				/* pointer to mapped memory */
	0,				/* board type */
	0,				/* board present flag */
	0,				/* board status */
	0,				/* number of streams open */
	WD_CMAJOR_1,			/* major device number */
	(WD_UNITS / WD_CNTLS),		/* number of minor devices allowed */
    },
#endif
#ifdef	WD_2
    {
	0,				/* board index */
	WD_2_VECT,			/* interrupt level */
	WD_2_SIOA,			/* I/O port for device */
	(caddr_t)WD_2_SCMA,		/* address of board's memory */
	(WD_2_ECMA-WD_2_SCMA)+1,	/* memory size */
	0,				/* pointer to mapped memory */
	0,				/* board type */
	0,				/* board present flag */
	0,				/* board status */
	0,				/* number of streams open */
	WD_CMAJOR_2,			/* major device number */
	(WD_UNITS / WD_CNTLS),		/* number of minor devices allowed */
    },
#endif
#ifdef	WD_3
    {
	0,				/* board index */
	WD_3_VECT,			/* interrupt level */
	WD_3_SIOA,			/* I/O port for device */
	(caddr_t)WD_3_SCMA,		/* address of board's memory */
	(WD_3_ECMA-WD_3_SCMA)+1,	/* memory size */
	0,				/* pointer to mapped memory */
	0,				/* board type */
	0,				/* board present flag */
	0,				/* board status */
	0,				/* number of streams open */
	WD_CMAJOR_3,			/* major device number */
	(WD_UNITS / WD_CNTLS),		/* number of minor devices allowed */
    },
#endif
};
