/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)master:llcloop/space.c	1.3"

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/llcloop.h>

#define LOOPCNT	8

struct loop_pcb	loop_pcb[LOOPCNT];
int		loopcnt = LOOPCNT;
struct ifstats loopstats[LOOPCNT];
