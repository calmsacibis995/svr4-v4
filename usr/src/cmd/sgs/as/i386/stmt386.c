/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:i386/stmt386.c	1.8"
/*
* i386/stmt386.c - i386 assembler instructions and statements
*/
#include <stdio.h>
#include "common/as.h"
#include "common/expr.h"
#include "common/nums.h"
#include "common/stmt.h"
#include "common/sect.h"
#include "common/syms.h"
#include "dirs386.h"
#include "chkgen.h"
#include "relo386.h"
#include "stmt386.h"


/* Define OLD_AS_COMPAT to get instruction encodings consistent
** with the way the old assembler did things.
*/
#undef OLD_AS_COMPAT

typedef Uchar Opclass;		/* big enough to hold an operand class */

/* Table of register information, indexed by register number
** (i.e., Reg_ecx):
**	register class
**	register's encoding in an instruction, values 0-7
**	register's presumed size as an operand (in bytes)
**	register's print-name.
**
** Obviously the entries must be in the order of the register
** numbers declared in stmt386.h.
*/

typedef struct {
    Opclass r_flags;			/* register's operand class */
    Uchar r_code;			/* register's instruction encoding */
    Uchar r_size;			/* register's presumed size (bytes) */
    const char * r_name;		/* register's print name */
} r_info;


static const r_info reginfo[Reg_TOTAL] = {
    { OC_R32, 0, 4, "%eax" },	{ OC_R32, 1, 4, "%ecx" },
    { OC_R32, 2, 4, "%edx" },	{ OC_R32, 3, 4, "%ebx" },
    { OC_R32, 4, 4, "%esp" },	{ OC_R32, 5, 4, "%ebp" },
    { OC_R32, 6, 4, "%esi" },	{ OC_R32, 7, 4, "%edi" },

    { OC_R16, 0, 2, "%ax" },	{ OC_R16, 1, 2, "%cx" },
    { OC_R16, 2, 2, "%dx" },	{ OC_R16, 3, 2, "%bx" },
    { OC_R16, 4, 2, "%sp" },	{ OC_R16, 5, 2, "%bp" },
    { OC_R16, 6, 2, "%si" },	{ OC_R16, 7, 2, "%di" },

    { OC_R8, 0, 1, "%al" },	{ OC_R8, 1, 1, "%cl" },
    { OC_R8, 2, 1, "%dl" },	{ OC_R8, 3, 1, "%bl" },
    { OC_R8, 4, 1, "%ah" },	{ OC_R8, 5, 1, "%ch" },
    { OC_R8, 6, 1, "%dh" },	{ OC_R8, 7, 1, "%bh" },

    { OC_SEG, 0, 2, "%es" },	{ OC_SEG, 1, 2, "%cs" },
    { OC_SEG, 2, 2, "%ss" },	{ OC_SEG, 3, 2, "%ds" },
    { OC_SEG, 4, 2, "%fs" },	{ OC_SEG, 5, 2, "%gs" },

    { OC_SPEC, 0, 4, "%cr0" },	{ OC_SPEC, 2, 4, "%cr2" },
    { OC_SPEC, 3, 4, "%cr3" },

    { OC_SPEC, 3, 4, "%tr3" },	{ OC_SPEC, 4, 4, "%tr4" },
    { OC_SPEC, 5, 4, "%tr5" },	{ OC_SPEC, 6, 4, "%tr6" },
    { OC_SPEC, 7, 4, "%tr7" },

    { OC_SPEC, 0, 4, "dr0" },	{ OC_SPEC, 1, 4, "dr1" },
    { OC_SPEC, 2, 4, "dr2" },	{ OC_SPEC, 3, 4, "dr3" },
    { OC_SPEC, 6, 4, "dr6" },	{ OC_SPEC, 7, 4, "dr7" },
 
    { OC_ST, 0, 8, "%st" },
    { OC_STn, 0, 8, "%st(0)" },	{ OC_STn, 1, 8, "%st(1)" },
    { OC_STn, 2, 8, "%st(2)" },	{ OC_STn, 3, 8, "%st(3)" },
    { OC_STn, 4, 8, "%st(4)" },	{ OC_STn, 5, 8, "%st(5)" },
    { OC_STn, 6, 8, "%st(6)" },	{ OC_STn, 7, 8, "%st(7)" },
};


/* This is a table of segment prefixes by segment register
** number, where the segment register number comes from the
** table above.
*/
static const Uchar seg_reg[] = {
	0x26,	/* Reg_es */	0x2E,	/* Reg_cs */
	0x36,	/* Reg_ss */	0x3E,	/* Reg_ds */
	0x64,	/* Reg_fs */	0x65,	/* Reg_gs */
	0xFF, 0xFF	/* dummy values */
};

/* This field is used to keep track of how many unchecked operands
** there are.  When they have all been checked, the entire instruction
** can be checked.
*/
#define code_unchecked code_impdep

/* This field is reused to keep track of the largest extent for
** the variable-size portion of an instruction.  The value is
** never allowed to get smaller, in keeping with span-dependent
** code constraints.
*/
#define code_varsize code_impdep

#define NOP_code 0x90
#define WORD_code 0x66		/* word-operand override */
#define JMP8_code 0xEB		/* code for 1-byte displacement jmp */
#define	FWAIT_code 0x9B		/* code for wait/fwait */

/* Macro to check whether a value always fits in a given number
** of bits.  If it depends on a difference of labels, we can't
** assume the value will always fit.
*/
#define ALWAYS_FITS_IN(vp, n) \
	(vp->val_snbits <= n && (vp->val_flags & (VAL_LDIFF|VAL_RELOC|VAL_OFLOW)) == 0)

/* This macro does the same thing, but the value can be a difference
** of two labels, in which case we're prepared for the difference to
** grow.
*/
#define FITS_IN(vp, n) \
	(vp->val_snbits <= n && (vp->val_flags & (VAL_RELOC|VAL_OFLOW)) == 0)


/* Macro that defines bad operand combination. */
#define	BAD_COMBO	((Ushort) ~0)

#ifdef __STDC__
static void doinst(const Inst *, Oplist *);
static void chk1oper(Operand *);
static void chk_list(Code *, const chklist_t *);
static void do_oper(Operand *);
static int gen_getregno(Expr *);
static Uchar gen_seg_override(const Operand *);
static size_t gen_lit(Operand *, int, Section *, Uchar *);
static size_t gen_slashr(int, Operand *, Section *, Uchar *);
static size_t gen_pcrel(Operand *, Section *, int, size_t, int);
static void invalidoper(Code *);
static void invalidreg(Inst386 *, Operand *);
#else
static void doinst();
static void chk1oper();
static void chk_list();
static void do_oper();
static int gen_getregno();
static Uchar gen_seg_override();
static size_t gen_lit();
static size_t gen_slashr();
static size_t gen_pcrel();
static void invalidoper();
static void invalidreg();
#endif

/* Produce a diagnostic relating to an operand.  Mark operand as erroneous. */
#define opererror(s,op) \
	op->oper_info = OC_error,				\
	backerror((Ulong) op->parent.oper_olst->olst_file,	\
			op->parent.oper_olst->olst_line, s)


/* Produce warning relating to an operand. */
#define operwarn(s,op) \
	backwarn((Ulong) op->parent.oper_olst->olst_file,	\
			op->parent.oper_olst->olst_line, s)


/* Return register encoding number for register expression. */
#define gen_regcode(ep) (reginfo[gen_getregno(ep)].r_code)


/* Check an operand.  If all operands for an instruction have
** been checked, then check the instruction itself.
*/

/*ARGSUSED*/
void
#ifdef __STDC__
operinst(const Inst *instp, Operand *op) /* final single operand check */
#else
operinst(instp, op)Inst *instp; Operand *op;
#endif
{
    Code *cp = op->parent.oper_olst->olst_code;

    if (cp->code_unchecked == 0)
	fatal("operinst():all operands checked already");
    /* Check the operand if we know everything about it. */
    if (extyamode(op) != ExpTy_Unknown)
	do_oper(op);			/* out of line check */
    return;
}

#ifdef DEBUG
void
#ifdef __STDC__
printoperand(const Operand *op)	/* output contents of operand */
#else
printoperand(op)Operand *op;
#endif
{
	if (op == 0)
	{
		(void)fputs("(Operand*)0", stderr);
		return;
	}
	if (op->oper_flags & Amode_Literal)
	{
		if (op->oper_flags != Amode_Literal)
		{
			fatal("printoperand():complex literal operand: %u",
				(Uint)op->oper_flags);
		}
		(void)putc('$', stderr);
	}
	if (op->oper_flags & Amode_Indirect)
		(void)putc('*', stderr);
	if (op->oper_flags & Amode_Segment)
	{
		printexpr(op->oper_amode.seg);
		(void)putc(':', stderr);
	}
	if (op->oper_flags & Amode_FPreg)
	{
		(void) fputs("%st(", stderr);
		printexpr(op->oper_expr);
		(void)putc(')', stderr);
	}
	else if (op->oper_expr != 0)
		printexpr(op->oper_expr);
	if (op->oper_flags & Amode_BIS)
	{
		(void)putc('(', stderr);
		if (op->oper_amode.base != 0)
			printexpr(op->oper_amode.base);
		if (op->oper_amode.index != 0)
		{
			(void)putc(',', stderr);
			printexpr(op->oper_amode.index);
			if (op->oper_amode.scale != 0)
			{
				(void)putc(',', stderr);
				printexpr(op->oper_amode.scale);
			}
		}
		else if (op->oper_amode.scale != 0)
			fatal("printoperand():scale w/out index");
		(void)putc(')', stderr);
	}
}
#endif	/*DEBUG*/


/* Return currently known type for an operand.  This routine gets
** called from common and mdp code.  Although the common code never
** calls us with oper_flags == 0, it's convenient, for the mdp code,
** to allow that case.
*/

