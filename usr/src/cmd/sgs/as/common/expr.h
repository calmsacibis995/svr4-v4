/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/expr.h	1.6"
/*
* common/expr.h - common assembler expression header
*
* Depends on:
*	"common/as.h"
*
* Includes:
*	"amode.h"	(implementation provides)
*/

	/*
	* Due to the limitations of certain compilers, the initial
	* values of the following enumeration constants are hard-
	* coded instead of the original expression now in comments.
	* When enough compilers are fixed, this comment should be
	* removed and the original initializers restored.
	*/

enum	/* expression operators */
{
	ExpOp_OPER_LeafFirst,
		ExpOp_LeafRegister = 0 /*ExpOp_OPER_LeafFirst*/,
		ExpOp_LeafName,
		ExpOp_LeafString,
		ExpOp_LeafNumber,
		ExpOp_LeafFloat,
		ExpOp_LeafCode,
	ExpOp_OPER_LeafLast,
	ExpOp_OPER_BinaryFirst = 6 /*ExpOp_OPER_LeafLast*/,
		ExpOp_Multiply = 6 /*ExpOp_OPER_BinaryFirst*/,
		ExpOp_Divide,
		ExpOp_Remainder,
		ExpOp_Add,
		ExpOp_Subtract,
		ExpOp_And,
		ExpOp_Or,
		ExpOp_Xor,
		ExpOp_Nand,
		ExpOp_LeftShift,
		ExpOp_RightShift,
		ExpOp_Maximum,		/* for internal calculations */
		ExpOp_LCM,		/* for internal calculations */
	ExpOp_OPER_BinaryLast,
	ExpOp_OPER_UnaryFirst = 19 /*ExpOp_OPER_BinaryLast*/,
		ExpOp_OPER_PrefixFirst = 19 /*ExpOp_OPER_BinaryLast*/,
			ExpOp_Complement = 19 /*ExpOp_OPER_PrefixFirst*/,
			ExpOp_UnaryAdd,
			ExpOp_Negate,
			ExpOp_SegmentPart,
			ExpOp_OffsetPart,
		ExpOp_OPER_PrefixLast,
		ExpOp_OPER_PostfixFirst = 24 /*ExpOp_OPER_PrefixLast*/,
			ExpOp_LowMask = 24 /*ExpOp_OPER_PostfixFirst*/,
			ExpOp_HighMask,
			ExpOp_HighAdjustMask,
			ExpOp_OPER_PicFirst,
				ExpOp_Pic_PC = 27 /*ExpOp_OPER_PicFirst*/,
				ExpOp_Pic_GOT,
				ExpOp_Pic_PLT,
				ExpOp_Pic_GOTOFF,
			ExpOp_OPER_PicLast,
		ExpOp_OPER_PostfixLast = 31 /*ExpOp_OPER_PicLast*/,
	ExpOp_OPER_UnaryLast = 31 /*ExpOp_OPER_PostfixLast*/,
	ExpOp_TOTAL = 31 /*ExpOp_OPER_UnaryLast*/	/* not an operator */
};

enum	/* expression types */
{
	ExpTy_Unknown,
	ExpTy_String,
	ExpTy_Register,
	ExpTy_Operand,		/* only due to .set identifier */
	ExpTy_Floating,
	ExpTy_Integral,
	ExpTy_Relocatable
};

enum	/* expression contexts */
{
	Cont_Unknown,	/* no known/current parent */
	Cont_Expr,	/* parent is expression */
	Cont_Set,	/* parent is operand with .set symbol parent */
	Cont_Label,	/* parent is label symbol */
	Cont_Operand	/* parent is operand */
};

	/*
	* The expression node is one of the most flexible data structures
	* in the assembler.  Each expression node has a parent pointer that
	* points to the data structure that refers to this node.  The value
	* of ex_cont (Cont_*) determines the appropriate parent pointer.
	* Within an expression, the parent is another expression node
	* (parent.ex_ptr). The top of each expression tree is pointed to
	* either by an operand or a symbol table entry.  The symbol table
	* entry only occurs for a label, and the expression is one that
	* evaluates to its address (the offset from the beginning of the
	* section in which the label is defined).  If parent of an expression
	* tree is an operand, it could be (part of) an operand for an
	* instruction or a directive.
	*
	* If expression's type starts out as unknown, later, when the
	* expression type becomes known, there are three means to check out
	* the result:
	*   1. oper_tybits is set to note valid expression types.
	*   2. operinst() is called to verify the operand for an instruction.
	*   3. a .set's operand is propagated to its uses.
	*
	* Note that left.ex_ptr is used to create a singly-linked list of
	* ExpOp_LeafNames for the same identifier (the head of the list
	* originating in the symbol table entry).  This allows exprtype()
	* to propagate changed expression type information about a symbol
	* to all dependent expressions.  So that the maximum checking occurs,
	* the type of an expression that contains an unknown node is unknown.
	*/

