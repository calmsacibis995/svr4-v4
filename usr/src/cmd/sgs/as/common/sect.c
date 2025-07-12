/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/sect.c	1.8"
/*
* common/sect.c - common assembler section handling
*/
#include <stdio.h>
#include <string.h>
#ifndef __STDC__
#  include <memory.h>
#endif
#include "common/as.h"
#include "common/expr.h"
#include "common/objf.h"
#include "common/nums.h"
#include "common/relo.h"
#include "common/sect.h"
#include "common/stmt.h"
#include "common/syms.h"
#include "align.h"

#define SECT_ASMGEN	0x01	/* section not available to users */
#define SECT_PREDEF	0x02	/* section predefined */
#define SECT_ATTR	0x04	/* attribute evaluated (as value, not expr) */
#define SECT_TYPE	0x08	/* type evaluated (as value, not expr) */
#define SECT_VARINST	0x10	/* contains variable-sized instruction(s) */

static struct	/* predefined section info */
{
	const char	*name;
	Uchar		flags;
	Ulong		type;
	Ulong		attr;
} const pds[] =	/* first is initial section, in desired order */
{
	{".text",	0,	SecTy_Progbits,	Attr_Alloc | Attr_Exec},
	{".init",	0,	SecTy_Progbits,	Attr_Alloc | Attr_Exec},
	{".fini",	0,	SecTy_Progbits,	Attr_Alloc | Attr_Exec},
	{".data",	0,	SecTy_Progbits,	Attr_Alloc | Attr_Write},
	{".data1",	0,	SecTy_Progbits,	Attr_Alloc | Attr_Write},
	{".rodata",	0,	SecTy_Progbits,	Attr_Alloc},
	{".rodata1",	0,	SecTy_Progbits,	Attr_Alloc},
	{".bss",	0,	SecTy_Nobits,	Attr_Alloc | Attr_Write},
	{".comment",	0,	SecTy_Progbits,	0},
	{".debug",	0,	SecTy_Progbits,	0},
	{".line",	0,	SecTy_Progbits,	0},
	{".note",	0,	SecTy_Note,	0},
	{".interp",	0,	SecTy_Progbits,	0},
	{".shstrtab",	0,	SecTy_Strtab,	0},
	{".dynstr",	0,	SecTy_Strtab,	Attr_Alloc},
	{".dynsym",	0,	SecTy_Dynsym,	Attr_Alloc},
	{".hash",	0,	SecTy_Hash,	Attr_Alloc},
	{".dynamic",	0,	SecTy_Dynamic,	Attr_Alloc | Attr_Write},
	{".got",	0,	SecTy_Progbits,	Attr_Alloc | Attr_Write},
	{".symtab",	SECT_ASMGEN,	SecTy_Symtab,	0},
	{".strtab",	SECT_ASMGEN,	SecTy_Strtab,	0},
	/*
	* ".plt" is not in this list because its attributes
	* are implementation-dependent (specifically, whether
	* the section is writable).
	*/
};

static Section predefs[sizeof(pds) / sizeof(pds[0])];
static Code precodes[sizeof(pds) / sizeof(pds[0])];

static Section *sections = &predefs[0]; /* linked list of all sections */
static Section *lastsect = &predefs[sizeof(pds) / sizeof(pds[0]) - 1];

typedef struct stk Stack;
struct stk	/* stack of sections */
{
	Section	*secp;	/* "current" section at this level */
	Section	*prev;	/* "previous" section at this level */
	Stack	*next;	/* "pop" to this Stack */
};

static Stack initial = {&predefs[0], &predefs[0], 0};
static Stack *current = &initial;

static Symbol *reloc_name;	/* entry for ".<reloc>" */

static const char MSGbig[] = "section too big: %s";

void
#ifdef __STDC__
initsect(void)	/* install all predefined sections */
#else
initsect()
#endif
{
	register Symbol *sp;
	int i;

	for (i = 0; i < sizeof(pds) / sizeof(pds[0]); i++)
	{
		sp = lookup((const Uchar *)pds[i].name,
			(size_t)strlen(pds[i].name));
		sp->sym_sect = &predefs[i];
		predefs[i].sec_next = &predefs[i + 1];
		predefs[i].sec_sym = sp;
		predefs[i].sec_flags = pds[i].flags
			| (SECT_PREDEF | SECT_ATTR | SECT_TYPE);
		predefs[i].attr.sec_value = pds[i].attr;
		predefs[i].type.sec_value = pds[i].type;
		predefs[i].sec_code = &precodes[i];
		predefs[i].sec_last = &precodes[i];
		precodes[i].code_kind = CodeKind_Unset;
	}
	predefs[i - 1].sec_next = 0;
	/*
	* The name for this symbol includes < and > to
	* prevent any conflicts with user-generated names.
	*/
	reloc_name = lookup((const Uchar *)".<reloc>", (size_t)8);
}

