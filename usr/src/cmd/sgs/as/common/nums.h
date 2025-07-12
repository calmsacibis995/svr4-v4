/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/nums.h	1.4"
/*
* common/nums.h - common assembler numeric header
*
* Depends on:
*	"common/as.h"
*/

#define VAL_OFLOW	0x01	/* overflow during evaluation */
#define VAL_TRUNC	0x02	/* val_ulong has truncated value */
#define VAL_RELOC	0x04	/* val_{sec,sym,dot,pic,mask} have values */
#define VAL_LDIFF	0x08	/* evaluation included symbol subtraction */
#define VAL_FLOAT	0x10	/* val_flt points to result */

/* Note: VAL_TRUNC is set iff val_unbits > CHAR_BIT * sizeof(Ulong) */

struct t_valu_	/* result of an expression evaluation */
{
	const Number	*val_num;	/* value (offset from base) */
	struct _fp_x_t	*val_flt;	/* floating value */
	const Expr	*val_expr;	/* the evaluated expression */
	Section		*val_sec;	/* base section for value */
	Symbol		*val_sym;	/* symbol defined in section */
	Code		*val_dot;	/* referenced code for . equiv. */
	Ulong		val_ulong;	/* (truncated) value of val_num */
	Uint		val_flags;	/* VAL_* bits */
	int		val_snbits;	/* minimum needed if signed */
	int		val_unbits;	/* minimum needed if unsigned */
	int		val_minbits;	/* minimum of val_{s,u}nbits */
	int		val_pic;	/* ExpOp_Pic_* applied */
	int		val_mask;	/* ExpOp_{Low,High,HighAdjust} */
};

	/*
	* If VAL_LDIFF is set, the evaluation included at least one
	* subtraction of relocatable subexpressions.  The resulting
	* difference depends on the current addresses of the bases
	* for those two relocatable subexpressions.  If addresses
	* have not yet been fixed [walksect() has not yet finished],
	* the evaluation can be put on a list for double checking at
	* the end of processing by using delayeval().
	*
	* Only a relocatable evaluation (val_flags & VAL_RELOC) has
	* meaningful values for val_sec, val_sym, val_dot, val_pic,
	* and val_mask.  The behavior of val_sec, val_sym, val_dot,
	* and val_num for relocatable values is as follows:
	*
	* val_sec
	*	is zero iff the relocation is based on an undefined symbol.
	* val_sym (and val_dot)
	*	is zero iff the relocation is based on some value of . (dot).
	* val_dot
	*	is zero iff the relocation is based on an undefined symbol.
	* val_num (val_ulong)
	*	is the (entire) offset from the beginning of the section if
	*	val_sec is nonzero; otherwise, it is the offset from val_sym.
	*
	* If both val_sec and val_sym are nonzero, and val_sym is not a .set
	* symbol, the offset from val_sym (instead of from val_sec) can be
	* calculated by subtracting [subvalue()] the address of the label
	* (val_dot->code_addr) from the current value.
	*
	* Normally, any use of .set symbols is lost during the evaluation of
	* the expression.  However, if the .set symbol is the subject of a
	* @GOT or @PLT operator, the .set symbol overrides the val_sym of the
	* .set symbol's replacement operand.
	*/

#ifdef __STDC__
void	initnums(void);				/* init number package */
Number	*mknum(const Uchar *, size_t);		/* to internal integral */
int	num2ulong(Ulong *, const Number *);	/* integral to Ulong */
Number	*ulong2num(Ulong);			/* Ulong to integral */
Number	*newnum(const Number *, Ulong);		/* allocate number copy */
char	*num2hex(const Number *);		/* printable hex version */
char	*num2dec(const Number *);		/* printable decimal vers */
void	val2data(Value *, const Code *, Section *);/* put value into data */
Value	*evalexpr(const Expr *);		/* eval int/flt/rel expr */
void	subvalue(Value *, Ulong);		/* Value -= Ulong */
void	delayeval(Value *);			/* reeval Value at end */
void	reevaluate(void);			/* do all reevals */

		/* implementation provides */
void	bcd2data(Uchar *, Uint n, Value *);	/* put bcd value into data */
#else
void	initnums();
Number	*mknum();
int	num2ulong();
Number	*ulong2num(), *newnum();
char	*num2hex(), *num2dec();
void	val2data();
Value	*evalexpr();
void	subvalue(), delayeval(), reevaluate();

void	bcd2data();
#endif
