/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/dirs.c	1.5"
/*
* common/dirs.c - common assembler directive handling
*
*	Some of the directive expressions are required to be
*	evaluatable when the directive is specified.  These
*	are only those expressions whose value has an immediate
*	affect on the value of . for some section.  These are:
*
*		The second and third operand for .bss.
*		The second operand for a ".set ." directive.
*		The operand for .align.
*		The operand for .zero.
*		The (string) operands for .ascii and .string.
*		The (string) operand for .ident.
*
*	To keep .file and .version simple, these also require
*	that their (string) operand be evaluatable when specified.
*/
#include <stdio.h>
#include <string.h>
#ifndef __STDC__
#  include <memory.h>
#endif
#include "common/as.h"
#include "common/dirs.h"
#include "common/expr.h"
#include "common/nums.h"
#include "common/objf.h"
#include "common/sect.h"
#include "common/syms.h"
#include "align.h"	/* default alignment for bss and common */

static const char *const dirstr[Dir_TOTAL] =
{
	".section",	".pushsection",	".popsection",	".previous",
	".text",	".data",
	".bss",
	".comm",	".lcomm",
	".globl",	".local",	".weak",
	".set",
	".size",	".type",
	".align",
	".zero",
	".byte",	".2byte",	".4byte",	".8byte",
	".float",	".double",	".ext",
	".ascii",	".string",
	".file",	".ident",	".version",
};

static const char *curstr;	/* current directive name */

static const char MSGtoo[] = "too many operands for %s";
static const char MSGxpg[] = "expecting %s operand for %s, operand %d";
static const char MSGnon[] = "non-%s operand for %s, operand %d";
static const char MSGlst[] = "invalid %s directive--expecting list of %s";
static const char MSGrng[] = "%s for %s out of range: %s";
static const char MSGaln[] = "%s has been aligned for %s";

enum { DSyms_Text, DSyms_Data, DSyms_Bss, DSyms_TOTAL };
static Symbol *dirsyms[DSyms_TOTAL];

enum { DSect_Comment, DSect_TOTAL };
static Section *dirsect[DSect_TOTAL];

static Expr *def_align_expr;	/* default align expr for common; shared */

static Symbol *
#ifdef __STDC__
dsym(const char *str)	/* look up symbol str */
#else
dsym(str)char *str;
#endif
{
	return lookup((const Uchar *)str, (size_t)strlen(str));
}

void
#ifdef __STDC__
initdirs(void)	/* initialize symbol and sections used here */
#else
initdirs()
#endif
{
	/*
	* Assumes that sections have been initialized.
	*/
	dirsyms[DSyms_Text] = dsym(dirstr[Dir_Text]);
	dirsyms[DSyms_Data] = dsym(dirstr[Dir_Data]);
	dirsyms[DSyms_Bss] = dsym(dirstr[Dir_Bss]);
	dirsect[DSect_Comment] = dsym(".comment")->sym_sect;
	def_align_expr = ulongexpr((Ulong)BSS_COMM_ALIGN);
}

static Symbol *
#ifdef __STDC__
ident_check(int num, register Operand *op)	/* verify Operand is name */
#else
ident_check(num, op)int num; register Operand *op;
#endif
{
	register Expr *ep;

	if (op == 0 || op->oper_flags != 0 || (ep = op->oper_expr) == 0
		|| ep->ex_op != ExpOp_LeafName)
	{
		error(MSGxpg, "name", curstr, num);
		return 0;
	}
	return ep->right.ex_sym;
}

static Expr *
#ifdef __STDC__
string_check(int num, register Operand *op)	/* verify Operand is string */
#else
string_check(num, op)int num; register Operand *op;
#endif
{
	register Expr *ep;

	/*
	* Don't allow ExpTy_Unknown here since all uses require an
	* immediately evaluatable string.  (Most affect the value
	* of . for the cooresponding section.)  The setlessexpr()
	* call removes any .set's so that a ExpOp_LeafString is
	* the guaranteed return.
	*/
	if (op == 0 || op->oper_flags != 0 || (ep = op->oper_expr) == 0
		|| ep->ex_type != ExpTy_String)
	{
		error(MSGxpg, "string", curstr, num);
		return 0;
	}
	return setlessexpr(ep);
}