static void
#ifdef __STDC__
attrstr_check(Section *secp)	/* validate section attribute string */
#else
attrstr_check(secp)Section *secp;
#endif
{
	Expr *ep = setlessexpr(secp->attr.sec_expr);
	const Uchar *p = ep->right.ex_str;
	size_t len;
	Ulong attr = 0;

	for (len = ep->left.ex_len; len != 0; len--)
	{
		switch (*p++)	/* multiple occurrances of letters okay */
		{
		default:
			break;
		case 'a':
			attr |= Attr_Alloc;
			continue;
		case 'w':
			attr |= Attr_Write;
			continue;
		case 'x':
			attr |= Attr_Exec;
			continue;
		}
		exprerror(secp->attr.sec_expr,
			"invalid section attribute string: \"%s\"",
			prtstr(ep->right.ex_str, ep->left.ex_len));
		break;
	}
	secp->sec_flags |= SECT_ATTR;
	secp->attr.sec_value = attr;
}

static void
#ifdef __STDC__
typestr_check(Section *secp)	/* validate section type string */
#else
typestr_check(secp)Section *secp;
#endif
{
	Expr *ep = setlessexpr(secp->type.sec_expr);
	const char *p = (const char *)ep->right.ex_str;
	Ulong type = SecTy_Progbits;	/* default section type */

	switch (ep->left.ex_len)
	{
	default:
		p = 0;
		break;
	case 4:
		if (strncmp(p, "note", (size_t)4) == 0)
			type = SecTy_Note;
		else
			p = 0;
		break;
	case 6:
		if (strncmp(p, "strtab", (size_t)6) == 0)
			type = SecTy_Strtab;
		else if (strncmp(p, "symtab", (size_t)6) == 0)
			type = SecTy_Symtab;
		else if (strncmp(p, "nobits", (size_t)6) == 0)
			type = SecTy_Nobits;
		else
			p = 0;
		break;
	case 8:
		if (strncmp(p, "progbits", (size_t)8) != 0)
			p = 0;
		break;
	}
	if (p == 0)	/* no match above */
	{
		exprerror(secp->type.sec_expr,
			"invalid section type string: \"%s\"",
			prtstr(ep->right.ex_str, ep->left.ex_len));
	}
	secp->sec_flags |= SECT_TYPE;
	secp->type.sec_value = type;
}

static Section *
#ifdef __STDC__
newsect(Symbol *sp)	/* allocate new section; must append to list */
#else
newsect(sp)Symbol *sp;
#endif
{
	register struct sectcode /* holds a Section and its initial Code */
	{
		Section	s;
		Code	c;
	} *scp;
	static const Section empty = {0};

#ifdef DEBUG
	if (DEBUG('a') > 0)
	{
		static Ulong total;

		(void)fprintf(stderr,
			"Total struct sectcodes=%lu @%lu ea.\n",
			total += 1, (Ulong)sizeof(struct sectcode));
	}
#endif
	scp = (struct sectcode *)alloc((void *)0, sizeof(struct sectcode));
	scp->s = empty;
	sp->sym_sect = &scp->s;
	scp->s.sec_sym = sp;
	lastsect->sec_next = &scp->s;
	lastsect = &scp->s;
	scp->s.sec_code = &scp->c;
	scp->s.sec_last = &scp->c;
	scp->c.code_kind = CodeKind_Unset;
	return &scp->s;
}

void
#ifdef __STDC__
setsect(Symbol *sp, Expr *attr, Expr *type)	/* define/set section */
#else
setsect(sp, attr, type)Symbol *sp; Expr *attr, *type;
#endif
{
	register Section *secp;

#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "setsect(%s,attr=",
			(const char *)sp->sym_name);
		printexpr(attr);
		(void)fputs(",type=", stderr);
		printexpr(type);
		(void)fputs(")\n", stderr);
	}
#endif
	if ((secp = sp->sym_sect) == 0)
		secp = newsect(sp);
	else if (secp->sec_flags & SECT_ASMGEN)
	{
		error("assembler-generated section unavailable: %s",
			(const char *)sp->sym_name);
		return;
	}
	else if (attr != 0)	/* also a definition */
	{
		if (secp->sec_flags & SECT_PREDEF)	/* allow override */
		{
			secp->sec_flags &=
				~(SECT_PREDEF | SECT_ATTR | SECT_TYPE);
		}
		else
		{
			warn("section already defined: %s",
				(const char *)sp->sym_name);
			return;
		}
	}
	current->prev = current->secp;
	current->secp = secp;
	if (attr == 0)		/* nothing further to do unless definition */
		return;
	if (flags & ASFLAG_TRANSITION && type == 0) /* use obsolete rules */
	{
		secp->sec_flags |= (SECT_ATTR | SECT_TYPE);
		secp->attr.sec_value = 0;
		secp->type.sec_value = SecTy_Progbits;
		obssectattr(secp, attr);
	}
	else	/* note strings now, handle others later */
	{
		secp->attr.sec_expr = attr;
		if (attr->ex_type == ExpTy_String)
			attrstr_check(secp);
		else
			fixexpr(attr);
		if ((secp->type.sec_expr = type) != 0)
		{
			if (type->ex_type == ExpTy_String)
				typestr_check(secp);
			else
				fixexpr(type);
		}
	}
}

