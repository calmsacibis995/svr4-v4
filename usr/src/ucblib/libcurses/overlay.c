/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ucblibcurses:overlay.c	1.1.1.1"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)overlay.c 1.9 89/07/13 SMI"; /* from UCB 5.2 86/02/12 */
#endif not lint

# include	"curses.ext"
# include	<ctype.h>

# define	min(a,b)	(a < b ? a : b)
# define	max(a,b)	(a > b ? a : b)

/*
 *	This routine writes win1 on win2 non-destructively.
 *
 */
overlay(win1, win2)
reg WINDOW	*win1, *win2; {

	reg char	*sp, *end;
	reg int		x, y, endy, endx, starty, startx;
	reg int 	y1,y2;

# ifdef DEBUG
	fprintf(outf, "OVERLAY(%0.2o, %0.2o);\n", win1, win2);
# endif
	starty = max(win1->_begy, win2->_begy);
	startx = max(win1->_begx, win2->_begx);
	endy = min(win1->_maxy + win1->_begy, win2->_maxy + win2->_begy);
	endx = min(win1->_maxx + win1->_begx, win2->_maxx + win2->_begx);
# ifdef DEBUG
	fprintf(outf, "OVERLAY:from (%d,%d) to (%d,%d)\n", starty, startx, endy, endx);
# endif
	if (starty >= endy || startx >= endx)
		return;
	y1 = starty - win1->_begy;
	y2 = starty - win2->_begy;
	for (y = starty; y < endy; y++, y1++, y2++) {
		end = &win1->_y[y1][endx - win1->_begx];
		x = startx - win2->_begx;
		for (sp = &win1->_y[y1][startx - win1->_begx]; sp < end; sp++) {
			if (!isspace(*sp))
				mvwaddch(win2, y2, x, *sp);
			x++;
		}
	}
}