static Expr *
#ifdef __STDC__
intrel_check(int num, register Operand *op)	/* verify int or reloc expr */
#else
intrel_check(num, op)int num; register Operand *op;
#endif
{
	static const char MSGintrel[] = "integral/relocatable expr";
	register Expr *ep;

	if (op == 0 || op->oper_flags != 0 || (ep = op->oper_expr) == 0)
	{
		error(MSGxpg, MSGintrel, curstr, num);
		return 0;
	}
	if (ep->ex_type == ExpTy_Unknown)
	{
		op->oper_tybits
			= BIT(ExpTy_Integral) | BIT(ExpTy_Relocatable);
	}
	else if (ep->ex_type != ExpTy_Integral
		&& ep->ex_type != ExpTy_Relocatable)
	{
		error(MSGnon, MSGintrel, curstr, num);
		return 0;
	}
	return ep;
}

static Expr *
#ifdef __STDC__
intexpr_check(int num, register Operand *op)	/* verify as int expr */
#else
intexpr_check(num, op)int num; register Operand *op;
#endif
{
	static const char MSGintexpr[] = "integral expression";
	register Expr *ep;

	if (op == 0 || op->oper_flags != 0 || (ep = op->oper_expr) == 0)
	{
		error(MSGxpg, MSGintexpr, curstr, num);
		return 0;
	}
	if (ep->ex_type == ExpTy_Unknown)
		op->oper_tybits = BIT(ExpTy_Integral);
	else if (ep->ex_type != ExpTy_Integral)
	{
		error(MSGnon, MSGintexpr, curstr, num);
		return 0;
	}
	return ep;
}

static Expr *
#ifdef __STDC__
strint_check(int num, register Operand *op)	/* verify as int or str */
#else
strint_check(num, op)int num; register Operand *op;
#endif
{
	static const char MSGstrint[] = "string/integral expression";
	register Expr *ep;

	if (op == 0 || op->oper_flags != 0 || (ep = op->oper_expr) == 0)
	{
		error(MSGxpg, MSGstrint, curstr, num);
		return 0;
	}
	if (ep->ex_type == ExpTy_Unknown)
		op->oper_tybits = BIT(ExpTy_String) | BIT(ExpTy_Integral);
	else if (ep->ex_type != ExpTy_String
		&& ep->ex_type != ExpTy_Integral)
	{
		error(MSGnon, MSGstrint, curstr, num);
		return 0;
	}
	return ep;	/* if ExpTy_String, setlessexpr() called later */
}

static Expr *
#ifdef __STDC__
intflt_check(int num, register Operand *op)	/* verify as int or flt */
#else
intflt_check(num, op)int num; register Operand *op;
#endif
{
	static const char MSGintflt[] = "floating/integral expression";
	register Expr *ep;

	if (op == 0 || op->oper_flags != 0 || (ep = op->oper_expr) == 0)
	{
		error(MSGxpg, MSGintflt, curstr, num);
		return 0;
	}
	if (ep->ex_type == ExpTy_Unknown)
		op->oper_tybits = BIT(ExpTy_Floating) | BIT(ExpTy_Integral);
	else if (ep->ex_type != ExpTy_Floating
		&& ep->ex_type != ExpTy_Integral)
	{
		error(MSGnon, MSGintflt, curstr, num);
		return 0;
	}
	return ep;
}

static Value *
#ifdef __STDC__
eval_check(int num, register Operand *op)	/* verify as evaluatable */
#else
eval_check(num, op)int num; register Operand *op;
#endif
{
	static const char MSGevalexpr[] = "evaluatable integral expr";
	register Expr *ep;
	register Value *vp;

	if (op == 0 || op->oper_flags != 0 || (ep = op->oper_expr) == 0)
	{
		error(MSGxpg, MSGevalexpr, curstr, num);
		return 0;
	}
	if (ep->ex_type != ExpTy_Integral)
	{
		error(MSGnon, MSGevalexpr, curstr, num);
		return 0;
	}
	vp = evalexpr(ep);
	if (vp->val_flags & VAL_LDIFF)
		delayeval(vp);
	return vp;
}

