/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)xcplxcurses:cr_put.c	1.1"

/*
 *	@(#) cr_put.c 1.1 90/03/30 lxcurses:cr_put.c
 */
# include	"ext.h"

# define	HARDTABS	8

extern char	*tgoto();
int		plodput();

/*
 * Terminal driving and line formatting routines.
 * Basic motion optimizations are done here as well
 * as formatting of lines (printing of control characters,
 * line numbering and the like).
 *
 * 3/27/83 (Berkeley) @(#)cr_put.c	1.4
 */

/*
 * Sync the position of the output cursor.
 * Most work here is rounding for terminal boundaries getting the
 * column position implied by wraparound or the lack thereof and
 * rolling up the screen to get destline on the screen.
 */

static int	outcol, outline, destcol, destline;

WINDOW		*_win;

mvcur(ly, lx, y, x)
int	ly, lx, y, x; {

#ifdef DEBUG
	fprintf(outf, "MVCUR: moving cursor from (%d,%d) to (%d,%d)\n", ly, lx, y, x);
#endif
	destcol = x;
	destline = y;
	outcol = lx;
	outline = ly;
	fgoto();
}

char
_putchar(c)
reg char	c; {

	putchar(c);
#ifdef DEBUG
	fprintf(outf, "_PUTCHAR(%s)\n", unctrl(c));
#endif
}

fgoto()
{
	reg char	*cgp;
	reg int		l, c;

	if (destcol > COLS - 1) {
		destline += destcol / COLS;
		destcol %= COLS;
	}
	if (outcol > COLS - 1) {
		l = (outcol + 1) / COLS;
		outline += l;
		outcol %= COLS;
		if (AM == 0) {
			while (l > 0) {
				if (_pfast)
					if (CR)
						tputs(CR, 0, _putchar);
					else
						_putchar('\r');
				if (NL)
					tputs(NL, 0, _putchar);
				else
					_putchar('\n');
				l--;
			}
			outcol = 0;
		}
		if (outline > LINES - 1) {
			destline -= outline - (LINES - 1);
			outline = LINES - 1;
		}
	}
	if (destline > LINES - 1) {
		l = destline;
		destline = LINES - 1;
		if (outline < LINES - 1) {
			c = destcol;
			if (_pfast == 0 && !CA)
				destcol = 0;
			fgoto();
			destcol = c;
		}
		while (l > LINES - 1) {
			/*
			 * The following linefeed (or simulation thereof)
			 * is supposed to scroll up the screen, since we
			 * are on the bottom line.  We make the assumption
			 * that linefeed will scroll.  If ns is in the
			 * capability list this won't work.  We should
			 * probably have an sc capability but sf will
			 * generally take the place if it works.
			 *
			 * Superbee glitch:  in the middle of the screen we
			 * have to use esc B (down) because linefeed screws up
			 * in "Efficient Paging" (what a joke) mode (which is
			 * essential in some SB's because CRLF mode puts garbage
			 * in at end of memory), but you must use linefeed to
			 * scroll since down arrow won't go past memory end.
			 * I turned this off after recieving Paul Eggert's
			 * Superbee description which wins better.
			 */
			if (NL /* && !XB */ && _pfast)
				tputs(NL, 0, _putchar);
			else
				_putchar('\n');
			l--;
			if (_pfast == 0)
				outcol = 0;
		}
	}
	if (destline < outline && !(CA || UP))
		destline = outline;
	if (CA)
	{
		cgp = tgoto(CM, destcol, destline);
		if (plod(strlen(cgp)) > 0)
			plod(0);
		else
			tputs(cgp, 0, _putchar);
	}
	else
		plod(0);
	outline = destline;
	outcol = destcol;
}

/*
 * Move (slowly) to destination.
 * Hard thing here is using home cursor on really deficient terminals.
 * Otherwise just use cursor motions, hacking use of tabs and overtabbing
 * and backspace.
 */

static int plodcnt, plodflg;

plodput(c)
{
	if (plodflg)
		plodcnt--;
	else
		_putchar(c);
}