int
#ifdef __STDC__
extyamode(register const Operand *op)
#else
extyamode(op)register Operand *op;
#endif
{
	register Expr *ep;

	if (op->oper_flags == 0)
	    return( op->oper_expr->ex_type );
	ep = op->oper_expr;
	if ((op->oper_flags & Amode_Literal) != 0)
	{
		if (ep == 0)
			fatal("extyamode():expr-less literal");
		if (ep->ex_type == ExpTy_Unknown)
			return ExpTy_Unknown;
		return ExpTy_Operand;
	}
	if (ep != 0 && ep->ex_type == ExpTy_Unknown)
		return ExpTy_Unknown;
	if ((ep = op->oper_amode.seg) != 0 && ep->ex_type == ExpTy_Unknown)
		return ExpTy_Unknown;
	if ((ep = op->oper_amode.base) != 0 && ep->ex_type == ExpTy_Unknown)
		return ExpTy_Unknown;
	if ((ep = op->oper_amode.index) != 0 && ep->ex_type == ExpTy_Unknown)
		return ExpTy_Unknown;
	if ((ep = op->oper_amode.scale) != 0 && ep->ex_type == ExpTy_Unknown)
		return ExpTy_Unknown;
	return ExpTy_Operand;
}

int
#ifdef __STDC__
setamode(register const Operand *op)	/* fix amode as .set; return type */
#else
setamode(op)register Operand *op;
#endif
{
	register Expr *ep;
	register int exty = ExpTy_Operand;

	if (op->oper_flags == 0)
		fatal("setamode():single expression operand");
	if ((ep = op->oper_expr) != 0)
	{
		fixexpr(ep);
		ep->ex_cont = Cont_Set;
		/* Early exit for literal, %st(n) */
		if ((op->oper_flags & (Amode_Literal|Amode_FPreg)) != 0)
			return ExpTy_Operand;
		if (ep->ex_type == ExpTy_Unknown)
			exty = ExpTy_Unknown;
	}
	if ((ep = op->oper_amode.seg) != 0)
	{
		fixexpr(ep);
		ep->ex_cont = Cont_Set;
		if (ep->ex_type == ExpTy_Unknown)
			exty = ExpTy_Unknown;
	}
	if (op->oper_flags & Amode_BIS) {
	    if ((ep = op->oper_amode.base) != 0)
	    {
		    fixexpr(ep);
		    ep->ex_cont = Cont_Set;
		    if (ep->ex_type == ExpTy_Unknown)
			    exty = ExpTy_Unknown;
	    }
	    if ((ep = op->oper_amode.index) != 0)
	    {
		    fixexpr(ep);
		    ep->ex_cont = Cont_Set;
		    if (ep->ex_type == ExpTy_Unknown)
			    exty = ExpTy_Unknown;
	    }
	    if ((ep = op->oper_amode.scale) != 0)
	    {
		    fixexpr(ep);
		    ep->ex_cont = Cont_Set;
		    if (ep->ex_type == ExpTy_Unknown)
			    exty = ExpTy_Unknown;
	    }
	}
	return exty;
}


/* Check operands for proper form. */


/* Substitute for any .set symbols in an operand.  The general
** form is shown below, and that parts that can be substituted
** are underlined.  Note that relationship to the operand parsing
** in parse.c.
**
**	[seg:][expr][([r][,r[,expr]])]
**	 ---   ----    -   -  ----
**	       ----------------------
**	 ----------------------------
**
*/

static void
#ifdef __STDC__
setsubst(Operand *op)
#else
setsubst(op) Operand *op;
#endif
{
    Expr *ep;
    Operand *setop;			/* operand that's subject of .set */

    op->oper_info = (Uchar) ~0;		/* Pick absurd invalid value so we
					** can tell (chk1oper()) if there has
					** been an error yet.
					*/
    /* As a first step, substitute for individual pieces. */
    if (op->oper_flags & Amode_Segment) {
	ep = op->oper_amode.seg;
	if (ep->ex_type == ExpTy_Register && ep->ex_op != ExpOp_LeafRegister)
	    op->oper_amode.seg = setlessexpr(ep);
    }
    if (op->oper_flags & Amode_BIS) {
	if ((ep = op->oper_amode.base) != 0) {
	    if (   ep->ex_type == ExpTy_Register
		&& ep->ex_op != ExpOp_LeafRegister
	    )
		op->oper_amode.base = setlessexpr(ep);
	}
	if ((ep = op->oper_amode.index) != 0) {
	    if (   ep->ex_type == ExpTy_Register
		&& ep->ex_op != ExpOp_LeafRegister
	    )
		op->oper_amode.index = setlessexpr(ep);
	}
    }

    if (op->oper_expr == 0)
	return;				/* no further substitutions */

    /* Follow chain of .set's for "main" expression until we find
    ** one that isn't a simple ".set x,y".  Then look for possible
    ** substitutions of large pieces.
    */
    for (ep = op->oper_expr; ep->ex_op == ExpOp_LeafName; ) {
	Symbol *sp = ep->right.ex_sym;

	if (sp->sym_kind != SymKind_Set)
	    break;
	if (sp->sym_exty == ExpTy_Unknown)
	    fatal("setsubst():  unknown symbol type?");
	/* Check for "complex" operand. */
	if ((setop = sp->addr.sym_oper)->oper_flags != 0)
	    break;
	if ((ep = setop->oper_expr) == 0)
	    fatal("setsubst(): can't follow .set chain");
    }

    switch(ep->ex_type) {
    default:
	/* %st(n) case handled elsewhere, and better. */
	if ((op->oper_flags & Amode_FPreg) == 0)
	    opererror("operand must be integral or relocatable", op);
	break;
    case ExpTy_Integral:
    case ExpTy_Relocatable:
    case ExpTy_Register:
	op->oper_expr = ep;
	break;
    case ExpTy_Operand:
	/* "setop" could have embedded .set's in it that have not
	** yet been resolved.  Resolve them.
	*/
	setsubst(setop);

	/* Only excitement is when .set operand has Amode_ bits. */
	if (setop->oper_flags != 0) {
	    /* No duplicate flags.  Can't have FPreg in one and
	    ** anything in the other.
	    */
	    if (   (setop->oper_flags & op->oper_flags) != 0
		|| (setop->oper_flags & Amode_FPreg) != 0 &&    op->oper_flags
		|| (   op->oper_flags & Amode_FPreg) != 0 && setop->oper_flags
	    )
		opererror(".set substitution produces invalid operand", op);
	    else {
		op->oper_flags |= setop->oper_flags;
		op->oper_expr = setop->oper_expr;
		if (setop->oper_flags & Amode_Segment)
		    op->oper_amode.seg = setop->oper_amode.seg;
		if (setop->oper_flags & Amode_BIS) {
		    op->oper_amode.base = setop->oper_amode.base;
		    op->oper_amode.index = setop->oper_amode.index;
		    op->oper_amode.scale = setop->oper_amode.scale;
		}
	    }
	}
	break;
    }
    return;
}


/* Process a single operand:  do any substitutions resulting
** from .set symbols.  Check the operand.  Decrement the
** associated instruction's count of operands to check.
** If zero, call the instruction's checking routine for
** operands.  The operand is presumed to have completely
** known expression types.
*/
static void
#ifdef __STDC__
do_oper(Operand *op)
#else
do_oper(op) Operand *op;
#endif
{
    Code *cp = op->parent.oper_olst->olst_code;

    if (cp->code_unchecked == 0)
	fatal("do_oper():  all operands checked?");
    setsubst(op);
    chk1oper(op);

    /* If we have complete information about all the operands,
    ** check the operand combinations.  If there is a list,
    ** try chk_list first.  Then try the check routine, if
    ** there is one.
    */
    if ( --cp->code_unchecked == 0 ) {
	Inst386 *ip3 = (Inst386 *) cp->info.code_inst;

	if (ip3->chklist)
	    chk_list(cp, ip3->chklist);
	if (ip3->operchk)
	    (*ip3->operchk)(cp);
    }
    return;
}

/* Check a list of operand combinations for a match.  There
** may be one or two operands to match.  If a match is found,
** the number of the matching combination is stored in
** olst_combo.  Otherwise a diagnostic is printed if neither
** of the operands has operand class OC_error.
*/

static void
#ifdef __STDC__
chk_list(Code *cp, const chklist_t *clp)
#else
chk_list(cp, clp) Code *cp; chklist_t *clp;
#endif
{
    Operand *first = cp->data.code_olst->olst_first;
    Operand *second;
    Inst386 *ip3 = (Inst386 *) cp->info.code_inst;
    Uchar checkval;
    Uchar l_auxno, r_auxno;		/* auxiliary numbers */
    const chklist_t *clp2;
    Expr *ep;
    
    /* A single operand is treated as the right operand. */
    if ((ep = first->oper_expr)->ex_type == ExpTy_Register)
	r_auxno = ep->right.ex_reg + 1;
    else
	r_auxno = ip3->opersize;
    

    if ((second = first->oper_next) != 0) {
	/* First (left, syntactically) operand is second in i386 book. */
	checkval = CASEVAL(second->oper_info, first->oper_info);
	ep = second->oper_expr;
	if (ep->ex_type == ExpTy_Register)
	    l_auxno = ep->right.ex_reg + 1;
	else
	    l_auxno = ip3->opersize;
    }
    else
	checkval = CASEVAL(first->oper_info,0);

    /* Look for matching list entry. */
    for (clp2 = clp; clp2->cl_combo != 0; ++clp2) {
	if (clp2->cl_combo != checkval) continue;
	/* Look for matching register or operand size on both sides. */
	if (clp2->cl_l_auxno != 0  && clp2->cl_l_auxno != l_auxno)
	    continue;
	if (clp2->cl_r_auxno != 0 && clp2->cl_r_auxno != r_auxno)
	    continue;

	/* Found a match.  Record combination in operand list. */
	cp->data.code_olst->olst_combo = clp2 - clp;
#ifdef DEBUG
	if (DEBUG('C') > 0) {
	    /* print mnemonic and combination number */
	    (void) fprintf(stderr, "%s\t%d\n",
			(const char *) ((Inst *)ip3)->inst_name, clp2-clp);
	}
#endif
	return;
    }
    /* Suppress diagnostic if one of the operands has type OC_error:
    ** presumably a diagnostic has been issued already.
    */
    if (   first->oper_info != OC_error
	&& (second == 0 || second->oper_info != OC_error)
	)
	invalidoper(cp);

    /* Choose combination that won't match any in check-routine. */
    cp->data.code_olst->olst_combo = BAD_COMBO;
    return;
}
    