static Ulong
#ifdef __STDC__
align_check(int num, Operand *op)	/* verify Operand is valid align */
#else
align_check(num, op)int num; Operand *op;
#endif
{
	register Value *vp;

	if ((vp = eval_check(num, op)) == 0)
		return 0;
	if (vp->val_flags & (VAL_OFLOW | VAL_TRUNC))
	{
		error(MSGrng, "alignment", curstr, num2hex(vp->val_num));
		return 0;
	}
#ifdef ALIGN_IS_POW2
	/*
	* Valid alignments are positive and are powers of two.
	*/
	if (vp->val_ulong == 0
		|| ((vp->val_ulong - 1) & vp->val_ulong) != 0)
	{
		error("invalid alignment (%lu) for %s, operand %d",
			vp->val_ulong, curstr, num);
		return 0;
	}
#else
	/*
	* Otherwise use implementation-provided test that must
	* at least verify that the value is nonzero.
	*/
	if (!aligncheck(vp->val_ulong))
	{
		error("invalid alignment (%lu) for %s, operand %d",
			vp->val_ulong, curstr, num);
		return 0;
	}
#endif
	return vp->val_ulong;
}

static void
#ifdef __STDC__
dot_section(register Operand *op)	/* .section id [int/str [int/str]] */
#else
dot_section(op)register Operand *op;
#endif
{
	Symbol *sp;
	Expr *e2 = 0, *e3 = 0;

	if ((sp = ident_check(1, op)) == 0)
		return;
	if ((op = op->oper_next) != 0)	/* a second operand */
	{
		if ((e2 = strint_check(2, op)) == 0)
			return;
		if ((op = op->oper_next) != 0)	/* a third operand */
		{
			if ((e3 = strint_check(3, op)) == 0)
				return;
			if (op->oper_next != 0)
			{
				error(MSGtoo, curstr);
				return;
			}
		}
	}
	setsect(sp, e2, e3);
}

static void
#ifdef __STDC__
dot_pushsect(Operand *op)	/* .pushsection ident */
#else
dot_pushsect(op)Operand *op;
#endif
{
	Symbol *sp;
	Section *secp;

	if ((sp = ident_check(1, op)) == 0)
		return;
	if ((secp = sp->sym_sect) == 0)
	{
		error(MSGnon, "section name", curstr, 1);
		return;
	}
	if (op->oper_next != 0)
	{
		error(MSGtoo, curstr);
		return;
	}
	(void)stacksect(secp);
}

static void
#ifdef __STDC__
dot_popsect(Operand *op)	/* .popsection */
#else
dot_popsect(op)Operand *op;
#endif
{
	if (op != 0)
	{
		error(MSGtoo, curstr);
		return;
	}
	if (!stacksect((Section *)0))
		error("section stack empty: too many %s directives", curstr);
}

static void
#ifdef __STDC__
dot_previous(Operand *op)	/* .previous */
#else
dot_previous(op)Operand *op;
#endif
{
	if (op != 0)
	{
		error(MSGtoo, curstr);
		return;
	}
	prevsect();
}

static void
#ifdef __STDC__
dot_textdata(Operand *op, int sym)	/* .text/.data */
#else
dot_textdata(op, sym)Operand *op; int sym;
#endif
{
	if (op != 0)
	{
		error(MSGtoo, curstr);
		return;
	}
	setsect(dirsyms[sym], (Expr *)0, (Expr *)0);
}

static void
#ifdef __STDC__
dot_bss(register Operand *op)	/* .bss [ident evalexpr [evalexpr]] */
#else
dot_bss(op)register Operand *op;
#endif
{
	register Value *vp;
	Symbol *sp;
	Ulong size, align;

	if (op == 0)	/* change to .bss section */
	{
		setsect(dirsyms[DSyms_Bss], (Expr *)0, (Expr *)0);
		return;
	}
	if ((sp = ident_check(1, op)) == 0)
		return;
	op = op->oper_next;
	if ((vp = eval_check(2, op)) == 0)
		return;
	if (vp->val_flags & (VAL_OFLOW | VAL_TRUNC))
	{
		error(MSGrng, "size", curstr, num2hex(vp->val_num));
		return;
	}
	size = vp->val_ulong;
	if ((op = op->oper_next) == 0)
		align = BSS_COMM_ALIGN;
	else
	{
		if ((align = align_check(3, op)) == 0)
			return;
		if (op->oper_next != 0)
		{
			error(MSGtoo, curstr);
			return;
		}
	}
	bsssym(sp, Bind_Local, size, align);
}