Section *
#ifdef __STDC__
relosect(Section *secp, int type) /* create matching relocation section */
#else
relosect(secp, type)Section *secp; int type;
#endif
{
	register Section *new;

#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "relosect(%s,type=%d)\n",
			(const char *)secp->sec_sym->sym_name, type);
	}
#endif
	if (secp->sec_relo != 0)
		fatal("relosect():already has relocation section");
	new = newsect(reloc_name);
	new->sec_relo = secp;
	secp->sec_relo = new;
	new->sec_flags = SECT_ASMGEN | SECT_ATTR | SECT_TYPE;
	new->attr.sec_value = 0;
	new->type.sec_value = type;
	return new;
}

void
#ifdef __STDC__
prevsect(void)	/* toggle between current and previous sections */
#else
prevsect()
#endif
{
	Section *tmp;

	tmp = current->prev;
	current->prev = current->secp;
	current->secp = tmp;
}

Section *
#ifdef __STDC__
cursect(void)	/* return current section */
#else
cursect()
#endif
{
	return current->secp;
}

int
#ifdef __STDC__
stacksect(Section *secp)	/* push/pop current section */
#else
stacksect(secp)Section *secp;
#endif
{
	static Stack *avail;
	register Stack *tmp;

	if (secp == 0)	/* pop */
	{
		if (current->next == 0)
			return 0;
		else
		{
			tmp = current;
			current = current->next;
			tmp->next = avail;
			avail = tmp;
		}
	}
	else	/* push */
	{
		if ((tmp = avail) != 0)
			avail = tmp->next;
		else
		{
#ifdef DEBUG
			if (DEBUG('a') > 0)
			{
				static Ulong total;

				(void)fprintf(stderr,
					"Total Stacks=%lu @%lu ea.\n",
					total += 1, (Ulong)sizeof(Stack));
			}
#endif
			tmp = (Stack *)alloc((void *)0, sizeof(Stack));
		}
		tmp->secp = secp;
		tmp->prev = secp;
		tmp->next = current;
		current = tmp;
	}
	return 1;
}

#ifndef ALIGN_IS_POW2
static Ulong
#ifdef __STDC__
lcm(Ulong u0, Ulong v0)	/* return the least common multiple of u0,v0 */
#else
lcm(u0, v0)Ulong u0, v0;
#endif
{
	register Ulong t, u = u0, v = v0;
	register int neg, nbits;

	/*
	* Adapted from "Algorithm B" from Knuth Vol. 2, 4.5.2.,
	* using a flag instead of negative values.
	*/
	for (nbits = 0; ((u | v) & 0x1) == 0; nbits++)
	{
		u >>= 1;
		v >>= 1;
	}
	if ((neg = u & 0x1) != 0)
		t = v;
	else
		t = u;
	for (;;)
	{
		while ((t & 0x1) == 0)
			t >>= 1;
		if (neg)
			u = t;
		else
			v = t;
		if (u > v)
		{
			t = u - v;
			neg = 0;
		}
		else if (u == v)
			break;	/* only way out */
		else
		{
			t = v - u;
			neg = 1;
		}
	}
	/*
	* At this point, u << nbits is GCD(u0, v0).
	* LCM(u0, v0) is (u0 * v0) / GCD(u0, v0).
	*/
	return (u0 / (u << nbits)) * v0;
}
#endif	/*ALIGN_IS_POW2*/

