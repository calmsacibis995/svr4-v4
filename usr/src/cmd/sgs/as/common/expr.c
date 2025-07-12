/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/expr.c	1.7"
/*
* common/expr.c - common assembler expressions
*/
#include <stdio.h>
#include "common/as.h"
#include "common/expr.h"
#include "common/nums.h"
#include "common/sect.h"
#include "common/stmt.h"
#include "common/syms.h"

char *
#ifdef __STDC__
exopstr(int op)	/* return printable version for expression operator */
#else
exopstr(op)int op;
#endif
{
	/* for now: should be imported from implementation? */
	static const char *const opstr[ExpOp_TOTAL] = /* in ExpOp_* order */
	{
		"<reg>", "<name>",   "<str>",	"<num>", "<flt>", "<dot>",
		"*",	 "/",	      "%",
		"+",	 "-",
		"&",	 "|",	      "^",	"!",
		"<<",	 ">>",
		"<max>", "<lcm>",
		"~",	 "(unary) +", "(unary) -",
		"<s>",	 "<o>",				/* i386 only */
		"@l",	 "@h",	      "@ha",		/* i860 only */
		"@PC",	 "@GOT",      "@PLT",	"@GOTOFF",
	};
	static char buf[32];	/* assumed big enough */

	if (op >= 0 && op <= ExpOp_TOTAL)
		return (char *)opstr[op];
	(void)sprintf(buf, "<op%d>", op);
	return buf;
}

static Expr *
#ifdef __STDC__
newexpr(void)	/* allocate new Expr */
#else
newexpr()
#endif
{
	static Expr *avail, *endavail;
	register Expr *ep;

#ifndef ALLOC_EXPR_CHUNK
#define ALLOC_EXPR_CHUNK 5000
#endif
	if ((ep = avail) == endavail)
	{
#ifdef DEBUG
		if (DEBUG('a') > 0)
		{
			static Ulong total;

			(void)fprintf(stderr, "Total Exprs=%lu @%lu ea.\n",
				total += ALLOC_EXPR_CHUNK,
				(Ulong)sizeof(Expr));
		}
#endif
		avail = ep = (Expr *)alloc((void *)0,
			ALLOC_EXPR_CHUNK * sizeof(Expr));
		endavail = ep + ALLOC_EXPR_CHUNK;
	}
	avail = ep + 1;
	/*
	* left, right, ex_op and ex_type set by newexpr() callers.
	*/
	ep->ex_cont = Cont_Unknown;
	ep->parent.ex_ptr = 0;
	ep->ex_mods = 0;
	return ep;
}

Expr *
#ifdef __STDC__
regexpr(int regno)	/* build expression from register */
#else
regexpr(regno)int regno;
#endif
{
	register Expr *ep;

#ifdef DEBUG
	if (DEBUG('e') > 0)
		(void)fprintf(stderr, "regexpr(regno=%d)\n", regno);
#endif
	ep = newexpr();
	ep->ex_op = ExpOp_LeafRegister;
	ep->left.ex_ptr = 0;
	ep->right.ex_reg = regno;
	ep->ex_type = ExpTy_Register;
	return ep;
}

Expr *
#ifdef __STDC__
idexpr(const Uchar *str, size_t len)	/* build expression from name */
#else
idexpr(str, len)Uchar *str; size_t len;
#endif
{
	register Expr *ep;
	register Symbol *sp;

#ifdef DEBUG
	if (DEBUG('e') > 0)
		(void)fprintf(stderr, "idexpr(%.*s)\n", len, str);
#endif
	ep = newexpr();
	ep->ex_op = ExpOp_LeafName;
	sp = lookup(str, len);
	ep->right.ex_sym = sp;
	ep->ex_type = sp->sym_exty;
	ep->ex_mods = sp->sym_mods;
	ep->left.ex_ptr = sp->sym_uses;
	sp->sym_uses = ep;
	return ep;
}

Expr *
#ifdef __STDC__
strexpr(const Uchar *str, size_t len)	/* build expression from string */
#else
strexpr(str, len)Uchar *str; size_t len;
#endif
{
	register Expr *ep;

#ifdef DEBUG
	if (DEBUG('e') > 0)
		(void)fprintf(stderr, "strexpr(%s)\n", prtstr(str, len));
#endif
	ep = newexpr();
	ep->ex_op = ExpOp_LeafString;
	ep->left.ex_len = len;
	ep->right.ex_str = str;	   /* savestr() done later, if needed */
	ep->ex_type = ExpTy_String;
	return ep;
}

