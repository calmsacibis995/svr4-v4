/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:i386/dirs386.c	1.1"
/*
* i386/dirs386.c - i386 assembler-specific directive handling
*/
#include <stdio.h>
#include <string.h>
#include "common/as.h"
#include "common/dirs.h"
#include "common/expr.h"
#include "common/nums.h"
#include "common/sect.h"
#include "common/syms.h"
#include "dirs386.h"

void
#ifdef __STDC__
bcd2data(Uchar *p, Uint n, Value *vp)	/* convert value to BCD bytes */
#else
bcd2data(p, n, vp) Uchar *p; Uint n; Value *vp;
#endif
{
    /* Convert value to packed BCD bytes.  The i386 BCD form is
    ** 	S0 DD DD DD DD DD DD DD DD DD
    ** where S0 is the most significant byte (S is the sign),
    ** DD are packed BCD digit pairs, reading from most
    ** significant to least significant byte.  (The lowest
    ** addressed byte is least significant.)
    */
#define BCD_DIGS 18			/* 18 actual digits allowed */
#define BCD_BYTES 10			/* 10 bytes in representation */
    char *vs = num2dec(vp->val_num);	/* get decimal digit string */
    Uint vlen = strlen(vs);		/* length of value string */

    p[BCD_BYTES-1] = 0;			/* sign */
    if (vs[0] == '-') {
	p[BCD_BYTES-1] = 0x80;
	++vs;
	--vlen;
    }

    if (vlen > BCD_DIGS) {
	Ulong fno, lno;
	exprfrom(&fno, &lno, vp->val_expr);
	backerror(fno, lno, "BCD number too big: %s",
		p[BCD_BYTES-1] != 0 ? vs-1 : vs);
    }
    else {
	int i;
	char *vend = &vs[vlen-1];	/* point at last digit */

	for (i = 0; i < vlen; ++i, --vend) {
	    if ((i & 1) == 0)
		p[i/2] = *vend & 0xF;
	    else
		p[i/2] |= (*vend & 0xF) << 4;
	}

	/* zero most significant bytes */
	for (i = (i+1)/2; i < BCD_DIGS/2; ++i)
	    p[i] = 0;
    }
    return;
}

void
#ifdef __STDC__
directive386(const Uchar *s, size_t n, Oplist *p)	/* i386 directives */
#else
directive386(s, n, p)Uchar *s; size_t n; Oplist *p;
#endif
{
	static const char MSGtoo[] = "too many operands for %s";
	Operand *op;

#ifdef DEBUG
	if (DEBUG('d') > 1)
	{
		(void)fprintf(stderr, "directive386(%s,p=", prtstr(s, n));
		printoplist(p);
		(void)fputs(")\n", stderr);
	}
#endif
	if (p == 0)
		op = 0;
	else
		op = p->olst_first;
	/*
	* i386-specific directives are:
	*	.bcd	evalexpr-list
	*	.even
	*	.long	expr-list
	*	.value	expr-list
	*	.jmpbeg
	*	.jmpend
	*/
	switch (n)
	{
	case 4:
		if (s[1] == 'b' && s[2] == 'c' && s[3] == 'd')
		{
			aligndata(".bcd", op,
				(Ulong)10, (Ulong)1, CodeForm_BCD);
			return;
		}
		break;
	case 5:
		if (s[1] == 'l')
		{
			if (s[2] == 'o' && s[3] == 'n' && s[4] == 'g')
			{
				aligndata(".long", op,
					(Ulong)4, (Ulong)1, CodeForm_Integral);
				return;
			}
		}
		else if (s[1] == 'e' && s[2] == 'v' && s[3] == 'e'
			&& s[4] == 'n')
		{
			if (op != 0)
			{
				error(MSGtoo, ".even");
				return;
			}
			sectalign((Section *)0, (Ulong)2);
			return;
		}
		break;
	case 6:
		if (s[1] == 'v' && s[2] == 'a' && s[3] == 'l'
			&& s[4] == 'u' && s[5] == 'e')
		{
			aligndata(".value", op,
				(Ulong)2, (Ulong)1, CodeForm_Integral);
			return;
		}
		break;
	case 7:
		if (s[1] == 'j' && s[2] == 'm' && s[3] == 'p')
		{
			if (s[4] == 'b')
			{
				if (s[5] == 'e' && s[6] == 'g')
				{
					if (op != 0)
						error(MSGtoo, ".jmpbeg");
					sectexpr((Section *)0,
						ulongexpr((Ulong)0xf1),
						(Ulong)1,
						CodeForm_Integral);
					return;
				}
			}
			else if (s[4] == 'e' && s[5] == 'n' && s[6] == 'd')
			{
				if (op != 0)
					error(MSGtoo, ".jmpend");
				sectexpr((Section *)0,
					ulongexpr((Ulong)0xffffL),
					(Ulong)2,
					CodeForm_Integral);
				return;
			}
		}
		break;
	}
	directive(s, n, p);	/* try common directives */
}