static void
#ifdef __STDC__
setvarsz(Section *secp)	/* choose sizes for varying-sized instructions */
#else
setvarsz(secp)Section *secp;
#endif
{
	register Code *cp;
	register size_t sz;
	Code *topvar = secp->sec_code;
	Code *topchg;
#ifdef DEBUG
	int passno = 0;		/* count of passes with changes */

	if (DEBUG('v') > 0)
	{
		(void)fprintf(stderr, "VarSize:section=%s\n",
			(const char *)secp->sec_sym->sym_name);
	}
#endif
	/*
	* Walk through the contents of the section repeatedly until the
	* sizes of all CodeKind_VarInst's have stopped changing.
	*
	* This algorithm requires that encodings for instructions never
	* get smaller.  (Otherwise, it might never converge.)  It also
	* relies on the restriction imposed by the .set (of .)
	* directive: that it can only be based on a label before or at
	* the current code.  (This means that a CodeKind_Pad will never
	* reduce the size of a section.)
	*
	* The following is a nonoptimal compromise algorithm that
	* requires far fewer interations than the optimal algorithm.
	* In practice, it is at least one or two orders of magnitude
	* faster.
	*
	* The algorithm:
	*  1. Find the next varying-sized instruction.
	*  2. Determine the new sizes for all subsequent variable-sized
	*     instructions.  Note that since the addresses are not
	*     changed in this pass, some instructions may become bigger
	*     than may be necessary.
	*  3. Sweep though from just after the first variable-sized
	*     instruction to the end of the section updating addresses.
	*  4. Go back to step 1.
	* Once either step 1 or 2 finds no candidate, the process is
	* complete.
	*/
	for (;;)
	{
		/*
		* 1. Find first CodeKind_VarInst still in section.
		*/
		for (cp = topvar; cp != 0; cp = cp->code_next)
		{
			if (cp->code_kind == CodeKind_VarInst)
				break;
		}
		if ((topvar = cp) == 0)
			break;		/* no more sizes to choose */
		/*
		* 2. For each CodeKind_VarInst, check for a size change,
		*    remembering the first that changed size.
		*/
		topchg = 0;
		do		/* check each VarInst's code_size */
		{
			if (cp->code_kind != CodeKind_VarInst)
				continue;
			if ((sz = (*cp->info.code_inst->inst_gen)(secp, cp))
				== cp->code_size)
			{
				continue;
			}
			if (sz < cp->code_size)
				fatal("setvarsz():smaller varinst encoding");
			if (topchg == 0)	/* first change */
			{
				topchg = cp;
#ifdef DEBUG
				if (DEBUG('v') > 0)
				{
					(void)fprintf(stderr,
						"VarSize:pass %d\n", ++passno);
				}
#endif
			}
#ifdef DEBUG
			if (DEBUG('v') > 1)
			{
				const Inst *ip = cp->info.code_inst;

				(void)fprintf(stderr,
					"\tcp=%#lx: %s %lu[%u] -> [%lu]\n",
					(Ulong)cp, (const char *)ip->inst_name,
					cp->code_addr, (Uint)cp->code_size,
					(Ulong)sz);
			}
#endif
			if ((cp->code_size = sz) != sz)
				fatal("setvarsz():varinst encoding too big");
		} while ((cp = cp->code_next) != 0);
		/*
		* 3. Starting with the first CodeKind_VarInst that grew,
		*    update all the addresses.  CodeKind_{Align,Pad} are
		*    the tricky ones here.
		*/
		if ((cp = topchg) == 0)
			break;		/* no size changes */
		sz = cp->code_addr + cp->code_size;
#ifdef DEBUG
		if (DEBUG('v') > 1)
		{
			(void)fprintf(stderr,
				"VarSize:Address changes (start %#lx):\n",
				(Ulong)sz);
		}
#endif
		for (;;)	/* update each code_addr */
		{
			cp = cp->code_next;
#ifdef DEBUG
			if (DEBUG('v') > 1)
			{
				const char *str = "?HUH?";

				switch (cp->code_kind)
				{
				case CodeKind_Unset:
					str = "UNSET";
					break;
				case CodeKind_FixInst:
				case CodeKind_VarInst:
					str = (const char *)
						cp->info.code_inst->inst_name;
					break;
				case CodeKind_Expr:
					str = "EXPR";
					break;
				case CodeKind_Align:
					str = "ALIGN";
					break;
				case CodeKind_Pad:
					str = "PAD";
					break;
				case CodeKind_Zero:
					str = "ZERO";
					break;
				case CodeKind_String:
					str = "STRING";
					break;
				}
				(void)fprintf(stderr,
					"\tcp=%#lx: %s %lu becomes %lu\n",
					(Ulong)cp, str, cp->code_addr,
					(Ulong)sz);
			}
#endif
			if (cp->code_addr > sz)	/* check for overflow */
			{
				error(MSGbig,
					(const char *)secp->sec_sym->sym_name);
			}
			cp->code_addr = sz;
			switch (cp->code_kind)
			{
			default:
				fatal("setvarsz():unknown code kind: %u",
					(Uint)cp->code_kind);
				/*NOTREACHED*/
			case CodeKind_RelSym:
			case CodeKind_RelSec:
				fatal("setvarsz():bad code kind (reloc)");
				/*NOTREACHED*/
			case CodeKind_Unset:
				break;
			case CodeKind_Align:
#ifdef ALIGN_IS_POW2
				if ((sz & (cp->data.code_skip - 1)) != 0)
				{
					sz |= cp->data.code_skip - 1;
					sz++;
				}
#else
				{
					register Ulong rem;

					if ((rem = sz % cp->data.code_skip)
						!= 0)
					{
						sz += cp->data.code_skip - rem;
					}
				}
#endif
				continue;
			case CodeKind_Pad:
				sz = cp->info.code_base->code_addr
					+ cp->data.code_skip;
				continue;
			case CodeKind_Zero:
				sz += cp->data.code_skip;
				continue;
			case CodeKind_FixInst:
			case CodeKind_VarInst:
			case CodeKind_Expr:
			case CodeKind_String:
				sz += cp->code_size;
				continue;
			}
			break;	/* only here if is CodeKind_Unset */
		}
	}
	/*
	* Finished.  Set resulting section size.
	*/
	if ((sz = secp->sec_last->code_addr) < secp->sec_size)
		error(MSGbig, (const char *)secp->sec_sym->sym_name);
#ifdef DEBUG
	if (DEBUG('v') > 0)
	{
		(void)fprintf(stderr,
			"VarSize:%s grows from %lu to %lu bytes\n",
			(const char *)secp->sec_sym->sym_name,
			secp->sec_size, sz);
	}
#endif
	secp->sec_size = sz;
}

