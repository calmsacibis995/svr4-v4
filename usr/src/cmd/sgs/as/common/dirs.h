/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/dirs.h	1.3"
/*
* common/dirs.h - common assembler directives header
*
* Depends on:
*	"common/as.h"
*/

enum	/* common directives */
{
	Dir_Section,	/* .section	ident [int-or-str [int-or-str]]	*/
	Dir_Pushsect,	/* .pushsection	ident				*/
	Dir_Popsect,	/* .popsection					*/
	Dir_Previous,	/* .previous					*/
	Dir_Text,	/* .text					*/
	Dir_Data,	/* .data					*/
	Dir_Bss,	/* .bss		[ident evalexpr [evalexpr]]	*/
	Dir_Comm,	/* .comm	ident intexpr [intexpr]		*/
	Dir_Lcomm,	/* .lcomm	ident intexpr [intexpr]		*/
	Dir_Global,	/* .globl	ident-list			*/
	Dir_Local,	/* .local	ident-list			*/
	Dir_Weak,	/* .weak	ident-list			*/
	Dir_Set,	/* .set		ident operand			*/
	Dir_Size,	/* .size	ident intexpr			*/
	Dir_Type,	/* .type	ident int-or-str		*/
	Dir_Align,	/* .align	evalexpr			*/
	Dir_Zero,	/* .zero	evalexpr			*/
	Dir_Byte,	/* .byte	expr-list			*/
	Dir_2Byte,	/* .2byte	expr-list			*/
	Dir_4Byte,	/* .4byte	expr-list			*/
	Dir_8Byte,	/* .8byte	expr-list			*/
	Dir_Float,	/* .float	floatexpr-list			*/
	Dir_Double,	/* .double	floatexpr-list			*/
	Dir_Ext,	/* .ext		floatexpr-list			*/
	Dir_Ascii,	/* .ascii	string-list			*/
	Dir_String,	/* .string	string-list			*/
	Dir_File,	/* .file	string				*/
	Dir_Ident,	/* .ident	string				*/
	Dir_Version,	/* .version	string				*/
	Dir_TOTAL		/* not a directive */
};

#ifdef __STDC__
void	initdirs(void);
void	aligndata(const char *, Operand *, Ulong, Ulong, int); /* data init */
void	directive(const Uchar *, size_t, Oplist *);	/* above directives */

		/* implementation provides */
void	versioncheck(const Uchar *, size_t);		/* verify version */
int	obsdirect(const Uchar *, size_t, Operand *);	/* handle obsolete */
int	aligncheck(Ulong);				/* verify align val */
#else
void	initdirs(), aligndata(), directive();

void	versioncheck();
int	obsdirect();
int	aligncheck();
#endif