void
#ifdef __STDC__
obssectattr(Section *secp, const Expr *ep)	/* handle obsolete attrs */
#else
obssectattr(secp, ep)Section *secp; Expr *ep;
#endif
{
	static const char MSGattr[] = "invalid attribute string: \"%s\"";
	static int have_warned;
	register const Uchar *p = ep->right.ex_str;
	register size_t len;
	register Ulong offbits = 0;

	if (!have_warned)
	{
		warn("using obsolete semantics for .section attributes");
		have_warned = 1;
	}
	for (len = ep->left.ex_len; len != 0; len--)
	{
		switch (*p++)
		{
		default:
			error(MSGattr, prtstr(ep->right.ex_str,
				ep->left.ex_len));
			return;
		case 'b':
			secp->attr.sec_value |= Attr_Alloc | Attr_Write;
			secp->type.sec_value = SecTy_Nobits;
			break;
		case 'i':
			secp->type.sec_value = SecTy_Progbits;
			break;
		case 'w':
			secp->attr.sec_value |= Attr_Alloc | Attr_Write;
			secp->type.sec_value = SecTy_Progbits;
			break;
		case 'x':
			secp->attr.sec_value |= Attr_Alloc | Attr_Exec;
			secp->type.sec_value = SecTy_Progbits;
			break;
		case 'n':
			offbits = Attr_Alloc;
			break;
		case 'c':
		case 'd':
		case 'l':
		case 'o':
			secp->type.sec_value = SecTy_Progbits;
			break;
		}
	}
	secp->attr.sec_value &= ~offbits;
}

static Expr *dot_def_expr;	/* last .def identifier as expr */
static Expr *dot_val_expr;	/* nonzero => last .val was .val . */
static int scl_negone;		/* nonzero => last .scl was .scl -1 */

static void
#ifdef __STDC__
dot_def(Expr *ep)	/* remember most recent name operand */
#else
dot_def(ep)Expr *ep;
#endif
{
	if (ep != 0 && ep->ex_op == ExpOp_LeafName)
		dot_def_expr = ep;
	else
		dot_def_expr = 0;
}

static void
#ifdef __STDC__
dot_val(Expr *ep)	/* remember most recent . (dot) operand */
#else
dot_val(ep)Expr *ep;
#endif
{
	if (ep != 0 && ep->ex_op == ExpOp_LeafName
		&& ep->right.ex_sym->sym_kind == SymKind_Dot)
	{
		dot_val_expr = ep;
	}
	else
		dot_val_expr = 0;
}

static void
#ifdef __STDC__
dot_scl(const Expr *ep)	/* remember whether most recent operand is -1 */
#else
dot_scl(ep)Expr *ep;
#endif
{
	Ulong value;

	if (ep != 0 && ep->ex_type == ExpTy_Integral
		&& ep->ex_op == ExpOp_Negate
		&& ep->left.ex_ptr->ex_op == ExpOp_LeafNumber
		&& num2ulong(&value, ep->left.ex_ptr->right.ex_num) != 0
		&& value == 1)
	{
		scl_negone = 1;
	}
	else
		scl_negone = 0;
}