void
#ifdef __STDC__
walksect(void)	/* preliminary scan through all sections */
#else
walksect()
#endif
{
	register Section *secp;
	size_t nsects = 0;	/* number of sections with content */
	size_t nschars = 0;	/* amount of additional strtab space */

	/*
	* Give each section an index and handle variable-sized
	* instructions here.
	*/
	secp = sections;
	do
	{
		if (secp->sec_code == secp->sec_last)
			continue;
		secp->sec_index = ++nsects;
		nschars += secp->sec_sym->sym_nlen + 1;
		if (secp->sec_flags & SECT_VARINST)
			setvarsz(secp);	/* choose sizes for varinsts */
	} while ((secp = secp->sec_next) != 0);
	objfmksect(nsects, nschars);	/* prime object file sections */
}

static void
#ifdef __STDC__
attr_check(Section *secp) /* validate section attribute value/string */
#else
attr_check(secp)Section *secp;
#endif
{
	Ulong attr = 0;		/* default attributes */
	Value *vp;
	Expr *ep;

	if ((ep = secp->attr.sec_expr) != 0)
	{
		if (ep->ex_type == ExpTy_String)
		{
			attrstr_check(secp);
			return;
		}
		if (ep->ex_type != ExpTy_Integral)
		{
			fatal("attr_check():non-str/int section attr: %s",
				(const char *)secp->sec_sym->sym_name);
		}
		vp = evalexpr(ep);
		/*
		* No need for a reevaluation check as all addresses have
		* been fixed by now.
		*/
		if (vp->val_flags & (VAL_OFLOW | VAL_TRUNC))
		{
			exprerror(ep, "section attribute out of range: %s",
				num2hex(vp->val_num));
		}
	}
	secp->sec_flags |= SECT_ATTR;
	secp->attr.sec_value = attr;
}

static void
#ifdef __STDC__
type_check(Section *secp) /* validate section type value/string */
#else
type_check(secp)Section *secp;
#endif
{
	Ulong type = SecTy_Progbits;	/* default section type */
	Value *vp;
	Expr *ep;

	if ((ep = secp->type.sec_expr) != 0)
	{
		if (ep->ex_type == ExpTy_String)
		{
			typestr_check(secp);
			return;
		}
		if (ep->ex_type != ExpTy_Integral)
		{
			fatal("type_check():non-str/int section type: %s",
				(const char *)secp->sec_sym->sym_name);
		}
		vp = evalexpr(ep);
		/*
		* No need for a reevaluation check as all addresses have
		* been fixed by now.
		*/
		if (vp->val_flags & (VAL_OFLOW | VAL_TRUNC))
		{
			exprerror(ep, "section type out of range: %s",
				num2hex(vp->val_num));
		}
	}
	secp->sec_flags |= SECT_TYPE;
	secp->type.sec_value = type;
}

static void
#ifdef __STDC__
gencode(Section *secp)	/* generate one section's content */
#else
gencode(secp)Section *secp;
#endif
{
	static const char MSGzby[] = "section contains only zero bytes: %s";
	register Code *cp;
	register Value *vp;
	register Uchar *base;
	register int exec = 0;
	register int nobits = 0;

	/*
	* Walk through all data for the section.
	* If this item is an instruction, hand it off to the
	* appropriate encoding function.  If a section is
	* executable, pad with nop's instead of zeroes.
	*/
	if (secp->attr.sec_value & Attr_Exec)
		exec = 1;
	if (secp->type.sec_value == SecTy_Nobits)
		nobits = 1;
	else
	{
		base = (Uchar *)alloc((void *)0, secp->sec_size);
		secp->sec_data = base;
	}
	for (cp = secp->sec_code;; cp = cp->code_next)
	{
		/*
		* The "?check?" comments flag those places in which
		* the bytes to be generated could be checked for
		* explicit zero values and silently let such go by
		* for SecTy_Nobits sections.
		*/
		switch (cp->code_kind)
		{
		default:
			fatal("gencode():unknown code kind: %u",
				(Uint)cp->code_kind);
			/*NOTREACHED*/
		case CodeKind_RelSym:
		case CodeKind_RelSec:
			fatal("gencode():inappropriate code kind (reloc): %u",
				(Uint)cp->code_kind);
			/*NOTREACHED*/
		case CodeKind_Unset:
			return;		/* ends list of contents */
		case CodeKind_FixInst:
		case CodeKind_VarInst:
			if (nobits)
				break;
			if ((*cp->info.code_inst->inst_gen)(secp, cp)
				!= cp->code_size)
			{
				fatal("gencode():\"%s\" not expected size: %u",
					(const char *)cp->info.code_inst
						->inst_name,
					(Uint)cp->code_size);
			}
			continue;
		case CodeKind_Expr:
			vp = evalexpr(cp->data.code_expr);
			if (nobits)	/* allow explicit simple zeroes */
			{
				if (vp->val_flags & (VAL_OFLOW | VAL_TRUNC
					| VAL_FLOAT | VAL_RELOC)
					|| vp->val_ulong != 0)
				{
					break;
				}
			}
			else if (vp->val_flags & VAL_RELOC)
			{
				relocexpr(vp, cp, secp);
			}
			else
			{
				val2data(vp, cp, secp);
			}
			continue;
		case CodeKind_String:
			if (nobits)	/*?check?*/
				break;
			(void)memcpy((void *)(base + cp->code_addr),
				(const void *)cp->data.code_str,
				(size_t)cp->code_size);
			continue;
		case CodeKind_Pad:
			/*
			* It is possible, due to variable-sized instructions,
			* to have a .set of . move backwards.  However, this
			* is very unlikely.  The following check is placed
			* here [instead of in setvarsz()] because some
			* subsequent step might fix a temporary overlap
			* condition.
			*/
			if (cp->code_addr > cp->code_next->code_addr)
			{
				exprerror(cp->data.code_expr,
					"backward .set of . causes overlap");
				continue;
			}
			/*FALLTHROUGH*/
		case CodeKind_Align:
			/*
			* By now, the old value in data.code_skip has
			* been used to determine the address for the
			* next code.  Convert it to the distance between
			* here and the next code, ignoring a distance
			* of zero.
			*/
			if ((cp->data.code_skip = cp->code_next->code_addr
				- cp->code_addr) == 0)
			{
				continue;
			}
			if (exec)
			{
				if (nobits)
					break;	/* "nop"s probably not 0 */
				gennops(secp, cp);
				continue;
			}
			/*FALLTHROUGH*/
		case CodeKind_Zero:
			if (!nobits)
			{
				(void)memset((void *)(base + cp->code_addr),
					0, (size_t)cp->data.code_skip);
			}
			continue;
		}
		/*
		* Only here when attempting to generate (possibly)
		* nonzero byte(s) for nobits section.
		*/
		exprerror(cp->data.code_expr, MSGzby,
			(const char *)secp->sec_sym->sym_name);
	}
}