Expr *
#ifdef __STDC__
numexpr(const Uchar *str, size_t len)	/* build expr from number token */
#else
numexpr(str, len)Uchar *str; size_t len;
#endif
{
	register Expr *ep;

#ifdef DEBUG
	if (DEBUG('e') > 0)
		(void)fprintf(stderr, "numexpr(%.*s)\n", len, str);
#endif
	ep = newexpr();
	ep->ex_op = ExpOp_LeafNumber;
	ep->left.ex_ptr = 0;
	ep->right.ex_num = mknum(str, len);
	ep->ex_type = ExpTy_Integral;
	return ep;
}

Expr *
#ifdef __STDC__
ulongexpr(Ulong val)	/* build expression from Ulong */
#else
ulongexpr(val)Ulong val;
#endif
{
	register Expr *ep;

	ep = newexpr();
	ep->ex_op = ExpOp_LeafNumber;
	ep->left.ex_ptr = 0;
	ep->right.ex_num = newnum(ulong2num(val), val);
	ep->ex_type = ExpTy_Integral;
	return ep;
}

Expr *
#ifdef __STDC__
fltexpr(const Uchar *str, size_t len)	/* build expr from floating num */
#else
fltexpr(str, len)Uchar *str; size_t len;
#endif
{
	register Expr *ep;

#ifdef DEBUG
	if (DEBUG('e') > 0)
		(void)fprintf(stderr, "fltexpr(%.*s)\n", len, str);
#endif
	ep = newexpr();
	ep->ex_op = ExpOp_LeafFloat;
	ep->left.ex_ptr = 0;
	ep->right.ex_flt = savestr(str, len);
	ep->ex_type = ExpTy_Floating;
	return ep;
}

Expr *
#ifdef __STDC__
dotexpr(Section *secp)	/* build expression from code (for dot's value) */
#else
dotexpr(secp)Section *secp;
#endif
{
	register Expr *ep;

#ifdef DEBUG
	if (DEBUG('e') > 0)
	{
		(void)fprintf(stderr, "dotexpr(%s)\n",
			secp->sec_sym->sym_name);
	}
#endif
	ep = newexpr();
	ep->ex_op = ExpOp_LeafCode;
	ep->left.ex_sect = secp;
	ep->right.ex_code = secp->sec_last;
	ep->ex_type = ExpTy_Relocatable;
	return ep;

}

void		/* set *fp and *lp to where expression originated */
#ifdef __STDC__
exprfrom(Ulong *fp, Ulong *lp, register const Expr *ep)
#else
exprfrom(fp, lp, ep)Ulong *fp, *lp; register Expr *ep;
#endif
{
	/*
	* Only called just prior to error message print.
	*/
	for (;;)	/* only loop for Cont_Expr */
	{
		switch (ep->ex_cont)
		{
		default:
			fatal("exprfrom():unknown context: %d",
				ep->ex_cont);
			/*NOTREACHED*/
		case Cont_Unknown:	/* still being built */
			*fp = curfileno;
			*lp = curlineno;
			return;
		case Cont_Expr:
			ep = ep->parent.ex_ptr;
			continue;
		case Cont_Set:
			if (!ep->parent.ex_oper->oper_setsym)
				fatal("exprfrom():not .set sym w/Cont_Set");
			*fp = ep->parent.ex_oper->parent.oper_sym->sym_file;
			*lp = ep->parent.ex_oper->parent.oper_sym->sym_line;
			return;
		case Cont_Label:
			*fp = ep->parent.ex_sym->sym_file;
			*lp = ep->parent.ex_sym->sym_line;
			return;
		case Cont_Operand:
			if (ep->parent.ex_oper->oper_setsym)
				fatal("exprfrom():.set sym w/Cont_Operand");
			if (ep->parent.ex_oper->parent.oper_olst == 0)
			{
				*fp = curfileno;
				*lp = curlineno;
				return;
			}
			else
			{
				*fp = ep->parent.ex_oper->
					parent.oper_olst->olst_file;
				*lp = ep->parent.ex_oper->
					parent.oper_olst->olst_line;
			}
			return;
		}
	}
}

