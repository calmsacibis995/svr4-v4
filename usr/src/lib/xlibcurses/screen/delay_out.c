/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/delay_out.c	1.6"

/*
 * Code for various kinds of delays.  Most of this is nonportable and
 * requires various enhancements to the operating system, so it won't
 * work on all systems.  It is included in curses to provide a portable
 * interface, and so curses itself can use it for function keys.
 */

#include	"curses_inc.h"

/*
 * Delay the output for ms milliseconds.
 * Note that this is NOT the same as a high resolution sleep.  It will
 * cause a delay in the output but will not necessarily suspend the
 * processor.  For applications needing to sleep for 1/10th second,
 * this is not a usable substitute.  It causes a pause in the displayed
 * output, for example, for the eye wink in snake.  It is disrecommended
 * for "delay" to be much more than 1/2 second, especially at high
 * baud rates, because of all the characters it will output.  Note
 * that due to system delays, the actual pause could be even more.
 * You can't get a decent pac-man game with this routine.
 */
delay_output(ms)
int	ms;
{
    extern	int	_outchar();

    return (_delay(ms * 10, _outchar));
}
