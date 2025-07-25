/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/putchar.c	1.8"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 * A subroutine version of the macro putchar
 */
#include "synonyms.h"
#include "shlib.h"
#include <stdio.h>
#undef putchar

int
putchar(ch)
	int ch;
{
	register FILE *iop = stdout;

	return putc(ch, iop);
}