void
#ifdef __STDC__
exprtype(register Expr *ep)	/* check and set expression type(s) */
#else
exprtype(ep)register Expr *ep;
#endif
{
	static const char MSGexp[] = "invalid expression operand type, op %s";
	static const char MSGopr[] = "invalid operand expression type";
	register Expr *el, *er;
	register int result;
	int complain = 0;
	Symbol *sp;
	Operand *op;
	Oplist *olp;

	/*
	* Check for type mismatches and set the resulting type
	* for each expression, propagating the results upward,
	* if they differ from the current version.  Recursively
	* set all expressions that depend on an identifier, if
	* an ExpOp_LeafName is reached.  If the usage context
	* of an expression is a .set symbol, then continue to
	* propagate types if the symbol's expression type changes.
	*
	* This function is called (for the top of the tree) while
	* the expression is being built, and when a symbol's type
	* changes in the symbol table.  In this case, the operator
	* is ExpOp_LeafName, and all the expressions that make use
	* of the same name will be handled by the recursive call.
	*/
	for (;;)
	{
		el = ep->left.ex_ptr;	/* used by all valid cases */
		result = ExpTy_Unknown;	/* by default */
		switch (ep->ex_op)
		{
		default:
			fatal("exprtype():unknown operator: %s",
				exopstr((int)ep->ex_op));
			/*NOTREACHED*/
		case ExpOp_LeafRegister:
		case ExpOp_LeafString:
		case ExpOp_LeafNumber:
		case ExpOp_LeafFloat:
		case ExpOp_LeafCode:
			fatal("exprtype():inappropriate leaf operator: %s",
				exopstr((int)ep->ex_op));
			/*NOTREACHED*/
		case ExpOp_LeafName:
			/*
			* Only here when the type for a symbol is
			* determined.  Propagate the type to all
			* expressions that make use of the symbol.
			*/
			if (el != 0)
				exprtype(el);	/* fix next expr's types */
			result = ep->right.ex_sym->sym_exty;
			ep->ex_mods = ep->right.ex_sym->sym_mods;
			break;
		case ExpOp_Multiply:
		case ExpOp_Divide:
		case ExpOp_Remainder:
		case ExpOp_And:
		case ExpOp_Or:
		case ExpOp_Xor:
		case ExpOp_Nand:
		case ExpOp_LeftShift:
		case ExpOp_RightShift:
		case ExpOp_Maximum:
		case ExpOp_LCM:
			/*
			* Most binary operators only allow
			* integral left and right types and
			* produce integral results.
			*/
			er = ep->right.ex_ptr;
			if (el->ex_type == ExpTy_Integral)
			{
				if (er->ex_type == ExpTy_Integral)
					result = ExpTy_Integral;
				else if (er->ex_type != ExpTy_Unknown)
					complain = 1;
			}
			else if (er->ex_type != ExpTy_Integral
				&& er->ex_type != ExpTy_Unknown)
			{
				complain = 1;
			}
			break;
		case ExpOp_Add:
		case ExpOp_Subtract:
			/*
			* The combinations are more complex for binary +/-:
			*
			*		RIGHT			KEY
			*   -\+ | int | rel | unk |other    i - integral
			*   ----+-----+-----+-----+-----    r - relocatable
			* L int | i\i | e\r | u\u | e\e	    u - unknown
			* E ----+-----+-----+-----+-----    e - error
			* F rel | r\r | i\e | u\u | e\e
			* T ----+-----+-----+-----+-----
			*   unk | u\u | u\u | u\u | e\e
			*   ----+-----+-----+-----+-----
			*  other| e\e | e\e | e\e | e\e
			*/
			er = ep->right.ex_ptr;
			switch (er->ex_type)
			{
			case ExpTy_Integral:
				if (el->ex_type != ExpTy_Relocatable
					&& el->ex_type != ExpTy_Integral
					&& el->ex_type != ExpTy_Unknown)
				{
					complain = 1;
				}
				else
					result = el->ex_type;
				break;
			case ExpTy_Relocatable:
				if (ep->ex_op == ExpOp_Add)
				{
					if (el->ex_type == ExpTy_Integral)
						result = ExpTy_Relocatable;
					else if (el->ex_type != ExpTy_Unknown)
						complain = 1;
				}
				else if (el->ex_type == ExpTy_Relocatable)
				{
					result = ExpTy_Integral;
					/*
					* Both sides must either have no
					* modifiers, or be EXPMOD_GOTOFF.
					*/
					if (el->ex_mods != er->ex_mods
						|| (el->ex_mods != 0 &&
						el->ex_mods != EXPMOD_GOTOFF))
					{
						complain = 1;
					}
				}
				else if (el->ex_type != ExpTy_Unknown)
					complain = 1;
				break;
			case ExpTy_Unknown:
				if (el->ex_type != ExpTy_Relocatable
					&& el->ex_type != ExpTy_Integral
					&& el->ex_type != ExpTy_Unknown)
				{
					complain = 1;
				}
				break;
			default:
				complain = 1;
				break;
			}
			/*
			* If the result is relocatable, then the relocatable
			* operand must either be only EXPMOD_GOTOFF or 0.
			*/
			if (result == ExpTy_Relocatable)
			{
				if (el->ex_type == ExpTy_Relocatable)
				{
					if (el->ex_mods != EXPMOD_GOTOFF
						&& el->ex_mods != 0)
					{
						complain = 1;
					}
					else
						ep->ex_mods = el->ex_mods;
				}
				else	/* right is relocatable */
				{
					if (er->ex_mods != EXPMOD_GOTOFF
						&& er->ex_mods != 0)
					{
						complain = 1;
					}
					else
						ep->ex_mods = er->ex_mods;
				}
			}
			break;
		case ExpOp_UnaryAdd:
		case ExpOp_Negate:
			/*
			* Unary + and - allow floating and integral types.
			*/
			if (el->ex_type != ExpTy_Floating
				&& el->ex_type != ExpTy_Integral
				&& el->ex_type != ExpTy_Unknown)
			{
				complain = 1;
			}
			else
				result = el->ex_type;
			break;
		case ExpOp_SegmentPart:	/* i386 only */
		case ExpOp_OffsetPart:	/* i386 only */
			fatal("exprtype():<s>,<o> not implemented");
			/*NOTREACHED*/
		case ExpOp_Complement:
			/*
			* Unary ~ only allows integral type.
			*/
			if (el->ex_type == ExpTy_Integral)
				result = ExpTy_Integral;
			else if (el->ex_type != ExpTy_Unknown)
				complain = 1;
			break;
		case ExpOp_LowMask:		/* i860 only */
		case ExpOp_HighMask:		/* i860 only */
		case ExpOp_HighAdjustMask:	/* i860 only */
			/*
			* @l, @h, and @ha allow both integral and
			* relocatable types.  However, at most one
			* of these operations can be done to a
			* relocatable type.
			*/
			switch (el->ex_type)
			{
			case ExpTy_Relocatable:
				if (el->ex_mods & EXPMOD_MASK)
					complain = 1;
				ep->ex_mods = el->ex_mods | EXPMOD_MASK;
				/*FALLTHROUGH*/
			case ExpTy_Integral:
			case ExpTy_Unknown:
				result = el->ex_type;
				break;
			default:
				complain = 1;
				break;
			}
			break;
		case ExpOp_Pic_PC:
		case ExpOp_Pic_GOT:
		case ExpOp_Pic_PLT:
			/*
			* These PIC operators can only be applied to
			* relocatable types.  No modifiers can have
			* been applied to the expression.
			*/
			ep->ex_mods = EXPMOD_PIC;
		pictype:;
			if (el->ex_mods != 0)
				complain = 1;
			if (el->ex_type == ExpTy_Relocatable)
				result = ExpTy_Relocatable;
			else if (el->ex_type != ExpTy_Unknown)
				complain = 1;
			break;
		case ExpOp_Pic_GOTOFF:
			/*
			* GOTOFF differs from the other PIC operators
			* in that it permits further arithmetic except
			* for the difference of two relocatables with
			* only one having GOTOFF.  No other modifiers
			* can have been applied to the expression.
			*/
			ep->ex_mods = EXPMOD_GOTOFF;
			goto pictype;
		}
		if (complain)	/* issue diagnostic */
		{
			exprerror(ep, MSGexp, exopstr((int)ep->ex_op));
			complain = 0;
		}
		if (ep->ex_type == result)
			return;		/* no use propagating further */
		ep->ex_type = result;
		switch (ep->ex_cont)
		{
		default:
			fatal("exprtype():unknown context: %u",
				(Uint)ep->ex_cont);
			/*NOTREACHED*/
		case Cont_Label:
			fatal("exprtype():at label context");
			/*NOTREACHED*/
		case Cont_Expr:
			ep = ep->parent.ex_ptr;
			break;
		case Cont_Unknown:
			return;		/* expression still being built */
		case Cont_Set:
			/*
			* Update expression type for .set symbol.
			* Propagate the new type to all dependent
			* expressions, if any.
			*/
			op = ep->parent.ex_oper;
			if (!op->oper_setsym)
				fatal("exprtype():not .set w/Cont_Set");
			sp = op->parent.oper_sym;
			if (sp->sym_exty != ExpTy_Unknown)
				return;		/* already handled */
			if (op->oper_flags != 0)
				result = extyamode(op);
			if (result == ExpTy_Unknown)
				return;		/* don't bother */
			sp->sym_exty = result;
			sp->sym_mods = ep->ex_mods;
			if ((ep = sp->sym_uses) == 0)
				return;
			break;
		case Cont_Operand:
			/*
			* Check expected type, if any, for the operand.
			* Otherwise, if it is an instruction operand,
			* check it out.
			*/
			op = ep->parent.ex_oper;
			if (op->oper_tybits != 0)
			{
				if (!(op->oper_tybits & BIT(result)))
				{
					if ((olp = op->parent.oper_olst) == 0)
						fatal("exprtype():olp==0");
					backerror((Ulong)olp->olst_file,
						olp->olst_line, MSGopr);
				}
			}
			else if ((olp = op->parent.oper_olst) == 0)
				fatal("exprtype():olp==0");
			else if (olp->olst_code != 0)
				operinst(olp->olst_code->info.code_inst, op);
			return;
		}
	}
}