plod(cnt)
{
	register int i, j, k;
	register int soutcol, soutline;

	plodcnt = plodflg = cnt;
	soutcol = outcol;
	soutline = outline;
	/*
	 * Consider homing and moving down/right from there, vs moving
	 * directly with local motions to the right spot.
	 */
	if (HO) {
		/*
		 * i is the cost to home and tab/space to the right to
		 * get to the proper column.  This assumes ND space costs
		 * 1 char.  So i+destcol is cost of motion with home.
		 */
		if (GT)
			i = (destcol / HARDTABS) + (destcol % HARDTABS);
		else
			i = destcol;
		/*
		 * j is cost to move locally without homing
		 */
		if (destcol >= outcol) {	/* if motion is to the right */
			j = destcol / HARDTABS - outcol / HARDTABS;
			if (GT && j)
				j += destcol % HARDTABS;
			else
				j = destcol - outcol;
		}
		else
			/* leftward motion only works if we can backspace. */
			if (outcol - destcol <= i && (BS || BC))
				i = j = outcol - destcol; /* cheaper to backspace */
			else
				j = i + 1; /* impossibly expensive */

		/* k is the absolute value of vertical distance */
		k = outline - destline;
		if (k < 0)
			k = -k;
		j += k;

		/*
		 * Decision.  We may not have a choice if no UP.
		 */
		if (i + destline < j || (!UP && destline < outline)) {
			/*
			 * Cheaper to home.  Do it now and pretend it's a
			 * regular local motion.
			 */
			tputs(HO, 0, plodput);
			outcol = outline = 0;
		}
		else if (LL) {
			/*
			 * Quickly consider homing down and moving from there.
			 * Assume cost of LL is 2.
			 */
			k = (LINES - 1) - destline;
			if (i + k + 2 < j && (k<=0 || UP)) {
				tputs(LL, 0, plodput);
				outcol = 0;
				outline = LINES - 1;
			}
		}
	}
	else
	/*
	 * No home and no up means it's impossible.
	 */
		if (!UP && destline < outline)
			return -1;
	if (GT)
		i = destcol % HARDTABS + destcol / HARDTABS;
	else
		i = destcol;
/*
	if (BT && outcol > destcol && (j = (((outcol+7) & ~7) - destcol - 1) >> 3)) {
		j *= (k = strlen(BT));
		if ((k += (destcol&7)) > 4)
			j += 8 - (destcol&7);
		else
			j += k;
	}
	else
*/
		j = outcol - destcol;
	/*
	 * If we will later need a \n which will turn into a \r\n by
	 * the system or the terminal, then don't bother to try to \r.
	 */
	if ((NONL || !_pfast) && outline < destline)
		goto dontcr;
	/*
	 * If the terminal will do a \r\n and there isn't room for it,
	 * then we can't afford a \r.
	 */
	if (NC && outline >= destline)
		goto dontcr;
	/*
	 * If it will be cheaper, or if we can't back up, then send
	 * a return preliminarily.
	 */
	if (j > i + 1 || outcol > destcol && !BS && !BC) {
		/*
		 * BUG: this doesn't take the (possibly long) length
		 * of CR into account.
		 */
		if (CR)
			tputs(CR, 0, plodput);
		else
			plodput('\r');
		if (NC) {
			if (NL)
				tputs(NL, 0, plodput);
			else
				plodput('\n');
			outline++;
		}
		outcol = 0;
	}
dontcr:
	while (outline < destline) {
		outline++;
		if (NL && _pfast)
			tputs(NL, 0, plodput);
		else
			plodput('\n');
		if (plodcnt < 0)
			goto out;
		if (NONL || _pfast == 0)
			outcol = 0;
	}
	if (BT)
		k = strlen(BT);
	while (outcol > destcol) {
		if (plodcnt < 0)
			goto out;
/*
		if (BT && outcol - destcol > k + 4) {
			tputs(BT, 0, plodput);
			outcol--;
			outcol &= ~7;
			continue;
		}
*/
		outcol--;
		if (BC)
			tputs(BC, 0, plodput);
		else
			plodput('\b');
	}
	while (outline > destline) {
		outline--;
		tputs(UP, 0, plodput);
		if (plodcnt < 0)
			goto out;
	}
	if (GT && destcol - outcol > 1) {
		for (;;) {
			i = tabcol(outcol, HARDTABS);
			if (i > destcol)
				break;
			if (TA)
				tputs(TA, 0, plodput);
			else
				plodput('\t');
			outcol = i;
		}
		if (destcol - outcol > 4 && i < COLS && (BC || BS)) {
			if (TA)
				tputs(TA, 0, plodput);
			else
				plodput('\t');
			outcol = i;
			while (outcol > destcol) {
				outcol--;
				if (BC)
					tputs(BC, 0, plodput);
				else
					plodput('\b');
			}
		}
	}
	while (outcol < destcol) {
		/*
		 * move one char to the right.  We don't use ND space
		 * because it's better to just print the char we are
		 * moving over.
		 */
		if (_win != (WINDOW *)NULL)
			if (plodflg)	/* avoid a complex calculation */
				plodcnt--;
			else {
				i = curscr->_y[outline][outcol];
				if ((i&_STANDOUT) == (curscr->_flags&_STANDOUT))
					putchar(i&0177);
				else
					goto nondes;
			}
		else
nondes:
		     if (ND)
			tputs(ND, 0, plodput);
		else
			plodput(' ');
		outcol++;
		if (plodcnt < 0)
			goto out;
	}
out:
	if (plodflg) {
		outcol = soutcol;
		outline = soutline;
	}
	return(plodcnt);
}

/*
 * Return the column number that results from being in column col and
 * hitting a tab, where tabs are set every ts columns.  Work right for
 * the case where col > COLS, even if ts does not divide COLS.
 */
tabcol(col, ts)
int col, ts;
{
	int offset, result;

	if (col >= COLS) {
		offset = COLS * (col / COLS);
		col -= offset;
	}
	else
		offset = 0;
	return col + ts - (col % ts) + offset;
}