/* Check a single operand for context-independent correctness. */

static void
#ifdef __STDC__
chk1oper(Operand * op)
#else
chk1oper(op) Operand * op;
#endif
{
    int fl = op->oper_flags;
    Expr * ep;

    /* Assume operand is okay. */
    if (op->oper_info != OC_error)
	op->oper_info = OC_MEM;		/* assume a memory operand if no
					** errors yet; if an error is detected,
					** opererror() will overwrite
					*/

    /* Check literal for proper types. */
    if (fl & Amode_Literal) {
	switch( op->oper_expr->ex_type ){
	case ExpTy_Integral:
	case ExpTy_Relocatable:
	    op->oper_info = OC_LIT;
	    break;			/* these are okay */
	default:
	    opererror("literal must be integral or relocatable", op);
	}
	return;
    }

    if (   (fl & Amode_Indirect) != 0
	&& ((op->parent.oper_olst->olst_code->info.code_inst)->chkflags & IF_STAR) == 0
    ) {
	operwarn("'*' indirection invalid here", op);
	op->oper_flags &= ~Amode_Indirect;
    }

    /* Check segment. */
    if (fl & Amode_Segment) {
	ep = op->oper_amode.seg;
	if (   ep->ex_op != ExpOp_LeafRegister
	    || (reginfo[ep->right.ex_reg].r_flags != OC_SEG)
	)
	    opererror("invalid segment register", op);
	
	/* Segment register cannot modify register operand (without '*'). */
	if (   (ep = op->oper_expr) != 0
	    && ep->ex_type == ExpTy_Register
	    && (op->oper_flags & Amode_Indirect) == 0
	    )
	    operwarn("segment override ineffective with register operand", op);
    }
    /* Check floating register. */
    if (fl & Amode_FPreg) {
	static const char mesg[] =
		"floating point register designator must be integral, 0-7";
	ep = op->oper_expr;
	if (ep->ex_type != ExpTy_Integral)
	    opererror(mesg, op);
	else {
	    Value * vp;
	    vp = evalexpr(ep);
	    if (   (vp->val_flags & (VAL_OFLOW|VAL_TRUNC)) != 0
		|| vp->val_ulong > 7
	    )
		opererror(mesg, op);
	    else {
		if (vp->val_flags & VAL_LDIFF)	/* register for later */
		    delayeval(vp);

		ep = regexpr(Reg_st0 + vp->val_ulong);
		ep->ex_cont = Cont_Operand;
		ep->parent.ex_oper = op;
		op->oper_expr = ep;
		op->oper_flags &= ~Amode_FPreg;
	    }
	}
    }

    /* If no base/index/displacement, operand must be register,
    ** integer, or relocatable.
    */
    if ((fl & Amode_BIS) == 0) {
	ep = op->oper_expr;
	switch( ep->ex_type ){
	default:
	    break;			/* setsubst() already detected these */
	case ExpTy_Register:
	{
	    Inst386 *ip3 = (Inst386 *) op->parent.oper_olst->olst_code->info.code_inst;
	    if (ep->ex_op != ExpOp_LeafRegister)
		fatal("chk1oper(): confused register");
	    op->oper_info = reginfo[ep->right.ex_reg].r_flags;
	    if (((Inst *) ip3)->chkflags & IF_RSIZE) {
		if (ip3->opersize != reginfo[ep->right.ex_reg].r_size)
		    invalidreg(ip3, op);
	    }
	    break;
	}
	case ExpTy_Integral:
	case ExpTy_Relocatable:
	    break;
	}
    }
    else {
	/* Base and index must be 32-bit registers; index must be
	** integral and 1, 2, 4, 8.
	*/
	if ((ep = op->oper_amode.base) != 0) {
	    if (   ep->ex_op != ExpOp_LeafRegister
		|| (reginfo[ep->right.ex_reg].r_flags != OC_R32)
	    ) {
		/* There's one special case (uugh!):  %dx can look
		** like base register for in/out instructions (IF_BASE_DX
		** set).
		*/
		if (   ep->right.ex_reg != Reg_dx
		    || (op->parent.oper_olst->
			    olst_code->
				info.code_inst->chkflags & IF_BASE_DX) == 0
		    )
		    opererror("base register must be 32-bit register", op);
	    }
	}
	if ((ep = op->oper_amode.index) != 0) {
	    if (   ep->ex_op != ExpOp_LeafRegister
		|| (reginfo[ep->right.ex_reg].r_flags != OC_R32)
	    )
		opererror("index register must be 32-bit register", op);
	    else if (ep->right.ex_reg == Reg_esp) {
		opererror("index register cannot be %%esp", op);
	    }
	}
	if ((ep = op->oper_amode.scale) != 0) {
	    if (ep->ex_type != ExpTy_Integral)
		opererror("scale must be integral", op);
	    else {
		/* Check for valid scale value. */
		Value * vp = evalexpr(ep);

		switch ( vp->val_ulong ){
		case 1:
		case 2:
		case 4:
		case 8:
		    if ((vp->val_flags & (VAL_OFLOW|VAL_TRUNC)) == 0) {
			if (vp->val_flags & VAL_LDIFF)
			    delayeval(vp);
			break;
		    }
		    /* error on overflow/value too big */
		    /*FALLTHRU*/
		default:
		    opererror("scale must be 1, 2, 4, or 8", op);
		    break;
		}
	    }
	}
    }
    return;
}


/* Print "invalid operand combination" diagnostic. */
static void
#ifdef __STDC__
invalidoper(Code *cp)
#else
invalidoper(cp) Code *cp;
#endif
{
    static const char mesg[] = "invalid operand combination: %s";
    Oplist *parent = cp->data.code_olst;

    backerror((Ulong) parent->olst_file, parent->olst_line,
		mesg, (const char *) cp->info.code_inst->inst_name);
    return;
}

/* Print "invalid register for instruction: %s in %s". */
static void
#ifdef __STDC__
invalidreg(Inst386 *ip3, Operand *op)
#else
invalidreg(ip3, op) Inst386 *ip3; Operand *op;
#endif
{
    backerror((Ulong) op->parent.oper_olst->olst_file,
			op->parent.oper_olst->olst_line,
			"invalid register for instruction: %s in %s",
			reginfo[op->oper_expr->right.ex_reg].r_name,
			(const char *) ((Inst *) ip3)->inst_name);

    op->oper_info = OC_error;		/* mark operand as erroneous */
    return;
}


void
#ifdef __STDC__
stmt(const Uchar *str, size_t len, Oplist *olp)	/* handle statement */
#else
stmt(str, len, olp)Uchar *str; size_t len; Oplist *olp;
#endif
{
	const Inst * ip;
#ifdef DEBUG
	if (DEBUG('I') > 0)
	{
		(void)fprintf(stderr, "stmt(%s,ops=", prtstr(str, len));
		printoplist(olp);
		(void)fputs(")\n", stderr);
	}
#endif
	if (str[0] == '.')	/* must be a directive */
	{
		directive386(str, len, olp);
		return;
	}
	if ((ip = findinst(str, len)) != 0)
		doinst(ip, olp);
}


/* Do initial processing on instruction:
**	1.  Count the operands
**	2.  Check operand count
**	3.  For each operand, check its health
**	4.  Build Code entry for the instruction.
*/
static void
#ifdef __STDC__
doinst(const Inst *ip, Oplist *olp)
#else
doinst(ip, olp) Inst *ip; Oplist *olp;
#endif
{
    Uchar numops = 0;
    Operand * op;
    Inst386 *ip3 = (Inst386 *) ip;

    if (olp) {
	for (op = olp->olst_first; op != 0; op = op->oper_next) {
	    /* Fix each expression. */
	    if (op->oper_expr)
		fixexpr(op->oper_expr);
	    if (op->oper_flags & Amode_BIS) {
		if (op->oper_amode.base != 0)
		    fixexpr(op->oper_amode.base);
		if (op->oper_amode.index != 0)
		    fixexpr(op->oper_amode.index);
		if (op->oper_amode.scale != 0)
		    fixexpr(op->oper_amode.scale);
	    }
	    ++numops;
	}
    }
    
    if (numops < ip3->minops)
	error("too few operands: %s", (const char *) ip->inst_name);
    else if (numops > ip3->maxops)
	error("too many operands: %s", (const char *) ip->inst_name);
    else {
	Section * secp = cursect();
	Code * cp = secp->sec_last;	/* current last will be new Code */

	if (ip->chkflags & IF_VARSIZE)
	    sectvinst(secp, ip, olp);
	else
	    sectfinst(secp, ip, olp);

	cp->code_unchecked = numops;

	if (olp) {
	    for (op = olp->olst_first; op != 0; op = op->oper_next) {
		if (extyamode(op) != ExpTy_Unknown)
		    do_oper(op);	/* checking in line */
	    }
	}
    }
    return;
}
 
/*********************************************************
**							**
**	Routines to Check Operand Combinations		**
**							**
*********************************************************/
/* The routines below get called to check combinations of
** operands.  Unless otherwise noted, chk_list() has already
** been called, and these routines just select special case
** encodings.  In rare instances, the routines here do all
** the checking.
**
** For those cases where there are both a check- and a
** gen-routine for an instruction, the check routine will
** be found just before the gen-routine.
*/


/* Check arithmetic/logical instructions.  Select a small-form
** literal if we know it always fits.  Otherwise leave the
** large-form encoding as the one to use.  If the left (in the
** table) operand is al/ax/eax, use the special encoding for it.
*/