static void
#ifdef __STDC__
dot_comm(register Operand *op, int bind) /* .[l]comm id intexpr [intexpr] */
#else
dot_comm(op, bind)register Operand *op; int bind;
#endif
{
	Expr *ep;
	Expr *align;
	Symbol *sp;

	if ((sp = ident_check(1, op)) == 0)
		return;
	op = op->oper_next;
	if ((ep = intexpr_check(2, op)) == 0)
		return;
	if ((op = op->oper_next) == 0)
		align = def_align_expr;	/* shared expr here is okay */
	else
	{
		if ((align = intexpr_check(3, op)) == 0)
			return;
		if (op->oper_next != 0)
		{
			error(MSGtoo, curstr);
			return;
		}
	}
	commsym(sp, bind, ep, align);
}

static void
#ifdef __STDC__
dot_bind(register Operand *op, int bind) /* .globl/.local/.weak id-list */
#else
dot_bind(op, bind)register Operand *op; int bind;
#endif
{
	register Symbol *sp;
	register int num = 0;

	if (op == 0)
	{
		error(MSGlst, curstr, "names");
		return;
	}
	do
	{
		if ((sp = ident_check(++num, op)) != 0)
			bindsym(sp, bind);
	} while ((op = op->oper_next) != 0);
}

static void
#ifdef __STDC__
dot_set(register Operand *op)	/* .set ident operand */
#else
dot_set(op)register Operand *op;
#endif
{
	Symbol *sp;

	if ((sp = ident_check(1, op)) == 0)
		return;
	if ((op = op->oper_next) == 0)
	{
		error("%s missing operand 2", curstr);
		return;
	}
	if (op->oper_next != 0)
	{
		error(MSGtoo, curstr);
		return;
	}
	opersym(sp, op);
}

static void
#ifdef __STDC__
dot_size(register Operand *op)	/* .size ident intexpr */
#else
dot_size(op)register Operand *op;
#endif
{
	Expr *ep;
	Symbol *sp;

	if (flags & ASFLAG_TRANSITION
		&& obsdirect((const Uchar *)curstr, (size_t)0, op) != 0)
	{
		return;		/* obsolete usage */
	}
	if ((sp = ident_check(1, op)) == 0)
		return;
	op = op->oper_next;
	if ((ep = intexpr_check(2, op)) == 0)
		return;
	if (op->oper_next != 0)
	{
		error(MSGtoo, curstr);
		return;
	}
	sizesym(sp, ep);
}

static void
#ifdef __STDC__
dot_type(register Operand *op)	/* .type ident int/str */
#else
dot_type(op)register Operand *op;
#endif
{
	Expr *ep;
	Symbol *sp;

	if (flags & ASFLAG_TRANSITION
		&& obsdirect((const Uchar *)curstr, (size_t)0, op) != 0)
	{
		return;		/* obsolete usage */
	}
	if ((sp = ident_check(1, op)) == 0)
		return;
	op = op->oper_next;
	if ((ep = strint_check(2, op)) == 0)
		return;
	if (op->oper_next != 0)
	{
		error(MSGtoo, curstr);
		return;
	}
	typesym(sp, ep);
}

static void
#ifdef __STDC__
dot_align(Operand *op)	/* .align evalexpr */
#else
dot_align(op)Operand *op;
#endif
{
	Ulong align;

	if ((align = align_check(1, op)) == 0)
		return;
	if (op->oper_next != 0)
	{
		error(MSGtoo, curstr);
		return;
	}
	sectalign((Section *)0, align);
}

static void
#ifdef __STDC__
dot_zero(Operand *op)	/* .zero evalexpr */
#else
dot_zero(op)Operand *op;
#endif
{
	register Value *vp;

	if ((vp = eval_check(1, op)) == 0)
		return;
	if (vp->val_flags & (VAL_OFLOW | VAL_TRUNC))
	{
		error(MSGrng, "number of bytes", curstr,
			num2hex(vp->val_num));
		return;
	}
	if (op->oper_next != 0)
	{
		error(MSGtoo, curstr);
		return;
	}
	sectzero((Section *)0, vp->val_ulong);	/* don't ignore ".zero 0" */
}

static void
#ifdef __STDC__
dot_bytes(register Operand *op, Ulong size, int form) /* .byte/... expr-list */
#else
dot_bytes(op, size, form)register Operand *op; Ulong size; int form;
#endif
{
	register Expr *ep;
	register int num = 0;
	register Section *secp;

	if (op == 0)
	{
		error(MSGlst, curstr, "expressions");
		return;
	}
	secp = cursect();
	do
	{
		if ((ep = intrel_check(++num, op)) != 0)
		{
			fixexpr(ep);
			sectexpr(secp, ep, size, form);
		}
	} while ((op = op->oper_next) != 0);
}