void
#ifdef __STDC__
gensect(void)	/* generate contents of all sections */
#else
gensect()
#endif
{
	register Section *secp;

	/*
	* For each user section:
	* 1. Finalize its section type and attributes.
	* 2. Update the minimum alignments.
	* 3. Allocate (if not nobits) and fill in data.
	* 4. Hand the result to the object file producer.
	*/
	secp = sections;
	do
	{
		if (secp->sec_index == 0)
			continue;
		if (!(secp->sec_flags & SECT_ATTR))
			attr_check(secp);
		if (!(secp->sec_flags & SECT_TYPE))
			type_check(secp);
#if EXEC_SECT_ALIGN > 1
#  ifdef ALIGN_IS_POW2
		if (secp->attr.sec_value & Attr_Exec)
		{
			if (secp->sec_align < EXEC_SECT_ALIGN)
				secp->sec_align = EXEC_SECT_ALIGN;
		}
#  else
		if (secp->attr.sec_value & Attr_Exec)
		{
			if (secp->sec_align % EXEC_SECT_ALIGN != 0)
			{
				secp->sec_align = lcm(secp->sec_align,
					EXEC_SECT_ALIGN);
			}
		}
#  endif
#endif	/*EXEC_SECT_ALIGN*/
#if ALLO_SECT_ALIGN > 1
#  ifdef ALIGN_IS_POW2
		if (secp->attr.sec_value & Attr_Alloc)
		{
			if (secp->sec_align < ALLO_SECT_ALIGN)
				secp->sec_align = ALLO_SECT_ALIGN;
		}
#  else
		if (secp->attr.sec_value & Attr_Alloc)
		{
			if (secp->sec_align % ALLO_SECT_ALIGN != 0)
			{
				secp->sec_align = lcm(secp->sec_align,
					ALLO_SECT_ALIGN);
			}
		}
#  endif
#endif	/*ALLO_SECT_ALIGN*/
		gencode(secp);
		objfsection(secp);
	} while ((secp = secp->sec_next) != 0);
}

static void
#ifdef __STDC__
newcode(Section *secp)	/* allocate new code for section */
#else
newcode(secp)Section *secp;
#endif
{
	static Code *avail, *endavail;
	register Code *cp;

	/*
	* The "next" code for each section is always pointed to by
	* its current sec_last pointer.  This allows the value for
	* labels and . to be produced before the corresponding Code
	* has been filled.
	*/
#ifndef ALLOC_CODE_CHUNK
#define ALLOC_CODE_CHUNK 2000
#endif
	if ((cp = avail) == endavail)
	{
#ifdef DEBUG
		if (DEBUG('a') > 0)
		{
			static Ulong total;

			(void)fprintf(stderr, "Total Codes=%lu @%lu ea.\n",
				total += ALLOC_CODE_CHUNK,
				(Ulong)sizeof(Code));
		}
#endif
		avail = cp = (Code *)alloc((void *)0,
			ALLOC_CODE_CHUNK * sizeof(Code));
		endavail = cp + ALLOC_CODE_CHUNK;
	}
	avail = cp + 1;
	cp->code_next = 0;
	cp->code_addr = secp->sec_size;
	cp->code_kind = CodeKind_Unset;
	cp->code_impdep = 0;
	secp->sec_last->code_next = cp;
	secp->sec_last = cp;
}