/*ARGSUSED*/
void
#ifdef __STDC__
chk_ar2(Code *cp)
#else
chk_ar2(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;

    switch ( olp->olst_combo ){
	Value *vp;
    case ar2_rm32_lit:		/* r32,$ */
#ifdef OLD_AS_COMPAT
	/* Old assembler chose %eax encoding always. */
	if ( gen_getregno(olp->olst_first->oper_next->oper_expr) == Reg_eax )
	    olp->olst_combo += ar2_eax_lit - ar2_rm32_lit;
	else if (ALWAYS_FITS_IN(evalexpr(olp->olst_first->oper_expr), 8))
	    olp->olst_combo += ar2_lit8_32 - ar2_rm32_lit;
#else
	if (ALWAYS_FITS_IN(evalexpr(olp->olst_first->oper_expr), 8))
	    olp->olst_combo += ar2_lit8_32 - ar2_rm32_lit;
	else if ( gen_getregno(olp->olst_first->oper_next->oper_expr) == Reg_eax )
	    olp->olst_combo += ar2_eax_lit - ar2_rm32_lit;
#endif
	break;
    case ar2_rm32_lit+1:	/* m32,$ */
	vp = evalexpr(olp->olst_first->oper_expr);
	if (ALWAYS_FITS_IN(vp, 8))
	    olp->olst_combo += ar2_lit8_32 - ar2_rm32_lit;
	break;
    case ar2_rm16_lit:		/* r16,$ */
	if (gen_getregno(olp->olst_first->oper_next->oper_expr) == Reg_ax) {
	    olp->olst_combo += ar2_ax_lit - ar2_rm16_lit;
	    break;
	}
	/*FALLTHRU*/
    case ar2_rm16_lit+1:	/* m16,$ */
	vp = evalexpr(olp->olst_first->oper_expr);
	if (ALWAYS_FITS_IN(vp, 8))
	    olp->olst_combo += ar2_lit8_16 - ar2_rm16_lit;
	break;

    case ar2_r8_lit:		/* r8,$ */
	if (gen_getregno(olp->olst_first->oper_next->oper_expr) == Reg_al)
	    olp->olst_combo += ar2_al_lit - ar2_r8_lit;
	break;
    }
	
    return;
}


/* Check in/out instructions.  Select proper encoding based on
** instruction (operand) size.  Verify, for "m" case, that the
** operand looks like "(%dx)".
*/

void
#ifdef __STDC__
chk_inout(Code *cp)
#else
chk_inout(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;

    if (olp->olst_combo == cl_inout_dx) {
	/* Make sure operand has proper form. */
	Operand *op = olp->olst_first;

	if (   op->oper_expr != 0
	    || (op->oper_flags & Amode_BIS) == 0
	    || op->oper_amode.base == 0
	    || op->oper_amode.base->right.ex_reg != Reg_dx
	    || op->oper_amode.index != 0
	    || op->oper_amode.scale != 0
	)
	    opererror("operand must be \"(%%dx)\" or literal", op);
    }

    /* Choose appropriate encoding variant of literal or (%dx). */
    switch ( ((Inst386 *)cp->info.code_inst)->opersize ) {
    default:	fatal("chk_inout(): bad size");	/*NOTREACHED*/
    case 4:	++olp->olst_combo;	/*FALLTHRU*/
    case 2:	++olp->olst_combo;	/*FALLTHRU*/
    case 1:	break;
    }
    return;
}

/* Check int instruction:  select special case for 3. */

/*ARGSUSED*/
void
#ifdef __STDC__
chk_int(Code *cp)
#else
chk_int(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Value *vp;

    if (olp->olst_combo == 0) {
	vp = evalexpr(olp->olst_first->oper_expr);
	if (ALWAYS_FITS_IN(vp, 8) && vp->val_ulong == 3)
	    olp->olst_combo += cl_int_3 - 0;
    }
    return;
}


/* Most of checking for mov instructions is taken care of by
** chk_list().  Change move of special register to appropriate
** variant.  For move between register and memory, if the register
** is %[e]a[xl] and the memory operand is a pure offset, choose the
** special encoding.
*/

void
#ifdef __STDC__
chk_mov(Code *cp)
#else
chk_mov(cp) Code *cp;
#endif
{
    Ushort combo = cp->data.code_olst->olst_combo;
    Operand *spreg = 0;

    /* Remember that operands in table are reversed from operand
    ** order of assembly syntax.
    */
    switch( combo ){
	Operand *op;
    case mov_spreg:		/* special from CPU register */
	spreg = cp->data.code_olst->olst_first;
	break;
    case mov_spreg+1:		/* CPU register from special */
	spreg = cp->data.code_olst->olst_first->oper_next;
	break;
    case mov_regmem:
    case mov_regmem+2:
    case mov_regmem+4:
	/* Use special form if %[e]a[xl] and not base/index/scale. */
	op = cp->data.code_olst->olst_first;
	if (   (op->oper_flags & Amode_BIS) == 0
	    && gen_regcode(op->oper_next->oper_expr) == 0
	    )
	    combo += mov_axmoff - mov_regmem;
	break;
    case mov_memreg:
    case mov_memreg+2:
    case mov_memreg+4:
	/* Use special form if %[e]a[xl] and not base/index/scale. */
	op = cp->data.code_olst->olst_first;
	if (   (op->oper_next->oper_flags & Amode_BIS) == 0
	    && gen_regcode(op->oper_expr) == 0
	    )
	    combo += mov_moffax - mov_memreg;
	break;
    }
    /* Get the register number so we can discern its type and
    ** choose the correct gen_list variant.
    */
    if (spreg) {
	switch( gen_getregno(spreg->oper_expr) ) {
	case Reg_cr0: case Reg_cr2: case Reg_cr3:
	    combo += mov_cr - mov_spreg;
	    break;
	case Reg_dr0: case Reg_dr1: case Reg_dr2: case Reg_dr3:
	case Reg_dr6: case Reg_dr7:
	    combo += mov_dr - mov_spreg;
	    break;
	case Reg_tr3: case Reg_tr4: case Reg_tr5: case Reg_tr6:
	case Reg_tr7:
	    combo += mov_tr - mov_spreg;
	    break;
	default:
	    fatal("chk_mov(): confused special register");
	}
    }
    cp->data.code_olst->olst_combo = combo;
    return;
}


/* Check pop instruction:  choose variant for pop-segment-reg.
** Check operand size for register.
*/

void
#ifdef __STDC__
chk_pop(Code *cp)
#else
chk_pop(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    int instoper = ((Inst386 *) cp->info.code_inst)->opersize;
    int opersize = 0;

    switch( olp->olst_combo ){
	Ushort combo;
    case pop_r16:	opersize = 2; break;
    case pop_r32:	opersize = 4; break;
    case pop_sreg:
	/* Avoid operand size check:  allow popw or popl of segment. */
	switch( gen_getregno(olp->olst_first->oper_expr) ){
	case Reg_cs:	opersize = 100; break;	/* so we get diagnostic */
	case Reg_ss:	combo = pop_ss; break;
	case Reg_ds:	combo = pop_ds; break;
	case Reg_es:	combo = pop_es; break;
	case Reg_fs:	combo = pop_fs; break;
	case Reg_gs:	combo = pop_gs; break;
	}
	if (instoper == 2)
	    combo += pop_sreg_w - pop_sreg;	/* choose popw versions */

	olp->olst_combo = combo;
	break;
    }
    if (opersize && opersize != instoper)
	invalidreg((Inst386 *) cp->info.code_inst, olp->olst_first);
    return;
}

/* Check push instruction:  choose variant for push-segment-reg.,
** literal.  Check operand size for register.
*/

void
#ifdef __STDC__
chk_push(Code *cp)
#else
chk_push(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    int instoper = ((Inst386 *) cp->info.code_inst)->opersize;
    int opersize = 0;

    switch( olp->olst_combo ){
	Ushort combo;
	Value *vp;
    case push_r16:	opersize = 2; break;
    case push_r32:	opersize = 4; break;
    case push_sreg:
	/* Avoid operand size check:  allow pushw/pushl. */
	switch( gen_getregno(olp->olst_first->oper_expr) ){
	case Reg_cs:	combo = push_cs; break;
	case Reg_ss:	combo = push_ss; break;
	case Reg_ds:	combo = push_ds; break;
	case Reg_es:	combo = push_es; break;
	case Reg_fs:	combo = push_fs; break;
	case Reg_gs:	combo = push_gs; break;
	}
	if (instoper == 2)
	    combo += push_sreg_w - push_sreg;	/* choose pushw versions */

	olp->olst_combo = combo;
	break;
    case push_imm8:
	/* Push word, rather than long, if instruction requires. */
	if (instoper == 2)
	    ++olp->olst_combo;

	/* Use long-form immediate if we know short-form won't fit. */
	vp = evalexpr(olp->olst_first->oper_expr);
	if (! ALWAYS_FITS_IN(vp, 8))
	    olp->olst_combo += push_imm - push_imm8;
	break;
    }
    if (opersize && opersize != instoper)
	invalidreg((Inst386 *) cp->info.code_inst, olp->olst_first);
    return;
}


/* Check shift family of instructions.  Make sure any register
** operand (for the second operand) is the same size as the
** instruction expects.  (Can't use IF_RSIZE flag because %cl
** would fail for word/long shifts.)
*/

void
#ifdef __STDC__
chk_shift(Code *cp)
#else
chk_shift(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *second = olp->olst_first->oper_next;

   if(second != 0){
	if (second->oper_expr->ex_type == ExpTy_Register) {
		Inst386 *ip3 = (Inst386 *)cp->info.code_inst;
		if(reginfo[gen_getregno(second->oper_expr)].r_size != ip3->opersize)
			invalidreg(ip3, second);
	}
	if (olp->olst_combo >= shft_imm_s && olp->olst_combo < shft_imm_e) {
		/* Must check whether the literal value is 1. */
		Value *vp = evalexpr(olp->olst_first->oper_expr);

	/* 3 byte format on 486 takes only 2 cycles */
	if(ALWAYS_FITS_IN(vp, 2) && vp->val_ulong == 1 && (strcmp(proc_type,"386") == 0))
		olp->olst_combo += shft_imm_1 - shft_imm_s;
	}
   }

    return;
}


