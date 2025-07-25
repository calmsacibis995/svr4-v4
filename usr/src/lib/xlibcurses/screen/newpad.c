/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/newpad.c	1.4"
#include	"curses_inc.h"

WINDOW	*
newpad(l,nc)
int	l,nc;
{
    WINDOW	*pad;

    pad = newwin(l,nc,0,0);
    if (pad != (WINDOW *) NULL)
	pad->_flags |= _ISPAD;
    return (pad);
}
