/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/relo.h	1.1"
/*
* common/relo.h - common assembler interface to relocation
*
* Depends on:
*	"common/as.h"
*/

#ifdef __STDC__
void	relocexpr(Value *, const Code *, Section *);
#else
void	relocexpr();
#endif