Expr *
#ifdef __STDC__
setlessexpr(register const Expr *ep) /* return typed expr, skipping .set's */
#else
setlessexpr(ep)register Expr *ep;
#endif
{
	register Symbol *sp;

	if (ep->ex_type == ExpTy_Unknown)
		fatal("setlessexpr():typeless expression; looping possible");
	while (ep->ex_op == ExpOp_LeafName)
	{
		sp = ep->right.ex_sym;
		if (sp->sym_kind != SymKind_Set)
			fatal("setlessexpr():non-.set name");
		if ((ep = sp->addr.sym_oper->oper_expr) == 0)
			fatal("setlessexpr():no expr for simple type .set");
	}
	return (Expr *)ep;
}

void
#ifdef __STDC__
fixexpr(register Expr *ep) /* "." -> ExpOp_LeafCode and save strings */
#else
fixexpr(ep)register Expr *ep;
#endif
{
	Section *secp;
	Symbol *sp;

	while ((int)ep->ex_op >= ExpOp_OPER_LeafLast) /* until reach leaf */
	{
		if ((int)ep->ex_op < ExpOp_OPER_BinaryLast)
			fixexpr(ep->right.ex_ptr);
		ep = ep->left.ex_ptr;
	}
	if (ep->ex_op == ExpOp_LeafName)
	{
		sp = ep->right.ex_sym;
		sp->sym_refd = 1;	/* symbol referenced */
		if (sp->sym_kind == SymKind_Dot)
		{
			/*
			* Overwrite identifier (.) with ExpOp_LeafCode
			* node.  This breaks the chain of uses, but
			* they are never accessed for . since . cannot
			* be defined by the user.
			*/
			ep->ex_op = ExpOp_LeafCode;
			secp = cursect();
			ep->left.ex_sect = secp;
			ep->right.ex_code = secp->sec_last;
		}
		return;
	}
	else if (ep->ex_op != ExpOp_LeafString)
		return;
	ep->right.ex_str = savestr(ep->right.ex_str, ep->left.ex_len);
}

