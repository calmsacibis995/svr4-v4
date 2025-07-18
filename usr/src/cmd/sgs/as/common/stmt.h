/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/stmt.h	1.2"
/*
* common/stmt.h - common assembler statement/instruction header
*
* Depends on:
*	"common/as.h"
*/

#ifdef __STDC__
   typedef size_t InstGen(Section *, Code *);
#else
   typedef size_t InstGen();
#endif

struct t_inst_	/* common information about an instruction */
{
	const Uchar	*inst_name;	/* instruction mnemonic */
	InstGen		*inst_gen;	/* pointer to generation fcn */
	Ushort		inst_impdep;	/* for implementation use */
	Ushort		inst_minsz;	/* minimum size; used first */
};

	/* implementation provides */
#ifdef __STDC__
void	operinst(const Inst *, Operand *);	/* check inst operand */
void	gennops(Section *, const Code *);	/* fill with "nop"s */
#else
void	operinst(), gennops();
#endif
