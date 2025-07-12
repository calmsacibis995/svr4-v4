/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sp:ID/Space.c	1.1"

#include "sys/types.h"
#include "sys/stream.h"

#include "config.h"	/* to collect tunable parameter */

/* WARNING: Original code had this defined in sp.c. It is still defined there
 * we define it here so we can declare sp_sp.
 */
struct sp {
	queue_t *sp_rdq;		/* this stream's read queue */
	queue_t *sp_ordq;		/* other stream's read queue */
};

struct	sp	sp_sp[SP_UNITS] ;
int		spcnt ={SP_UNITS} ;