Expr *
#ifdef __STDC__
unaryexpr(int op, register Expr *el)	/* build unary expression */
#else
unaryexpr(op, el)int op; register Expr *el;
#endif
{
	register Expr *ep;

#ifdef DEBUG
	if (DEBUG('e') > 0)
	{
		(void)fprintf(stderr, "unaryexpr(op=%s,el=", exopstr(op));
		printexpr(el);
		(void)fputs(")\n", stderr);
	}
#endif
	if (op == ExpOp_Subtract)
		op = ExpOp_Negate;
	else if (op == ExpOp_Add)
		op = ExpOp_UnaryAdd;
	else if (op < ExpOp_OPER_UnaryFirst || op >= ExpOp_OPER_UnaryLast)
		fatal("unaryexpr():non-unary operator %s", exopstr(op));
	ep = newexpr();
	ep->ex_op = op;
	ep->left.ex_ptr = el;
	ep->right.ex_ptr = 0;
	ep->ex_type = ExpTy_Unknown;
	el->parent.ex_ptr = ep;
	el->ex_cont = Cont_Expr;
	exprtype(ep);
	return ep;
}

Expr *
#ifdef __STDC__
binaryexpr(int op, register Expr *el, register Expr *er) /* build binary */
#else
binaryexpr(op, el, er)int op; register Expr *el, *er;
#endif
{
	register Expr *ep;

#ifdef DEBUG
	if (DEBUG('e') > 0)
	{
		(void)fprintf(stderr, "binaryexpr(op=%s,el=", exopstr(op));
		printexpr(el);
		(void)fputs(",er=", stderr);
		printexpr(er);
		(void)fputs(")\n", stderr);
	}
#endif
	if (op < ExpOp_OPER_BinaryFirst || op >= ExpOp_OPER_BinaryLast)
		fatal("binaryexpr():non-binary operator %s", exopstr(op));
	ep = newexpr();
	ep->ex_op = op;
	ep->left.ex_ptr = el;
	ep->right.ex_ptr = er;
	ep->ex_type = ExpTy_Unknown;
	el->parent.ex_ptr = ep;
	er->parent.ex_ptr = ep;
	el->ex_cont = Cont_Expr;
	er->ex_cont = Cont_Expr;
	exprtype(ep);
	return ep;
}