static void
#ifdef __STDC__
dot_fltbytes(register Operand *op, int dir) /* .float/.double/.ext flt-list */
#else
dot_fltbytes(op, dir)register Operand *op; int dir;
#endif
{
	register Expr *ep;
	register int num = 0;
	register Section *secp;
	Ulong sz, al;

	if (op == 0)
	{
		error(MSGlst, curstr, "floating/integral expressions");
		return;
	}
	secp = cursect();
	/*
	* Hopefully the "unsupported directive" error and the align
	* check will be eliminated if they cannot be reached.
	*/
	switch (dir)
	{
	default:
		fatal("dot_fltbytes():unknown directive: (%d) %s",
			dir, curstr);
		/*NOTREACHED*/
	case Dir_Float:
		/*CONSTANTCONDITION*/
		if (FLOAT_SIZE == 0)
		{
		unsup:;
			error("unsupported directive: %s", curstr);
			return;
		}
		sz = FLOAT_SIZE;
		dir = CodeForm_Float;
		/*CONSTANTCONDITION*/
		if (FLOAT_ALIGN > 1)
		{
			al = FLOAT_ALIGN;
		check:;
			if (checkalign(secp, al) == 0)
				warn(MSGaln, secp->sec_sym->sym_name, curstr);
		}
		break;
	case Dir_Double:
		/*CONSTANTCONDITION*/
		if (DOUBLE_SIZE == 0)
			goto unsup;
		sz = DOUBLE_SIZE;
		dir = CodeForm_Double;
		/*CONSTANTCONDITION*/
		if (DOUBLE_ALIGN > 1)
		{
			al = DOUBLE_ALIGN;
			goto check;
		}
		break;
	case Dir_Ext:
		/*CONSTANTCONDITION*/
		if (EXT_SIZE == 0)
			goto unsup;
		sz = EXT_SIZE;
		dir = CodeForm_Extended;
		/*CONSTANTCONDITION*/
		if (EXT_ALIGN > 1)
		{
			al = EXT_ALIGN;
			goto check;
		}
		break;
	}
	do
	{
		if ((ep = intflt_check(++num, op)) != 0)
		{
			fixexpr(ep);
			sectexpr(secp, ep, sz, dir);
		}
	} while ((op = op->oper_next) != 0);
}

static void
#ifdef __STDC__
dot_strlist(register Operand *op, int dir) /* .ascii/.string str-list */
#else
dot_strlist(op, dir)register Operand *op; int dir;
#endif
{
	register Expr *ep;
	register int num = 0;
	register Section *secp;

	if (op == 0)
	{
		error(MSGlst, curstr, "strings");
		return;
	}
	secp = cursect();
	do
	{
		if ((ep = string_check(++num, op)) != 0)
		{
			fixexpr(ep);
			if (dir == Dir_String)
				ep->left.ex_len++; /* savestr() adds \0 */
			sectstr(secp, ep);
		}
	} while ((op = op->oper_next) != 0);
}

static void
#ifdef __STDC__
dot_strmisc(Operand *op, int dir)	/* .file/.ident/.version strexpr */
#else
dot_strmisc(op, dir)Operand *op; int dir;
#endif
{
	register Expr *ep;

	if ((ep = string_check(1, op)) == 0)
		return;
	if (op->oper_next != 0)
	{
		error(MSGtoo, curstr);
		return;
	}
	switch (dir)
	{
	default:
		fatal("do_strmisc():unknown directive: (%d) %s",
			dir, curstr);
		/*NOTREACHED*/
	case Dir_File:
		if (objfsrcstr(ep) == 0)
			error("too many %s directives", curstr);
		break;
	case Dir_Ident:
		fixexpr(ep);
		if (ep->right.ex_str[ep->left.ex_len - 1] != '\0')
			ep->left.ex_len++;	/* savestr() adds \0 */
		sectstr(dirsect[DSect_Comment], ep);
		break;
	case Dir_Version:
		versioncheck(ep->right.ex_str, ep->left.ex_len);
		break;
	}
}

void
#ifdef __STDC__
aligndata(const char *dir, Operand *op, Ulong sz, Ulong al, int form) /* init */
#else
aligndata(dir, op, sz, al, form)char *dir; Operand *op; Ulong sz, al; int form;
#endif
{
	/*
	* Called by implementation-specific code to initialize
	* aligned values.
	*/
	if (al > 1 && checkalign((Section *)0, al) == 0)
		warn(MSGaln, cursect()->sec_sym->sym_name, dir);
	curstr = dir;
	dot_bytes(op, sz, form);
}