enum	/* obsolete directives */
{
	Obs_Ln,
	Obs_Def,
	Obs_Scl,
	Obs_Val,
	Obs_Tag,
	Obs_Dim,
	Obs_Line,
	Obs_Size,
	Obs_Type,
	Obs_Endef,
	Obs_TOTAL	/* not an obsolete directive */
};


int
#ifdef __STDC__
obsdirect(const Uchar *s, size_t n, Operand *op)
#else
obsdirect(s, n, op)Uchar *s; size_t n; Operand *op;
#endif
{
	static int have_warned[Obs_TOTAL];
	register Expr *ep;
	int dirno;

	/*
	* Ugly patch for COFF to ELF transition.  Take the following
	* sequence of old COFF debugging directives as the equivalent
	* of a .type and .size directive pair for the function:
	*
	*	.def	func;	.val	.;	.scl	-1;	.endef
	* becomes
	*	.type	func, "function"
	*	.size	func, .-func
	*
	* Hold on to such operands for the above three obsolete
	* directives and at each .endef, check for all three being
	* set at once.
	*
	* Also, only issue one specific obsolete warning for each
	* particular directive.
	*/
	switch (n)
	{
	default:
		return 0;
	case 0:		/* only .size or .type */
		if (op == 0 || op->oper_next != 0)
			return 0;
		if (s[1] == 's')
			dirno = Obs_Size;
		else
			dirno = Obs_Type;
		break;
	case 3:
		if (strncmp((const char *)&s[1], "ln", 2) != 0)
			return 0;
		dirno = Obs_Ln;
		break;
	case 4:
		if (op != 0 && op->oper_next == 0 && op->oper_flags == 0)
			ep = op->oper_expr;
		else
			ep = 0;
		if (strncmp((const char *)&s[1], "def", 3) == 0)
		{
			dirno = Obs_Def;
			dot_def(ep);
			break;
		}
		if (strncmp((const char *)&s[1], "scl", 3) == 0)
		{
			dirno = Obs_Scl;
			dot_scl(ep);
			break;
		}
		if (strncmp((const char *)&s[1], "val", 3) == 0)
		{
			dirno = Obs_Val;
			dot_val(ep);
			break;
		}
		if (strncmp((const char *)&s[1], "tag", 3) == 0)
		{
			dirno = Obs_Tag;
			break;
		}
		if (strncmp((const char *)&s[1], "dim", 3) == 0)
		{
			dirno = Obs_Dim;
			break;
		}
		return 0;
	case 5:
		if (strncmp((const char *)&s[1], "line", 4) != 0)
			return 0;
		dirno = Obs_Line;
		break;
	case 6:
		if (strncmp((const char *)&s[1], "endef", 5) == 0)
		{
			dirno = Obs_Endef;
			if (scl_negone && dot_def_expr != 0
				&& dot_val_expr != 0)
			{
				ep = dot_def_expr;
				typesym(ep->right.ex_sym,
					ulongexpr((Ulong)SymTy_Function));
				sizesym(ep->right.ex_sym,
					binaryexpr(ExpOp_Subtract,
					dot_val_expr, ep));
			}
			scl_negone = 0;
			dot_def_expr = 0;
			dot_val_expr = 0;
			break;
		}
		return 0;
	}
	if (!have_warned[dirno])	/* issue one-time diagnostic */
	{
		if (n != 0)
			warn("obsolete directive ignored: %s", prtstr(s, n));
		else
		{
			/* only .size or .type */
			warn("obsolete use of directive ignored: %s",
				(const char *)s);
		}
		have_warned[dirno] = 1;
	}
	return 1;
}