Operand *
#ifdef __STDC__
operand(Expr *ep)	/* build simple single operand */
#else
operand(ep)Expr *ep;
#endif
{
	static const Operand empty = {0};
	static Operand *avail, *endavail;
	register Operand *op;

#ifdef DEBUG
	if (DEBUG('e') > 0)
	{
		(void)fputs("operand(ep=", stderr);
		printexpr(ep);
		(void)fputs(")\n", stderr);
	}
#endif
#ifndef ALLOC_OPER_CHUNK
#define ALLOC_OPER_CHUNK 4000
#endif
	if ((op = avail) == endavail)
	{
#ifdef DEBUG
		if (DEBUG('a') > 0)
		{
			static Ulong total;

			(void)fprintf(stderr, "Total Operands=%lu @%lu ea.\n",
				total += ALLOC_OPER_CHUNK,
				(Ulong)sizeof(Operand));
		}
#endif
		avail = op = (Operand *)alloc((void *)0,
			ALLOC_OPER_CHUNK * sizeof(Operand));
		endavail = op + ALLOC_OPER_CHUNK;
	}
	avail = op + 1;
	*op = empty;
	if ((op->oper_expr = ep) != 0)
	{
		ep->ex_cont = Cont_Operand;
		ep->parent.ex_oper = op;
	}
	return op;
}

Oplist *
#ifdef __STDC__
oplist(register Oplist *list, register Operand *op) /* add operand to list */
#else
oplist(list, op)register Oplist *list; register Operand *op;
#endif
{
	static Oplist *avail, *endavail;

#ifdef DEBUG
	if (DEBUG('e') > 0)
	{
		(void)fprintf(stderr, "oplist(list=%#lx,op=%#lx)\n",
			(Ulong)list, (Ulong)op);
	}
#endif
	if (list != 0)	/* append to existing list */
	{
		list->olst_last->oper_next = op;
	}
	else	/* first time for this list */
	{
#ifndef ALLOC_OLST_CHUNK
#define ALLOC_OLST_CHUNK 2000
#endif
		if ((list = avail) == endavail)
		{
#ifdef DEBUG
			if (DEBUG('a') > 0)
			{
				static Ulong total;

				(void)fprintf(stderr,
					"Total Oplists=%lu @%lu ea.\n",
					total += ALLOC_OLST_CHUNK,
					(Ulong)sizeof(Oplist));
			}
#endif
			avail = list = (Oplist *)alloc((void *)0,
				ALLOC_OLST_CHUNK * sizeof(Oplist));
			endavail = list + ALLOC_OLST_CHUNK;
		}
		avail = list + 1;
		list->olst_first = op;
		list->olst_code = 0;
		list->olst_line = curlineno;
		list->olst_file = curfileno;
	}
	if ((list->olst_last = op) != 0)	/* only 0 for no-op instrs */
		op->parent.oper_olst = list;
	return list;
}