void
#ifdef __STDC__
sectalign(register Section *secp, Ulong align)	/* add align request */
#else
sectalign(secp, align)register Section *secp; Ulong align;
#endif
{
	register Code *cp;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectalign(%s,align=%lu)\n",
			(const char *)secp->sec_sym->sym_name, align);
	}
#endif
	cp = secp->sec_last;
	cp->code_kind = CodeKind_Align;
	cp->data.code_skip = align;
#ifdef ALIGN_IS_POW2
	if (secp->sec_align < align)
		secp->sec_align = align;
	if ((secp->sec_size & (align - 1)) != 0) /* power of two */
	{
		secp->sec_size |= align - 1;
		if (++secp->sec_size == 0)
			error(MSGbig, (const char *)secp->sec_sym->sym_name);
	}
#else
	{
		Ulong rem;

		if (secp->sec_align % align != 0)
			secp->sec_align = lcm(secp->sec_align, align);
		if ((rem = secp->sec_size % align) != 0)
		{
			align -= rem;
			if ((secp->sec_size += align) < align)
			{
				error(MSGbig,
					(const char *)secp->sec_sym->sym_name);
			}
		}
	}
#endif
	newcode(secp);
}

int
#ifdef __STDC__
checkalign(register Section *secp, Ulong align) /* test alignment and fix */
#else
checkalign(secp, align)register Section *secp; Ulong align;
#endif
{
	register int currently_aligned = 0;	/* default to "no" */

	if (secp == 0)
		secp = current->secp;
#ifdef ALIGN_IS_POW2
	if ((secp->sec_size & (align - 1)) == 0) /* power of two */
	{
		if (secp->sec_align < align)
			secp->sec_align = align;
		if (!(secp->sec_flags & SECT_VARINST))
			return 1;	/* nothing further need be done */
		currently_aligned = 1;
	}
#else
	if (secp->sec_size % align == 0)
	{
		if (secp->sec_align % align != 0)
			secp->sec_align = lcm(secp->sec_align, align);
		if (!(secp->sec_flags & SECT_VARINST))
			return 1;	/* nothing further need be done */
		currently_aligned = 1;
	}
#endif
	sectalign(secp, align);
	return currently_aligned;
}

void
#ifdef __STDC__
sectzero(register Section *secp, Ulong size)	/* add zero-valued bytes */
#else
sectzero(secp, size)register Section *secp; Ulong size;
#endif
{
	register Code *cp;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectzero(%s,size=%lu)\n",
			(const char *)secp->sec_sym->sym_name, size);
	}
#endif
	cp = secp->sec_last;
	cp->code_kind = CodeKind_Zero;
	cp->data.code_skip = size;
	if ((secp->sec_size += size) < size)
		error(MSGbig, (const char *)secp->sec_sym->sym_name);
	newcode(secp);
}

void
#ifdef __STDC__
sectpad(register Section *secp, register Value *addr) /* add padding bytes */
#else
sectpad(secp, addr)register Section *secp; register Value *addr;
#endif
{
	register Code *cp;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr,
			"sectpad(%s,addr->val_dot=%#lx/_ulong=%lu)\n",
			(const char *)secp->sec_sym->sym_name,
			(Ulong)addr->val_dot, addr->val_ulong);
	}
#endif
	cp = secp->sec_last;
	cp->code_kind = CodeKind_Pad;
	secp->sec_size = addr->val_ulong;	/* new value for . */
	if ((cp->info.code_base = addr->val_dot) == 0)
		fatal("sectpad():null val_dot");
	cp->data.code_skip = addr->val_ulong - addr->val_dot->code_addr;
	newcode(secp);
}

void
#ifdef __STDC__
sectexpr(register Section *secp, Expr *ep, Ulong size, int form) /* add expr */
#else
sectexpr(secp, ep, size, form)
	register Section *secp; Expr *ep; Ulong size; int form;
#endif
{
	register Code *cp;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectexpr(%s,ep=",
			(const char *)secp->sec_sym->sym_name);
		printexpr(ep);
		(void)fprintf(stderr, ",size=%lu)\n", size);
	}
#endif
	cp = secp->sec_last;
	cp->code_kind = CodeKind_Expr;
	cp->data.code_expr = ep;
	cp->info.code_form = form;
	if ((secp->sec_size += size) < size)
		error(MSGbig, (const char *)secp->sec_sym->sym_name);
	cp->code_size = size;	/* assumed to fit */
	newcode(secp);
}

void
#ifdef __STDC__
sectstr(register Section *secp, Expr *ep)	/* add string value */
#else
sectstr(secp, ep)register Section *secp; Expr *ep;
#endif
{
	register Code *cp;
	register const Uchar *p;
	register size_t curlen;

	if (secp == 0)
		secp = current->secp;
	/*
	* Because strings can be longer than can be held in
	* code_size, break strings up into small-enough chunks.
	*/
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectstr(%s,ep=",
			(const char *)secp->sec_sym->sym_name);
		printexpr(ep);
		(void)fputs(")\n", stderr);
	}
