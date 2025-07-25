/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/fgo.c	1.2"

#include <stdio.h>
#include <curses.h>
#include "token.h"
#include "winp.h"

fgo(row, col)
int row;
int col;
{
	Cfld->currow = row;
	Cfld->curcol = col;
	wgo(row + Cfld->frow, col + Cfld->fcol);
}
