/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/copywin.c	1.9"

/*
 * This routine writes parts of Srcwin onto Dstwin,
 * either non-destructively (over_lay = TRUE) or destructively (over_lay = FALSE).
 */

#include	"curses_inc.h"

copywin(Srcwin, Dstwin,
	minRowSrc, minColSrc, minRowDst, minColDst, maxRowDst, maxColDst,
	over_lay)
register	WINDOW	*Srcwin, *Dstwin;
int			minRowSrc, minColSrc, minRowDst,
			minColDst, maxRowDst, maxColDst;
int			over_lay;
{
    register	int	numcopied;
    register	chtype	*spSrc, *spDst;
    register	int	ySrc, yDst, width_bytes;
    register	int	height = (maxRowDst - minRowDst) + 1;
    chtype		**_yDst = Dstwin->_y, **_ySrc = Srcwin->_y,
			bkSrc = Srcwin->_bkgd,
			atDst = Dstwin->_attrs;
    int			width = (maxColDst - minColDst) + 1, which_copy;

#ifdef	DEBUG
    if (outf)
	fprintf (outf, "copywin(%0.2o, %0.2o);\n", Srcwin, Dstwin);
#endif	/* DEBUG */

    /*
     * If we are going to be copying from curscr,
     * first offset into curscr the offset the Dstwin knows about.
     */
    if (Srcwin == curscr)
	minRowSrc += Dstwin->_yoffset;

    /*
     * There are three types of copy.
     * 0 - Straight memcpy allowed
     * 1 - We have to first check to see if the source character is a blank
     * 2 - Dstwin has attributes or bkgd that must changed
     *     on a char-by-char basis.
     */
    if (!(which_copy = (over_lay) ? 1 : (2 * ((Dstwin->_attrs != A_NORMAL) || (Dstwin->_bkgd != _BLNKCHAR)))))
	width_bytes = width * sizeof (chtype);

    /* for each Row */
    for (ySrc = minRowSrc, yDst = minRowDst; height-- > 0; ySrc++, yDst++)
    {
	if (which_copy)
	{
	    spSrc = &_ySrc[ySrc][minColSrc];
	    spDst = &_yDst[yDst][minColDst];
	    numcopied = width;

	    if (which_copy == 1)		/* overlay */
	    {
		for (; numcopied-- > 0; spSrc++, spDst++)
		    /* Check to see if the char is a "blank/bkgd". */
		    if (*spSrc != bkSrc)
			*spDst = *spSrc | atDst;
	    }
	    else
	    {
		for (; numcopied-- > 0; spSrc++, spDst++)
		    *spDst = *spSrc | atDst;
	    }
	}
	else
	{
	    /* ... copy all chtypes */
	    (void) memcpy ((char *) &_yDst[yDst][minColDst],
		(char *) &_ySrc[ySrc][minColSrc], width_bytes);
	}

	/* note that the line has changed */
	if (minColDst < Dstwin->_firstch[yDst])
	    Dstwin->_firstch[yDst] = minColDst;
	if (maxColDst > Dstwin->_lastch[yDst])
	    Dstwin->_lastch[yDst] = maxColDst;
    }

#ifdef	_VR3_COMPAT_CODE
    if (_y16update)
    {
	(*_y16update)(Dstwin, (maxRowDst - minRowDst) + 1,
	    (maxColDst - minColDst) + 1, minRowDst, minColDst);
    }
#endif	/* _VR3_COMPAT_CODE */

    /* note that something in Dstwin has changed */
    Dstwin->_flags |= _WINCHANGED;

    if (Dstwin->_sync)
	wsyncup(Dstwin);

    return (Dstwin->_immed ? wrefresh(Dstwin) : OK);
}