#endif
	curlen = ep->left.ex_len;
	p = ep->right.ex_str;
	for (;;)	/* loop only while curlen > 0xffff */
	{
		cp = secp->sec_last;
		cp->code_kind = CodeKind_String;
		cp->data.code_str = p;
		if (curlen <= 0xffffL)	/* at least 16 bits in Ushort */
		{
			cp->code_size = curlen;
			if ((secp->sec_size += curlen) < curlen)
			{
				error(MSGbig,
					(const char *)secp->sec_sym->sym_name);
			}
			newcode(secp);
			break;
		}
		cp->code_size = 0xffffL;
		p += 0xffffL;
		curlen -= 0xffffL;
		if ((secp->sec_size += 0xffffL) < 0xffffL)
			error(MSGbig, (const char *)secp->sec_sym->sym_name);
		newcode(secp);
	}
}

static Code *
#ifdef __STDC__
sectinst(register Section *secp, register const Inst *ip, Oplist *olp)
#else
sectinst(secp, ip, olp)register Section *secp; register Inst *ip; Oplist *olp;
#endif
{
	register Code *cp;

	cp = secp->sec_last;
	cp->info.code_inst = ip;
	if (olp == 0)		/* check for operand-less instruction */
		olp = oplist((Oplist *)0, (Operand *)0);
	cp->data.code_olst = olp;
	if ((secp->sec_size += ip->inst_minsz) < ip->inst_minsz)
		error(MSGbig, (const char *)secp->sec_sym->sym_name);
	cp->code_size = ip->inst_minsz;
	olp->olst_code = cp;
	newcode(secp);
	return cp;
}

void
#ifdef __STDC__
sectfinst(Section *secp, const Inst *ip, Oplist *olp)	/* add fixed-inst */
#else
sectfinst(secp, ip, olp)Section *secp; Inst *ip; Oplist *olp;
#endif
{
	Code *cp;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectfinst(%s,%s,olp=",
			(const char *)secp->sec_sym->sym_name,
			(const char *)ip->inst_name);
		printoplist(olp);
		(void)fputs(")\n", stderr);
	}
#endif
	cp = sectinst(secp, ip, olp);
	cp->code_kind = CodeKind_FixInst;
}

void
#ifdef __STDC__
sectvinst(Section *secp, const Inst *ip, Oplist *olp)	/* add var-inst */
#else
sectvinst(secp, ip, olp)Section *secp; Inst *ip; Oplist *olp;
#endif
{
	Code *cp;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectvinst(%s,%s,olp=",
			(const char *)secp->sec_sym->sym_name,
			(const char *)ip->inst_name);
		printoplist(olp);
		(void)fputs(")\n", stderr);
	}
#endif
	cp = sectinst(secp, ip, olp);
	cp->code_kind = CodeKind_VarInst;
	secp->sec_flags |= SECT_VARINST;
}

	/*
	* sectrelsec() and sectrelsym() differ from the rest of the
	* Code-filling functions in a number of ways.  These two are
	* only called to fill in the contents of relocation sections.
	* These contents are then only rescanned by common/objf.c
	* instead of here.  Moreover, some of the structure members
	* are used in differing ways:
	*
	*	info.code_{sec,sym}	base for the entry
	*	code_size		the relocation type
	*	code_addr		relocation to be applied here
	*	data.{setadd,addend}	optional addend for relocation
	*
	*	sec_size		number of relocation entries
	*/
Code *
#ifdef __STDC__
sectrelsec(register Section *rsp, int rty, Ulong addr, Section *base)
#else
sectrelsec(rsp, rty, addr, base)
	register Section *rsp; int rty; Ulong addr; Section *base;
#endif
{
	register Code *cp;

#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectrelsec(%s,rty=%d,addr=%#lx,%s)\n",
			(const char *)rsp->sec_sym->sym_name, rty, addr,
			(const char *)base->sec_sym->sym_name);
	}
#endif
	cp = rsp->sec_last;
	cp->code_kind = CodeKind_RelSec;
	cp->info.code_sec = base;
	cp->code_size = rty;	/* sorry about using code_size for this */
	rsp->sec_size++;	/* number of entries, not the size */
	newcode(rsp);
	cp->code_addr = addr;	/* overwrite newcode()'s code_addr value */
	return cp;
}

Code *
#ifdef __STDC__
sectrelsym(register Section *rsp, int rty, Ulong addr, Symbol *base)
#else
sectrelsym(rsp, rty, addr, base)
	register Section *rsp; int rty; Ulong addr; Symbol *base;
#endif
{
	register Code *cp;

#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectrelsym(%s,rty=%d,addr=%#lx,%s)\n",
			(const char *)rsp->sec_sym->sym_name, rty, addr,
			(const char *)base->sym_name);
	}
#endif
	cp = rsp->sec_last;
	cp->code_kind = CodeKind_RelSym;
	cp->info.code_sym = base;
	cp->code_size = rty;	/* sorry about using code_size for this */
	rsp->sec_size++;	/* number of entries, not the size */
	newcode(rsp);
	cp->code_addr = addr;	/* overwrite newcode()'s code_addr value */
	return cp;
}
