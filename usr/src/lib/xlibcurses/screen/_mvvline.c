/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/_mvvline.c	1.1"

#define		NOMACROS
#include	"curses_inc.h"

mvvline(y, x, c, n)
int	y, x, n;
chtype	c;
{
    return (mvwvline(stdscr, y, x, c, n));
}