#ifdef DEBUG

void
#ifdef __STDC__
printexpr(const Expr *ep)	/* output contents of expr */
#else
printexpr(ep)Expr *ep;
#endif
{
	if (ep == 0)
	{
		(void)fputs("(Expr*)0", stderr);
		return;
	}
	switch (ep->ex_type)	/* prefix expression with type */
	{
	default:
		(void)fprintf(stderr, "{%d?", ep->ex_type);
		break;
	case ExpTy_Register:
		(void)fputs("{%", stderr);
		break;
	case ExpTy_Operand:
		(void)fputs("{o", stderr);
		break;
	case ExpTy_Floating:
		(void)fputs("{f", stderr);
		break;
	case ExpTy_String:
		(void)fputs("{s", stderr);
		break;
	case ExpTy_Integral:
		(void)fputs("{i", stderr);
		break;
	case ExpTy_Relocatable:
		(void)fputs("{r", stderr);
		break;
	case ExpTy_Unknown:
		(void)fputs("{u", stderr);
		break;
	}
	if (ep->ex_mods == 0)
		(void)putc('}', stderr);
	else
		(void)fprintf(stderr, ",%#x}", ep->ex_mods);
	switch (ep->ex_op)	/* handle leaves specially */
	{
	case ExpOp_LeafRegister:
		(void)fprintf(stderr, "%%%d", ep->right.ex_reg);
		return;
	case ExpOp_LeafName:
		(void)fputs((char *)ep->right.ex_sym->sym_name, stderr);
		return;
	case ExpOp_LeafString:
		(void)fprintf(stderr, "\"%s\"",
			prtstr(ep->right.ex_str, ep->left.ex_len));
		return;
	case ExpOp_LeafNumber:
		(void)fputs(num2hex(ep->right.ex_num), stderr);
		return;
	case ExpOp_LeafFloat:
		(void)fputs((const char *)ep->right.ex_flt, stderr);
		return;
	case ExpOp_LeafCode:
		(void)fprintf(stderr, ".=<%s>",
			ep->left.ex_sect->sec_sym->sym_name);
		return;
	}
	(void)putc('[', stderr);
	if ((int)ep->ex_op >= ExpOp_OPER_BinaryFirst
		&& (int)ep->ex_op < ExpOp_OPER_BinaryLast)
	{
		printexpr(ep->left.ex_ptr);
		(void)fputs(exopstr((int)ep->ex_op), stderr);
		printexpr(ep->right.ex_ptr);
	}
	else if ((int)ep->ex_op >= ExpOp_OPER_PostfixFirst
		&& (int)ep->ex_op < ExpOp_OPER_PostfixLast)
	{
		printexpr(ep->left.ex_ptr);
		(void)fputs(exopstr((int)ep->ex_op), stderr);
	}
	else if ((int)ep->ex_op >= ExpOp_OPER_PrefixFirst
		&& (int)ep->ex_op < ExpOp_OPER_PrefixLast)
	{
		(void)fputs(exopstr((int)ep->ex_op), stderr);
		printexpr(ep->left.ex_ptr);
	}
	else
	{
		fatal("printexpr():unknown group for operator: %s",
			exopstr((int)ep->ex_op));
	}
	(void)putc(']', stderr);
}

void
#ifdef __STDC__
printoplist(const Oplist *olp)	/* output contents of operand list */
#else
printoplist(olp)Oplist *olp;
#endif
{
	Operand *op;

	if (olp == 0)
	{
		(void)fputs("(Oplist*)0", stderr);
		return;
	}
	for (op = olp->olst_first;; op = op->oper_next)
	{
		printoperand(op);
		if (op == olp->olst_last)
			break;
		(void)putc(',', stderr);
	}
}

#endif	/* DEBUG */
