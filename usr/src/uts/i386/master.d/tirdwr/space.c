/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)master:tirdwr/space.c	1.3"

#include "sys/types.h"
#include "sys/stream.h"
#include "sys/tirdwr.h"

#include "config.h"	/* to collect tunable parameter below */

struct	trw_trw	trw_trw[NUMTRW] ;
int		trw_cnt = { NUMTRW } ;
