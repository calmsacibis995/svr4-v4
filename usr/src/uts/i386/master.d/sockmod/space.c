/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)master:sockmod/space.c	1.3"
#include "config.h"

#include <sys/param.h>
#include <sys/stream.h>
#include <sys/tiuser.h>
#include <sys/sockmod.h>


struct so_so so_so[SOCK_UNITS];
int so_cnt = SOCK_UNITS;