#define EXPMOD_PIC	0x01	/* PIC other than GOTOFF applied to reloc */
#define EXPMOD_GOTOFF	0x02	/* ExpOp_Pic_GOTOFF applied to relocatable */
#define EXPMOD_MASK	0x04	/* Low/High/HighAdjust applied to reloc */

struct t_expr_	/* arbitrary expression */
{
	union
	{
		Expr		*ex_ptr;	/* parent expr context */
		Symbol		*ex_sym;	/* label name context */
		Operand		*ex_oper;	/* operand context */
	} parent;
	union
	{
		Expr		*ex_ptr;	/* left/unary/next expr */
		size_t		ex_len;		/* right.ex_str length */
		Section		*ex_sect;	/* for ExpOp_LeafCode */
	} left;
	union
	{
		Expr		*ex_ptr;	/* binary right child */
		const Uchar	*ex_str;	/* for ExpOp_LeafString */
		const Number	*ex_num;	/* for ExpOp_LeafNumber */
		const Uchar	*ex_flt;	/* for ExpOp_LeafFloat */
		int		ex_reg;		/* for ExpOp_LeafRegister */
		Symbol		*ex_sym;	/* for ExpOp_LeafName */
		Code		*ex_code;	/* for ExpOp_LeafCode */
	} right;
	Uchar			ex_op;		/* ExpOp_* for node */
	Uchar			ex_cont;	/* Cont_* for parent */
	Uchar			ex_type;	/* ExpTy_* for node */
	Uchar			ex_mods;	/* applied modifiers */
};

	/* implementation provides */
#ifdef __STDC__
int	setamode(const Operand *);	/* save amode version of .set */
int	extyamode(const Operand *);	/* return composite type of amode */
#else
int	setamode(), extyamode();
#endif
#include "amode.h"	/* implementation's Amode typedef */

struct t_oper_	/* single operand information */
{
	Operand		*oper_next;	/* next operand in list */
	union
	{
		Oplist	*oper_olst;	/* parent oplist */
		Symbol	*oper_sym;	/* parent .set symbol */
	} parent;
	Expr		*oper_expr;	/* single operand expression */
	Amode		oper_amode;	/* extra address mode-type info */
	Uchar		oper_info;	/* imp-dep info about operand */
	Uchar		oper_flags;	/* non-zero => oper_amode used */
	Uchar		oper_tybits;	/* valid expr types as bits */
	Uchar		oper_setsym;	/* non-zero => parent is symbol */
};

struct t_olst_	/* entire statement operand list */
{
	Operand	*olst_first;	/* first operand in list */
	Operand	*olst_last;	/* last operand in list */
	Code	*olst_code;	/* only for instruction */
	Ulong	olst_line;	/* originating line number */
	Ushort	olst_file;	/* originating file number */
	Ushort	olst_impdep;	/* for implementations */
};

#ifdef __STDC__
char	*exopstr(int);				/* printable vers of op */
Expr	*regexpr(int);				/* make expr from reg */
Expr	*idexpr(const Uchar *, size_t);		/* make expr from name */
Expr	*strexpr(const Uchar *, size_t);	/* make expr from str */
Expr	*numexpr(const Uchar *, size_t);	/* expr from int token */
Expr	*ulongexpr(Ulong);			/* make expr from Ulong */
Expr	*fltexpr(const Uchar *, size_t);	/* expr from float token */
Expr	*dotexpr(Section *);			/* expr from code (dot) */
void	exprfrom(Ulong *, Ulong *, const Expr *);/* where originated */
void	exprtype(Expr *);			/* check/set expr types */
Expr	*setlessexpr(const Expr *);		/* expr w/out .set's */
void	fixexpr(Expr *);			/* fix expr to "dot" */
Expr	*unaryexpr(int, Expr *);		/* build unary expr */
Expr	*binaryexpr(int, Expr *, Expr *);	/* build binary expr */
Operand	*operand(Expr *);			/* build simple operand */
Oplist	*oplist(Oplist *, Operand *);		/* add to operand list */
void	printexpr(const Expr *);		/* output expr contents */
void	printoplist(const Oplist *);		/* output list contents */

		/* implementation must provide */
void	printoperand(const Operand *);		/* contents of operand */
#else
char	*exopstr();
Expr	*regexpr(), *idexpr(), *strexpr();
Expr	*numexpr(), *ulongexpr(), *fltexpr(), *dotexpr();
void	exprfrom(), exprtype();
Expr	*setlessexpr();
void	fixexpr();
Expr	*unaryexpr(), *binaryexpr();
Operand	*operand();
Oplist	*oplist();
void	printexpr(), printoplist();

void	printoperand();
#endif