/*************************************************
**						**
**	Routines to Generate Instructions	**
**						**
*************************************************/
/* In most cases, the binary encoding of an instruction is produced
** by gen_list(), for which an operand combination ("combo") has
** been stored in olst_combo.  Special code must be used for some
** instructions, however, to deal with special cases, too many
** operands, or combinations that gen_list() doesn't support.
*/

/* All the generate routines return the size of their encoding.
** If the code's section's sec_data field is non-zero, the actual
** binary encoding is stored in the appropriate place.  The calls
** that occur when sec_data is zero are from setvarsz(), to get
** the size of variable-size instructions.
*/


/* Generate code based on a genlist_t.  The list is pointed to
** by the Inst386, and the selected combination is in olst_combo,
** selected (usually) by chk_list().
**
** If the operand encoding has no size-varying components, the
** instruction's code_kind is changed to CodeKind_FixInst.  In
** any case, the size of the operand encoding is returned.
**
** Other assumptions:
**	1. If there are two operands:
**	  a) One is assumed to be "memory", the other "register".
**	  b) If one is a literal, it behaves like the "register" operand.
**	2. This routine works for single-operand instructions.  The
**		operand may be treated as memory, register, or both.
**
** These flags in the genlist_t affect processing:
**	GL_MEMRIGHT	treat the right (second) operand in the chkgen
**			tables as the "memory" operand
**	GL_PREFIX	generate 0x0F prefix byte
**	GL_FWAIT	generate "fwait" prefix byte
**	GL_OVERRIDE	generate (word) operand size override byte
**	GL_PLUS_R	operand is "+r" form:  add register code to opcode
**	GL_MOFFSET	memory operand is pure memory offset (no /r form)
**	GL_SLASH_R	generate /r-type operand:  'r' value is from register
**			operand
**	GL_SLASH_N	generate /n-type operand:  'n' value is from
**			genlist_t entry
**	GL_IMM		generate immediate operand whose size is that of
**			the instruction operand (i.e., 1 for byte, 2 for
**			word, 4 for long)
**	GL_IMM8		generate 8-bit immediate value (only)
**
**	GL_PCREL	generate PC-relative branch; the size, either 1 or
**			4-byte, comes from the getnlist_t entry.
**
** GL_SLASH_R, GL_SLASH_N, GL_MOFFSET, and GL_PCREL are mutually
** exclusive.
*/

    Uchar *p0;
size_t
#ifdef __STDC__
gen_list(Section *secp, Code *cp)
#else
gen_list(secp, cp) Section *secp; Code *cp;
#endif
{
    const genlist_t *glp =
	&((Inst386 *)cp->info.code_inst)->
		genlist[cp->data.code_olst->olst_combo];
    Uchar *p;
    Uchar dummy[20];		/* Room to store the encoding of the largest
    				** instruction.  Use this space instead of
				** secp->sec_data when the latter is 0 if
				** we're just getting the instruction's size,
				** rather than storing its bytes.
				*/
    Ushort flags = glp->gl_flags;
    Operand *regop;
    Operand *memop;
    int gen = secp->sec_data != 0;	/* non-zero if we're generating code */

    /* Select the register operand and the memory operand.  If
    ** there's only one operand, it serves as both.  If MEMRIGHT
    ** (which refers to the chkgen table) is set, the memory
    ** operand is on the right in the table (left or first in
    ** the operand list).  MEMRIGHT is never set for one-operand
    ** instructions.
    */
    if ((flags & GL_MEMRIGHT) == 0) {
	regop = cp->data.code_olst->olst_first;
	memop = regop->oper_next;
	/* If there's only one operand, treat it as both kinds. */
	if (memop == 0)
	    memop = regop;
    }
    else {
	memop = cp->data.code_olst->olst_first;
	regop = memop->oper_next;
    }

    /* Establish initial, running pointers to data. */
    p = p0 = gen ? secp->sec_data + cp->code_addr : &dummy[0];

    if (flags & GL_OVERRIDE)
	*p++ = WORD_code;

    if (memop->oper_flags & Amode_Segment)
	*p++ = gen_seg_override(memop);

    if (flags & (GL_PREFIX|GL_FWAIT)) {
	if (flags & GL_PREFIX)
	    *p++ = 0x0F;
	if (flags & GL_FWAIT)
	    *p++ = FWAIT_code;
    }

    *p = glp->gl_opcode;

    /* For +r, "memop" is really the register operand,
    ** because regop is a literal.
    */
    if (flags & GL_PLUS_R)
	*p += gen_regcode(memop->oper_expr);
    ++p;

    if (flags & (GL_SLASH_R|GL_SLASH_N)) {
	p += gen_slashr(
		    (flags & GL_SLASH_R) ? gen_regcode(regop->oper_expr) : glp->gl_slashn,
		    memop, secp, p);
    }
    else if (flags & GL_PCREL)
	/* glp->gl_slashn contains minimum PC-rel size. */
	p += gen_pcrel(regop, secp, glp->gl_slashn, cp->code_addr+(p-p0), 1);
    else if (flags & GL_MOFFSET) {
	/* moff[8/16/32] */
	if (gen) {
	    Value *vp = evalexpr(memop->oper_expr);

	    if (   (vp->val_flags & VAL_OFLOW) != 0
		|| vp->val_minbits > 32
	    )
		opererror("memory operand too big", memop);
	    else {
		/* relocaddr() can change *vp. */
		if ((vp->val_flags & VAL_RELOC) != 0)
		    relocaddr(vp, p, secp);
		gen_value(vp, 4, p);
	    }
	}
	p += 4;
	cp->code_kind = CodeKind_FixInst;	/* inst. has no varying size */
    }
    else
	cp->code_kind = CodeKind_FixInst;	/* inst. has no varying size */


    if (flags & (GL_IMM8|GL_IMM)) {
	/* GL_IMM8 takes precedence for imposing size. */
	int size = (flags & GL_IMM8) ?
		1 : ((Inst386 *)cp->info.code_inst)->opersize;
	if (gen)
	    (void) gen_lit(regop, size, secp, p);
	p += size;
    }
    return p - p0;
}


/* This routine generates no-operation code to fill space that
** results from a .align or padding.
**
** We use the following encodings.  Note that a 1-byte nop is
** somewhat expensive, at least on the 386 (3 clocks), and we
** try to avoid using it if possible.
**									Clocks
**	1-byte:	nop			0x90				 3
**	2-byte:	movl %eax,%eax		0x8B, 0xC0			 2
**	3-byte:	leal 0(%eax),%eax	0x8D, 0x40, 0			 2
**	4-byte:	leal 0(%eax),%eax	0x8D, 0x44, 0x20, 0		 2
**	5-byte: [use 3 + 2]
**	6-byte: leal 0(%eax),%eax	0x8D, 0x80, 0x00, 0, 0, 0	 2
**	7-byte: leal 0(,%eax,1),%eax	0x8D, 0x04, 0x05, 0, 0, 0, 0	 2
**	8-byte: [use 4 + 4]
*/

void
#ifdef __STDC__
gennops(Section *secp, register const Code *cp)	/* generate nops */
#else
gennops(secp, cp)Section *secp; register Code *cp;
#endif
{
    /* Generate number of NOP's in cp->data.code_skip */
    Ulong n = cp->data.code_skip;
    Uchar *p = secp->sec_data + cp->code_addr;

    while (n != 0) {
	int size;
	switch(size = n) {
	default:
	    size = 7;
	    /*FALLTHRU*/
	case 7:
	    p[0] = 0x8D; p[1] = 0x04; p[2] = 0x05;
	    p[3] = 0; p[4] = 0; p[5] = 0; p[6] = 0;
	    break;
	case 6:
	    p[0] = 0x8D; p[1] = 0x80;
	    p[2] = 0; p[3] = 0; p[4] = 0; p[5] = 0;
	    break;
	case 8:
	    size = 4;
	    /*FALLTHRU*/
	case 4:
	    p[0] = 0x8D; p[1] = 0x44; p[2] = 0x20; p[3] = 0;
	    break;
	case 5:
	    size = 3;
	    /*FALLTHRU*/
	case 3:
	    p[0] = 0x8D; p[1] = 0x40; p[2] = 0x00;
	    break;
	case 2:
	    p[0] = 0x8B; p[1] = 0xC0;
	    break;
	case 1:
	    p[0] = NOP_code;
	    break;
	}
	p += size;
	n -= size;
    }
    return;
}


/* Useful service routines. */


/* Get register number from expression. */

static int
#ifdef __STDC__
gen_getregno(Expr *ep)
#else
gen_getregno(ep) Expr *ep;
#endif
{
    if (ep == 0)
	fatal("gen_getregno(): no expression");
    if (ep->ex_op != ExpOp_LeafRegister)
	fatal("gen_getregno(): not a register");
    return( ep->right.ex_reg );
}

    
/* Return segment override byte for operand. */

static Uchar
#ifdef __STDC__
gen_seg_override(const Operand *op)
#else
gen_seg_override(op) Operand *op;
#endif
{
    if ((op->oper_flags & Amode_Segment) == 0)
	fatal("gen_seg_override(): no segment");
    
    return seg_reg[gen_regcode(op->oper_amode.seg)];
}


