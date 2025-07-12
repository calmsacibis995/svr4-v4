/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/gram.h	1.1"
/*
* common/gram.h - common assembler parsing header
*
* Depends on:
*	<stdio.h>
*/

#ifdef __STDC__
void	initchtab(void);	/* initialize tokenization code */
void	setinput(FILE *);	/* setup stream for parsing */
int	yyparse(void);		/* parse current input stream */
#else
void	initchtab();
void	setinput();
int	yyparse();
#endif
