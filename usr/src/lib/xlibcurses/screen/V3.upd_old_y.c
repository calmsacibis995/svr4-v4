/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/V3.upd_old_y.c	1.2"

#include	"curses_inc.h"
extern	int	_outchar();

#ifdef	_VR3_COMPAT_CODE
void
_update_old_y_area(win, nlines, ncols, start_line, start_col)
WINDOW	*win;
int	nlines, ncols, start_line, start_col;
{
    int	row, col, num_cols;

    for (row = start_line; nlines > 0; nlines--, row++)
	for (num_cols = ncols, col = start_col; num_cols > 0; num_cols--, col++)
	    win->_y16[row][col] = _TO_OCHTYPE(win->_y[row][col]);
}
#endif	/* _VR3_COMPAT_CODE */