/* Generate encoding for a value. */
void
#ifdef __STDC__
gen_value(Value *vp, int size, Uchar *p)
#else
gen_value(vp, size, p) Value *vp; int size; Uchar *p;
#endif
{
    Ulong value = vp->val_ulong;

    /* Generate low-to-high order bytes in host-independent way.
    ** Assume CHAR_BIT is the same for host and target machines.
    */
    switch( size ) {
    default:	fatal("gen_value(): can't handle size %d", size);
		/*NOTREACHED*/
/* Select byte, from right to left; rely on implicit truncation. */
#define BYTE(i) ((Uchar) (value >> ((i) * CHAR_BIT)))
    case 4:	p[3] = BYTE(3);	/*FALLTHRU*/
    case 3:	p[2] = BYTE(2);	/*FALLTHRU*/
    case 2:	p[1] = BYTE(1);	/*FALLTHRU*/
    case 1:	p[0] = BYTE(0);	/*FALLTHRU*/
    }
    return;
}


/* Generate code for literal.  Return number of bytes generated.
** "size" is number of bytes for literal.  Accept either signed
** or unsigned numbers that fit the available space.
*/

static size_t
#ifdef __STDC__
gen_lit(Operand *op, int size, Section *secp, Uchar *p)
#else
gen_lit(op, size, secp, p)
Operand *op; int size; Section *secp; Uchar *p;
#endif
{
    Expr *ep;

    if ((ep = op->oper_expr) == 0)
	fatal("gen_lit():  no expr");

    switch (ep->ex_type) {
    default: fatal("gen_lit():  bad expr. type");
	/*NOTREACHED*/
    case ExpTy_Relocatable:
    case ExpTy_Integral:
	break;
    }

    /* Do the following checks only if we're actually generating
    ** code.  (This avoids duplicate work, and it suppresses
    ** duplicate diagnostics.)
    */
    if (p) {
	Value *vp = evalexpr(ep);

	/* Allow value if it fits as either signed or unsigned. */
	if (   (vp->val_flags & VAL_OFLOW) != 0
	    || (vp->val_minbits > size * CHAR_BIT)
	)
	    operwarn("literal value does not fit", op);

	if ((vp->val_flags & VAL_RELOC) != 0) {
	    if (size < 4)
		opererror("only 4-byte literal can be relocatable", op);
	    else
		/* relocaddr() can change *vp. */
		relocaddr(vp, p, secp);
	}
	if(vp->val_sym->sym_kind == SymKind_GOT)
		vp->val_ulong += p-p0;
	gen_value(vp, size, p);
    }
    return size;
}


/* Output various /r, /n forms of addressing.  For the purposes of
** this routine, one operand is considered a memory operand.  regcode
** is a register number (or alternate opcode value).  The operand
** may have an arbitrary memory (or register) addressing
** expression.  Store the encoding for output if memory pointer is non-null.
** Return the length of the whole affair in either case.
*/

static size_t
#ifdef __STDC__
gen_slashr(int regcode, Operand *memop, Section *secp, Uchar *p)
#else
gen_slashr(regcode, memop, secp, p)
int regcode; Operand *memop; Section *secp; Uchar *p;
#endif
{
    Expr *base = 0;
    Expr *index = 0;
    int scale = 0;
    Value *disp = 0;
    int dispsize = 0;
    Uchar rmbyte;			/* mod r/m byte */
    Ushort sib = 0;			/* SIB byte + flag bit*/
    int varies = 0;			/* non-zero if size could vary */
    Code *cp = memop->parent.oper_olst->olst_code;

    if (memop->oper_flags & Amode_BIS) {
	base = memop->oper_amode.base;
	index = memop->oper_amode.index;
	if (memop->oper_amode.scale)
	    /* Value has already been checked */
	    scale = evalexpr(memop->oper_amode.scale)->val_ulong;
    }
    /* Figure out how big a displacement we have. */
    if (memop->oper_expr) {
	switch( memop->oper_expr->ex_type ) {
	default: fatal("gen_slashr(): bad expr type");
	    /*NOTREACHED*/
	case ExpTy_Register:
	    dispsize = -1;		/* signal register */
	    break;
	case ExpTy_Integral:
	case ExpTy_Relocatable:
	    disp = evalexpr(memop->oper_expr);
	    if (secp->sec_data) {
		if (   (disp->val_flags & VAL_OFLOW) != 0
		    || disp->val_minbits > 32
		)
		    opererror("displacement too big", memop);
	    }
	    dispsize = 4;
	    if (memop->oper_expr->ex_type == ExpTy_Integral) {
#ifdef OLD_AS_COMPAT
		/* Duplicate old assembler's behavior:  discard
		** zero displacement if both base and index set.
		** If we find we need a displacement, a new one
		** will be constructed.
		*/
		if (base != 0 && index != 0 && disp->val_ulong == 0) {
		    disp = 0;
		    dispsize = 0;
		}
		else
#endif
		    if (FITS_IN(disp, 8)) {
			dispsize = 1;
			/* Size of operand could still vary (grow) if
			** it's the difference of two labels.
			*/
			if (disp->val_flags & VAL_LDIFF)
			    varies = 1;
		    }
	    }
	    if (dispsize < (int) cp->code_varsize)
		dispsize = cp->code_varsize;
	    cp->code_varsize = (Uchar) dispsize;
	    break;
	}
    }

    if ((memop->oper_flags & Amode_BIS) == 0) {
	if (dispsize < 0) {
	    /* "Memory" operand is actually register. */
	    rmbyte = gen_regcode(memop->oper_expr) | 0xC0;
	    dispsize = 0;
	}
	else {
	    dispsize = 4;		/* pure displacement is 4 bytes */
	    rmbyte = 0x5;
	}
    }
    else {
	/* Figure out cases where we need a SIB. */
	rmbyte = base ? gen_regcode(base) : 0;

	/* 4 and 5 (%esp and %ebp) are special cases because of
	** the encoding.  %esp gets handled via a SIB byte (which
	** code we fall into).  %ebp is special when there's no
	** displacement.  Also special is the case where there's
	** an index and no base or displacement.  Create a dummy
	** 0 displacement.  This shouldn't happen very often.
	*/
	if (   disp == 0
	    && (rmbyte == 5 || (base == 0 && index != 0))
	) {
	    disp = evalexpr(ulongexpr((Ulong) 0));
	    dispsize = 1;
	}
	if (index != 0 || rmbyte == 4) {
	    /* rmbyte == 4 is special case of %esp as base.
	    ** No base == 5.
	    */
	    if (base == 0)
		rmbyte = 0x5;
	    sib = (index ? gen_regcode(index) : 0x4) << 3;
	    switch( scale ){
	    case 2:	sib |= 0x40; break;
	    case 4:	sib |= 0x80; break;
	    case 8:	sib |= 0xC0; break;
	    }
	    sib |= rmbyte | 0x100;	/* so byte is never zero */
	    rmbyte = 0x4;		/* new value */
	}
	/* An index without a base always takes a disp32 and has
	** MOD == 0.
	*/
	if (index != 0 && base == 0)
	    dispsize = 4;
	else if (dispsize)
	    rmbyte |= (dispsize == 1 ? 0x40 : 0x80);
    }

    /* Add in /r part. */
    rmbyte |= regcode << 3;

    if (secp->sec_data) {
	*p++ = rmbyte | regcode << 3;
	if (sib)
	    *p++ = (Uchar) sib;
	if (disp) {
	    /* relocaddr() can change *disp. */
	    if ((disp->val_flags & VAL_RELOC) != 0)
		relocaddr(disp, p, secp);
	    gen_value(disp, dispsize, p);
	}
    }
    if (! varies)
	cp->code_kind = CodeKind_FixInst;
    return( 1 + (sib != 0) + dispsize );
}


/* Generate code for PC-relative operand.  PC-relative instructions
** have effective addresses relative to their end.  "offset" is the
** offset in the section (secp) of the start of the PC-relative part.
** "size" is the requested PC-relative size in bytes, presumably 
** 1 or 4.  If "gen" is zero, don't actually generate any code.
*/
static size_t
#ifdef __STDC__
gen_pcrel(Operand *op, Section *secp, int size, size_t offset, int gen)
#else
gen_pcrel(op, secp, size, offset, gen)
Operand *op; Section *secp; int size; size_t offset; int gen;
#endif
{
    Value *vp;
    int ispcrel = 1;			/* assume jump is PC-relative */
    Uchar *p;
    Code *cp = op->parent.oper_olst->olst_code;

    if (size < (int) cp->code_varsize)
	size = cp->code_varsize;	/* Don't let operand size shrink. */

    /* Determine the size of the PC-relative part.
    ** If the operand isn't a constant, or if it's in another
    ** section, or if it uses one of the PIC relocation types, we
    ** always require a 4-byte displacement.  The displacement is
    ** calculated relative to the end of the PC-relative part.
    */
    vp = evalexpr(op->oper_expr);
    if (   (vp->val_flags & VAL_RELOC) == 0
	|| vp->val_sec != secp
	|| vp->val_pic != 0
    ) {
	ispcrel = 0;
	/* Integral offset value must be adjusted to reflect relocation
	** relative to '.', rather than '.+4'.
	*/
	subvalue(vp, 4L);
	size = 4;
    }
    else {
	/* PC-relative displacement.  Figure out if a small
	** displacement will work.
	*/
	subvalue(vp, (Ulong) offset + size);
	if (vp->val_snbits > (size * CHAR_BIT)) {
	    /* Reevaluate expression, assuming large size, which changes
	    ** displacement value.
	    */
	    subvalue(vp, (Ulong) 4 - size);
	    size = 4;
	}
    }

    if ((vp->val_flags & VAL_OFLOW) != 0 || vp->val_snbits > 32)
	opererror("branch target too distant", op);

    if ((p = secp->sec_data) != 0 && gen) {
	/* Produce code and suitable relocation type. */
	p += offset;

	/* relocpcrel() can change *vp. */
	if (! ispcrel)
	    relocpcrel(vp, p, secp);
	gen_value(vp, size, p);
    }
    /* Instruction won't change size if it's big now. */
    if (size == 4)
	cp->code_kind = CodeKind_FixInst;

    cp->code_varsize = (Ushort) size;	/* remember size of variable portion */
    return size;
}