void
#ifdef __STDC__
directive(register const Uchar *s, size_t n, Oplist *p)
#else
directive(s, n, p)register Uchar *s; size_t n; Oplist *p;
#endif
{
	Operand *ops;

#ifdef DEBUG
	if (DEBUG('d') > 0)
	{
		(void)fprintf(stderr, "directive(%s,p=", prtstr(s, n));
		printoplist(p);
		(void)fputs(")\n", stderr);
	}
#endif
	if (p == 0)
		ops = 0;
	else
		ops = p->olst_first;
	/*
	* Determine which directive.  Fast (but ugly) selection code.
	* Purposefully extra-expanded to ease future new directives.
	* It would be nice if there were a tool that could generate
	* code such as the following given a simple (list of strings
	* and associated code) description.
	*/
	switch (n)
	{
	case 4: /* bss, ext, set */
		if (s[3] == 't')
		{
			if (s[2] == 'e')
			{
				if (s[1] == 's')
				{
					curstr = dirstr[Dir_Set];
					dot_set(ops);
					return;
				}
			}
			else if (s[2] == 'x' && s[1] == 'e')
			{
				curstr = dirstr[Dir_Ext];
				dot_fltbytes(ops, Dir_Ext);
				return;
			}
		}
		else if (s[1] == 'b' && s[2] == 's' && s[3] == 's')
		{
			curstr = dirstr[Dir_Bss];
			dot_bss(ops);
			return;
		}
		break;
	case 5: /* byte, comm, data, file, size, text, type, weak, zero */
		if (s[4] == 'e')
		{
			if (s[2] == 'y')
			{
				if (s[3] == 't')
				{
					if (s[1] == 'b')
					{
						curstr = dirstr[Dir_Byte];
						dot_bytes(ops, (Ulong)1,
							CodeForm_Integral);
						return;
					}
				}
				else if (s[3] == 'p')
				{
					if (s[1] == 't')
					{
						curstr = dirstr[Dir_Type];
						dot_type(ops);
						return;
					}
				}
			}
			else if (s[2] == 'i')
			{
				if (s[3] == 'z')
				{
					if (s[1] == 's')
					{
						curstr = dirstr[Dir_Size];
						dot_size(ops);
						return;
					}
				}
				else if (s[3] == 'l')
				{
					if (s[1] == 'f')
					{
						curstr = dirstr[Dir_File];
						dot_strmisc(ops, Dir_File);
						return;
					}
				}
			}
		}
		else if (s[2] == 'e')
		{
			if (s[1] == 't')
			{
				if (s[3] == 'x' && s[4] == 't')
				{
					curstr = dirstr[Dir_Text];
					dot_textdata(ops, DSyms_Text);
					return;
				}
			}
			else if (s[1] == 'z')
			{
				if (s[3] == 'r' && s[4] == 'o')
				{
					curstr = dirstr[Dir_Zero];
					dot_zero(ops);
					return;
				}
			}
			else if (s[1] == 'w')
			{
				if (s[3] == 'a' && s[4] == 'k')
				{
					curstr = dirstr[Dir_Weak];
					dot_bind(ops, Bind_Weak);
					return;
				}
			}
		}
		else if (s[1] == 'd')
		{
			if (s[2] == 'a' && s[3] == 't' && s[4] == 'a')
			{
				curstr = dirstr[Dir_Data];
				dot_textdata(ops, DSyms_Data);
				return;
			}
		}
		else if (s[1] == 'c')
		{
			if (s[2] == 'o' && s[3] == 'm' && s[4] == 'm')
			{
				curstr = dirstr[Dir_Comm];
				dot_comm(ops, Bind_Global);
				return;
			}
		}
		break;
	case 6: /* align, ascii, float, globl, ident, lcomm, local, Nbyte */
		if (s[2] == 'b')
		{
			if (s[3] == 'y' && s[4] == 't' && s[5] == 'e')
			{
				if (s[1] == '4')
				{
					curstr = dirstr[Dir_4Byte];
					dot_bytes(ops, (Ulong)4,
						CodeForm_Integral);
					return;
				}
				else if (s[1] == '2')
				{
					curstr = dirstr[Dir_2Byte];
					dot_bytes(ops, (Ulong)2,
						CodeForm_Integral);
					return;
				}
				else if (s[1] == '8')
				{
					curstr = dirstr[Dir_8Byte];
					dot_bytes(ops, (Ulong)8,
						CodeForm_Integral);
					return;
				}
			}
		}
		else if (s[2] == 'l')
		{
			if (s[3] == 'o')
			{
				if (s[1] == 'g')
				{
					if (s[4] == 'b' && s[5] == 'l')
					{
						curstr = dirstr[Dir_Global];
						dot_bind(ops, Bind_Global);
						return;
					}
				}
				else if (s[1] == 'f')
				{
					if (s[4] == 'a' && s[5] == 't')
					{
						curstr = dirstr[Dir_Float];
						dot_fltbytes(ops, Dir_Float);
						return;
					}
				}
			}
			else if (s[3] == 'i')
			{
				if (s[1] == 'a')
				{
					if (s[4] == 'g' && s[5] == 'n')
					{
						curstr = dirstr[Dir_Align];
						dot_align(ops);
						return;
					}
				}
			}
		}
		else if (s[1] == 'l')
		{
			if (s[2] == 'o')
			{
				if (s[3] == 'c')
				{
					if (s[4] == 'a' && s[5] == 'l')
					{
						curstr = dirstr[Dir_Local];
						dot_bind(ops, Bind_Local);
						return;
					}
				}
			}
			else if (s[2] == 'c')
			{
				if (s[3] == 'o')
				{
					if (s[4] == 'm' && s[5] == 'm')
					{
						curstr = dirstr[Dir_Lcomm];
						dot_comm(ops, Bind_Local);
						return;
					}
				}
			}
		}
		else if (s[1] == 'i')
		{
			if (s[2] == 'd')
			{
				if (s[3] == 'e')
				{
					if (s[4] == 'n' && s[5] == 't')
					{
						curstr = dirstr[Dir_Ident];
						dot_strmisc(ops, Dir_Ident);
						return;
					}
				}
			}
		}
		else if (s[1] == 'a')
		{
			if (s[2] == 's')
			{
				if (s[3] == 'c')
				{
					if (s[4] == 'i' && s[5] == 'i')
					{
						curstr = dirstr[Dir_Ascii];
						dot_strlist(ops, Dir_Ascii);
						return;
					}
				}
			}
		}
		break;
	case 7: /* double, string */
		if (s[1] == 's')
		{
			if (strncmp((const char *)&s[2], "tring", 5) == 0)
			{
				curstr = dirstr[Dir_String];
				dot_strlist(ops, Dir_String);
				return;
			}
		}
		else if (s[1] == 'd')
		{
			if (strncmp((const char *)&s[2], "ouble", 5) == 0)
			{
				curstr = dirstr[Dir_Double];
				dot_fltbytes(ops, Dir_Double);
				return;
			}
		}
		break;
	case 8: /* section, version */
		if (s[2] == 'e' && s[5] == 'i' && s[6] == 'o' && s[7] == 'n')
		{
			if (s[1] == 's')
			{
				if (s[3] == 'c' && s[4] == 't')
				{
					curstr = dirstr[Dir_Section];
					dot_section(ops);
					return;
				}
			}
			else if (s[1] == 'v')
			{
				if (s[3] == 'r' && s[4] == 's')
				{
					curstr = dirstr[Dir_Version];
					dot_strmisc(ops, Dir_Version);
					return;
				}
			}
		}
		break;
	case 9: /* previous */
		if (strncmp((const char *)&s[1], "previous", 8) == 0)
		{
			curstr = dirstr[Dir_Previous];
			dot_previous(ops);
			return;
		}
		break;
	case 11: /* popsection */
		if (strncmp((const char *)&s[1], "popsection", 10) == 0)
		{
			curstr = dirstr[Dir_Popsect];
			dot_popsect(ops);
			return;
		}
		break;
	case 12: /* pushsection */
		if (strncmp((const char *)&s[1], "pushsection", 11) == 0)
		{
			curstr = dirstr[Dir_Pushsect];
			dot_pushsect(ops);
			return;
		}
		break;
	}
	/*
	* Only here if no dot_... directive function called.
	*/
	if (flags & ASFLAG_TRANSITION && obsdirect(s, n, ops) != 0)
		return;
	error("unknown directive: %s", prtstr(s, n));
}
