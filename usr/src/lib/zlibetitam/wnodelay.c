/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stitam:wnodelay.c	1.1"
#include "cvttam.h"

int
TAMwnodelay (wn, flag)
short wn;
int flag;
{
  TAMWIN *tw;

  if (tw = _validwindow (wn)) {
    if (flag) {
      State(tw) |= NODELAY;
    }
    else {
      State(tw) &= ~NODELAY;
    }
    nodelay(Scroll(tw),flag);
    return (OK);
  }
  return (ERR);
}