/* Generate code for long jmp to "target". */

static size_t
#ifdef __STDC__
gen_dojmp(Operand *target, Section *secp, Ulong off, int gen)
#else
gen_dojmp(target, secp, off, gen)
Operand *target; Section *secp; Ulong off; int gen;
#endif
{
    int opersize = gen_pcrel(target, secp, 1, off+1, 0);
    Uchar *p;

    if ((p = secp->sec_data) != 0 && gen) {
	p[off] = (opersize == 1 ? JMP8_code : 0xE9);
	(void) gen_pcrel(target, secp, opersize, off+1, 1);
    }
    return( opersize + 1 );
}


/* Generate code for clr variants.  gen_list() can handle
** register cases.  For memory cases, produce a suitable
** literal after using gen_list().
*/
size_t
#ifdef __STDC__
gen_clr(Section *secp, Code *cp)
#else
gen_clr(secp, cp) Section *secp; Code *cp;
#endif
{
    int instoff = gen_list(secp, cp);

    if (cp->data.code_olst->olst_combo >= clr_mem) {
	Value *vp = evalexpr(ulongexpr((Ulong) 0));	/* create a zero */
	int opersize = ((Inst386 *)cp->info.code_inst)->opersize;

	if (secp->sec_data != 0)
	    gen_value(vp, opersize, secp->sec_data + cp->code_addr + instoff);
	instoff += opersize;
    }
    return instoff;
}


/* Generate code for enter $iw,$ib */

size_t
#ifdef __STDC__
gen_ent(Section *secp, Code *cp)
#else
gen_ent(secp, cp) Section *secp; Code *cp;
#endif
{
    Uchar *p;
    Operand *left = cp->data.code_olst->olst_first;
    Operand *right = left->oper_next;

    if ((p = secp->sec_data) == 0)
	fatal("gen_ent(): called for size");
    
    p += cp->code_addr;

    *p++ = 0xC8;
    p += gen_lit(left, 2, secp, p);
    (void) gen_lit(right, 1, secp, p);
    return 4;				/* instruction size */
}


/* Check fxch.  There are 0- and 1-operand forms.  For the
** latter, use the standard check routine.
** chk_list() has not been called before we get here.
*/

void
#ifdef __STDC__
chk_fxch(Code *cp)
#else
chk_fxch(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;

    if (olp->olst_first != 0)
	chk_list(cp, cl_fld0);
    return;
}


/* Generate code for fxch.  If there's an operand, use gen_list().
** Otherwise (and this is painful), put out the explicit bits for
** fxch	%st(1).
*/

size_t
#ifdef __STDC__
gen_fxch(Section *secp, Code *cp)
#else
gen_fxch(secp, cp) Section *secp; Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Uchar *p;

    cp->code_kind = CodeKind_FixInst;	/* it's always a fixed size */

    if (olp->olst_first != 0)
	return gen_list(secp, cp);
    if ((p = secp->sec_data) != 0) {
	p += cp->code_addr;

	p[0] = 0xD9;
	p[1] = 0xC8 + 1;
    }
    return 2;
}


/* Check imul forms.  The idea is to recognize the form with
** the immediate operand and run the check on a different set
** of operands after removing the immediate.
** chk_list() has not been called before we get here.
*/

void
#ifdef __STDC__
chk_imul(Code *cp)
#else
chk_imul(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *first = olp->olst_first;

    if (first->oper_info != OC_LIT) {
	/* Simpler case:  just do the ordinary chk_list. */
	/* Only literal case can have 3 operands. */
	if (first->oper_next != 0 && first->oper_next->oper_next != 0)
	    invalidoper(cp);
	else
	    chk_list(cp, cl_imul);
    }
    else if (first->oper_next == 0)
	invalidoper(cp);		/* literal cases have more operands */
    else {
	Ushort combo;
	Value *vp;

	/* Temporarily remove the first operand, check against
	** forms that take one or two more operands, then restore
	** operand list.
	*/
	olp->olst_first = first->oper_next;
	chk_list(cp, &cl_imul[imul_imm8]);
	olp->olst_first = first;

	/* Get "combo" offset from start of entire cl_imul list. */
	combo = olp->olst_combo + imul_imm8-0;

	vp = evalexpr(first->oper_expr);

	/* Use large-form literal if value may change size or is
	** too big now.
	*/
	if (! ALWAYS_FITS_IN(vp, 8))
	    combo += imul_imm - imul_imm8;
	olp->olst_combo = combo;
    }
    return;
}


/* Generate code for imul.  For combinations that don't involve
** an immediate, just generate the code.  Otherwise, remove the
** immediate operand (as in chk_imul), generate the code,
** generate the immediate operand, the restore the immediate.
*/

size_t
#ifdef __STDC__
gen_imul(Section *secp, Code *cp)
#else
gen_imul(secp, cp) Section *secp; Code *cp;
#endif
{
    Ushort combo = cp->data.code_olst->olst_combo;
    int instoff;

    if (combo < imul_imm8)
	instoff = gen_list(secp, cp);
    else {
	Oplist *olp = cp->data.code_olst;
	Operand *first = olp->olst_first;
	int size;

	olp->olst_first = first->oper_next;
	instoff = gen_list(secp, cp);		/* generate most of inst. */
	olp->olst_first = first;		/* restore operand list */
	if (combo < imul_imm)
	    size = 1;				/* small form */
	else
	    size = ((Inst386 *) cp->info.code_inst)->opersize;
	if (secp->sec_data)
	    (void) gen_lit(first, size, secp,
			secp->sec_data + cp->code_addr + instoff);
	instoff += size;
    }
    return instoff;
}


/* Check unconditional jump and call.  A '*' is required if we're
** doing any base/index/displacement addressing or if the operand
** is a register.
*/

void
#ifdef __STDC__
chk_jmp(Code *cp)
#else
chk_jmp(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *op = olp->olst_first;

    if (   (op->oper_flags & Amode_Indirect) == 0
	&& (
		(op->oper_flags & Amode_BIS) != 0
	    ||  (op->oper_expr->ex_type == ExpTy_Register)
	    )
    ) {
	operwarn("'*' required for address mode", op);
	op->oper_flags |= Amode_Indirect;
    }
    /* If the expression doesn't look like something that can be
    ** handled by PC-relative addressing, use straight mod r/m mode.
    */
    if (   olp->olst_combo == jmp_pcr8
	&& (op->oper_flags & (Amode_BIS|Amode_Indirect)) != 0
    )
	olp->olst_combo = jmp_mem;
    return;
}


/* Generate code for jmp. */
size_t
#ifdef __STDC__
gen_jmp(Section *secp, Code *cp)
#else
gen_jmp(secp, cp) Section *secp; Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *dst = olp->olst_first;
    int instoff;

    /* If we still think we can use a 1-byte PC-relative, check and
    ** possibly make it 4-byte.
    */
    if (olp->olst_combo == jmp_pcr8) {
	/* addr+1 for opcode */
	if (gen_pcrel(dst, secp, 1, cp->code_addr+1, 0) > 1)
	    olp->olst_combo = jmp_pcr32;
    }
    instoff = gen_list(secp, cp);

#ifdef OLD_AS_COMPAT
    /* Old assembler put NOP's after jmp's with 4-byte PC-relative address. */
    if (olp->olst_combo == jmp_pcr32) {
	Uchar *p;

	if ((p = secp->sec_data) != 0)
	    p[cp->code_addr+instoff] = NOP_code;
	++instoff;
    }
#endif
    return instoff;
}


/* Check long unconditional jump and call.  A '*' is required if we're
** doing any base/index/displacement addressing or if the operand
** is a register, but only for the r/m operand case (not two literals).
*/

/*ARGSUSED*/
void
#ifdef __STDC__
chk_ljmp(Code *cp)
#else
chk_ljmp(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *op = olp->olst_first;

    if (   olp->olst_combo >= cl_ljmp_rm
        && (op->oper_flags & Amode_Indirect) == 0
	&& (
		(op->oper_flags & Amode_BIS) != 0
	    ||  (op->oper_expr->ex_type == ExpTy_Register)
	    )
    ) {
	operwarn("'*' required for address mode", op);
	op->oper_flags |= Amode_Indirect;
    }
    return;
}


/* Generate code for ljmp/lcall.  Combination 0 is special:
** two literals.
*/

size_t
#ifdef __STDC__
gen_ljmp(Section *secp, Code *cp)
#else
gen_ljmp(secp, cp) Section *secp; Code *cp;
#endif
{
    Uchar *p;

    if (cp->data.code_olst->olst_combo >= cl_ljmp_rm)
	return gen_list(secp,cp);
    
    /* Two literals case. */
    if ((p = secp->sec_data) != 0) {
	Operand *op = cp->data.code_olst->olst_first;
	p += cp->code_addr;

	*p++ = ((Inst386 *)cp->info.code_inst)->code[0];
	p += gen_lit(op->oper_next, 4, secp, p);
	(void) gen_lit(op, 2, secp, p);
    }
    cp->code_kind = CodeKind_FixInst;	/* size completely known */
    return 1 + 4 + 2;
}



/* Generate code for instructions that take no operands.  These
** have fixed instruction encodings, which we can just plop down
** in the section.
*/

size_t
#ifdef __STDC__
gen_nop(Section *secp, Code *cp)
#else
gen_nop(secp, cp) Section *secp; Code *cp;
#endif
{
    int nbytes = cp->info.code_inst->inst_minsz;
    Uchar *ip = ((Inst386 *) cp->info.code_inst)->code;
    Uchar *p;

    if ((p = secp->sec_data) == 0)
	fatal("gen_nop(): called to get size");

    p += cp->code_addr;

    p[0] = ip[0];
    if (nbytes > 1) {
	p[1] = ip[1];
	if (nbytes > 2)
	    p[2] = ip[2];
    }
    return nbytes;
}


/* Generate code for instructions that optionally take no operands.  These
** have fixed instruction encodings, which we can just plop down
** in the section.
*/

size_t
#ifdef __STDC__
gen_optnop(Section *secp, Code *cp)
#else
gen_optnop(secp, cp) Section *secp; Code *cp;
#endif
{
    int nbytes = cp->info.code_inst->inst_minsz;
    Uchar *ip = ((Inst386 *) cp->info.code_inst)->code;
    Uchar *p;

    /* first check if the instruction has any operands.  If not, the encoding
     * and size comes from instree and is handled as a simple case.
     */

    if( cp->data.code_olst->olst_first == 0){
	if ((p = secp->sec_data) == 0){
		cp->code_kind = CodeKind_FixInst;
		return nbytes;	/* just checking for size */
	}else{
		p += cp->code_addr;

		p[0] = ip[0];
		if (nbytes > 1) {
		p[1] = ip[1];
		if (nbytes > 2)
		p[2] = ip[2];
		}
		return nbytes;
	}
   }else{	/* must have operands this time */
	return(gen_list(secp,cp));
   }
}


/* Check correctness of PC-relative branches. */

void
#ifdef __STDC__
chk_pcr(Code *cp)
#else
chk_pcr(cp) Code *cp;
#endif
{
    Operand *first = cp->data.code_olst->olst_first;

    if (first->oper_flags & Amode_Segment)
	operwarn("segment override ineffective with conditional branch", first);

    /* Allow any memory operand that doesn't have indirection or
    ** base/index/scale.
    */
    if (   first->oper_info != OC_MEM
	|| (first->oper_flags & (Amode_Indirect|Amode_BIS)) != 0
	)
	invalidoper(cp);
    return;
}


/* Generate code for most PC-relative instructions.  This group all
** has a short (byte) and long (32-bit) displacement form.  The short
** form code is in the instruction table, and the long form requires
** an escape byte, followed by the short form + 0x10.
*/

size_t
#ifdef __STDC__
gen_pcr(Section *secp, Code *cp)
#else
gen_pcr(secp, cp) Section *secp; Code *cp;
#endif
{
    Uchar *p;
    Operand *target = cp->data.code_olst->olst_first;
    Uchar opcode = ((Inst386 *) cp->info.code_inst)->code[0];
    int instoff = 0;
    int opersize;

    if ((p = secp->sec_data) != 0)
	p += cp->code_addr;

#if 0	/* segment override has no effect */
    if ((target->oper_flags & Amode_Segment) != 0 && p)
	p[instoff++] = gen_seg_override(target, p);
#endif

    /* Get size of operand; minimum size is 1 byte.  Allow for 1-byte
    ** opcode.  Don't generate anything yet.
    */
    opersize = gen_pcrel(target, secp, 1, cp->code_addr+instoff+1, 0);

    /* Now lay down the code. */
    if (p) {
	if (opersize == 1) {
	    p[instoff] = opcode;
	    ++instoff;
	}
	else {
	    p[instoff+0] = 0x0F;
	    p[instoff+1] = opcode + 0x10;
	    instoff += 2;
	}
        instoff += gen_pcrel(target, secp, opersize, instoff + cp->code_addr, 1);
    }
    else
	instoff += 1 + (opersize != 1) + opersize;
    return( instoff );
}

/* Generate code for PC-relative instructions that only have a byte
** displacement.  The instruction code is in the table.
** The long form is:
**		INST <lab0>
**		jmp <lab1>
**	<lab0:>
**		jmp dest
**	<lab1:>
*/

size_t
#ifdef __STDC__
gen_pc8(Section *secp, Code *cp)
#else
gen_pc8(secp, cp) Section *secp; Code *cp;
#endif
{
    Uchar *p;
    Operand *target = cp->data.code_olst->olst_first;
    Uchar opcode = ((Inst386 *) cp->info.code_inst)->code[0];
    int instoff = 0;
    int opersize;
    Uchar jmplen;

    if ((p = secp->sec_data) != 0)
	p += cp->code_addr;

#if 0	/* segment override has no effect */
    if ((target->oper_flags & Amode_Segment) != 0 && p)
	p[instoff++] = gen_seg_override(target, p);
#endif

    /* Get size of operand; minimum size is 1 byte.  Allow for 1-byte
    ** opcode.  Don't generate anything yet.
    */
    opersize = gen_pcrel(target, secp, 1, cp->code_addr+instoff+1, 0);

    if (opersize == 1) {
	/* Easy case:  short form. */
	if (p)
	    p[instoff] = opcode;
	++instoff;

	instoff +=
	    gen_pcrel(target, secp, opersize, instoff + cp->code_addr, p ? 1 : 0);
    }
    else {
	/* need jmp around jmp around jmp */
	instoff += 4;
	jmplen =
	    gen_dojmp(target, secp, instoff + cp->code_addr, p ? 1 : 0);
	if (p) {
	    p[instoff-4] = opcode;
	    p[instoff-3] = 2;
	    p[instoff-2] = JMP8_code;
	    p[instoff-1] = jmplen;
#ifdef OLD_AS_COMPAT
	    p[instoff+jmplen] = NOP_code;
	}
	++instoff;
#else
	}
#endif
	instoff += jmplen;
    }
    return( instoff );
}


/* Check ret instruction.
** chk_list() has not been called for this instruction.
*/

void
#ifdef __STDC__
chk_ret(Code *cp)
#else
chk_ret(cp) Code *cp;
#endif
{
    Operand *op = cp->data.code_olst->olst_first;

    if (op && op->oper_info != OC_LIT)
	invalidoper(cp);
    return;
}



/* Generate code for "ret".  Depends on whether there's an
** operand.  With-operand form is opcode from table - 1.
*/

size_t
#ifdef __STDC__
gen_ret(Section *secp, Code *cp)
#else
gen_ret(secp, cp) Section *secp; Code *cp;
#endif
{
    Operand *op = cp->data.code_olst->olst_first;
    Uchar *p;

    cp->code_kind = CodeKind_FixInst;	/* size is fixed, regardless */

    if ((p = secp->sec_data) != 0)
	p += cp->code_addr;

    if (op == 0) {
	if (p)
	    p[0] = ((Inst386 *)cp->info.code_inst)->code[0];
	return 1;
    }
    /* Case when there's a literal. */
    if (p) {
	*p++ = ((Inst386 *)cp->info.code_inst)->code[0] - 1;
	(void) gen_lit(op, 2, secp, p);
    }
    return 3;
}


/* Check double-shift instructions.  If there are three operands,
** the first must be a literal.  The assembler syntax implies that
** the two-operand form has an implicit %cl.
** chk_list() has not been called for these instructions.
*/

void
#ifdef __STDC__
chk_shxd(Code *cp)
#else
chk_shxd(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *first = olp->olst_first;
    int imm_form = 0;			/* 1 if $imm8 form */

    if (first->oper_next->oper_next != 0) {
	imm_form = 1;
	if (first->oper_info != OC_LIT)
	    invalidoper(cp);
    }

    /* If $imm8 form, strip first operand, check others, restore. */
    if (imm_form)
	olp->olst_first = first->oper_next;
    chk_list(cp, cl_shxd);
    if (imm_form) {
	olp->olst_first = first;
	/* Choose different encoding for $imm8 form. */
	olp->olst_combo += cl_shxd_imm8 - 0;
    }
    return;
}


/* Generate code for double-size shift.  If 2-operand form,
** strip off first operand temporarily, use gen_list(),
** generate appropriate code for $imm8 if necessary.
*/

size_t
#ifdef __STDC__
gen_shxd(Section *secp, Code *cp)
#else
gen_shxd(secp, cp) Section *secp; Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *first = olp->olst_first;
    int imm_form = olp->olst_combo >= cl_shxd_imm8;
    int instoff;
    
    if (imm_form)
	olp->olst_first = first->oper_next;
    instoff = gen_list(secp, cp);
    if (imm_form)
	olp->olst_first = first;

    if (imm_form) {
	/* is $imm8 form */
	Uchar *p;

	if ((p = secp->sec_data) != 0) {
	    p += cp->code_addr + instoff;

	    (void) gen_lit(first, 1, secp, p);
	}
	++instoff;
    }
    return instoff;
}


/* Check string instructions:  movs/smov/slod/scmp/scas/ssto/ins/outs.
** If there's an operand, it must be a segment register.
** Turn the apparent register operand into one that just
** has a segment override (and nothing else).  This allows
** us to use gen_segment_override later.
*/

void
#ifdef __STDC__
chk_str(Code *cp)
#else
chk_str(cp) Code *cp;
#endif
{
    Operand *op = cp->data.code_olst->olst_first;

    if (op->oper_info != OC_SEG)
	invalidoper(cp);
    else {
	/* Move the segment register into the segment override position.
	** That way we can use gen_seg_override() ultimately.
	*/
	if (cp->info.code_inst->chkflags & IF_NOSEG)
	    operwarn("segment override ineffective for instruction", op);
	op->oper_flags |= Amode_Segment;
	op->oper_amode.seg = op->oper_expr;
    }
    return;
}


/* Generate code for string instructions:
** movs/smov/slod/scmp/scas/ssto/ins/outs.
** There may be a segment override, a size override, and an opcode.
*/

size_t
#ifdef __STDC__
gen_str(Section *secp, Code *cp)
#else
gen_str(secp, cp) Section *secp; Code *cp;
#endif
{
    Inst386 *ip3 = (Inst386 *) cp->info.code_inst;
    Operand *op = cp->data.code_olst->olst_first;
    Uchar *p, *p0;
    Uchar dummy[3];			/* in case not really generating */

    p = p0 = secp->sec_data ? secp->sec_data + cp->code_addr : &dummy[0];

    cp->code_kind = CodeKind_FixInst;	/* fixed size hereafter */

    if (op && (op->oper_flags & Amode_Segment))
	*p++ = gen_seg_override(op);
    
    if (ip3->opersize == 2)
	*p++ = WORD_code;
    *p++ = ip3->code[0];

    return p - p0;
}
