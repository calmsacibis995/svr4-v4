/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/nums.c	1.10"
/*
* common/nums.c - common assembler numeric handling
*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#ifndef __STDC__
#  include <memory.h>
#endif
#include "common/as.h"
#include "common/expr.h"
#include "common/nums.h"
#include "common/sect.h"
#include "common/syms.h"

#define LITTLE_ENDIAN	1	/* low address contains least significant */
#define BIG_ENDIAN	2	/* low address contains most significant */

#ifndef ULONG_MAX
#  define ULONG_MAX (~(Ulong)0)	/* maximum Ulong value */
#endif

static Ulong mask[sizeof(Ulong) * CHAR_BIT + 1] = /* Ulong has >= 32 bits */
{
	MASK(0),  MASK(1),  MASK(2),  MASK(3),
	MASK(4),  MASK(5),  MASK(6),  MASK(7),
	MASK(8),  MASK(9),  MASK(10), MASK(11),
	MASK(12), MASK(13), MASK(14), MASK(15),
	MASK(16), MASK(17), MASK(18), MASK(19),
	MASK(20), MASK(21), MASK(22), MASK(23),
	MASK(24), MASK(25), MASK(26), MASK(27),
	MASK(28), MASK(29), MASK(30), MASK(31),
};

	/*
	* Integral numbers are stored as a fixed-length array of values
	* in a two's complement form, with the lowest order bits stored
	* in the first array element.  Each unit (or packet) of the
	* array is an unsigned integral type called Packet.  At most
	* half the bits in each packet are used to hold the values, as
	* this simplifies certain mathematical operations on the
	* whole array (e.g. umultiply, udivrem).  Moreover, the number
	* of bits used in a packet is forced to be a multiple of four.
	*
	* Requirements:
	*  1. NUMBITS cannot specify more bits than are in a Ulong
	*     and is at least 4.
	*  2. TOTNUMBITS (the total number of bits in a number) is a
	*     positive multiple of NUMBITS.
	*/
#include "packet.h"	/* implementation-specific choices */

#ifdef FLOATING_EXPRS	/* floating emulation package needed */
#   include "fpemu.h"
#endif

#define NUMBITS	((sizeof(Packet) * CHAR_BIT / 2) / 4 * 4)
#define NUMLEN	(TOTNUMBITS / NUMBITS)	/* number of packets in a number */

struct t_nums_	/* integral values */
{
	Packet	pkt[NUMLEN];
};

static Number prenums[256];	/* 0 through ... cached numbers */
static Number low16;		/* the value 0xffff */
static Number ten_9;		/* 1000000000 == 10**9 */

#define NBMASK		MASK(NUMBITS)	/* significant bits in a packet */
#define ISSET(a, n)	((a)[(n) / NUMBITS] & BIT((n) % NUMBITS))
#define NUMSIGN(a)	ISSET(a, TOTNUMBITS - 1)

static void
#ifdef __STDC__
num32(register Number *np, register Ulong val) /* convert up to 32 bits */
#else
num32(np, val)register Number *np; register Ulong val;
#endif
{
	/*
	* NUMBITS is at least 4 and is a multiple of 4.
	* To cover 32 bits of a Ulong, we need the following.
	*
	*   If NUMBITS is:    Then these members need to be set:
	*	32		pkt[0]
	*	28,24,20,16	pkt[0..1]
	*	12		pkt[0..2]
	*	8		pkt[0..3]
	*	4		pkt[0..7]
	*
	* Reasonable compilation systems will simply eliminate
	* the following "if"s that cannot be reached because
	* NUMBITS is a compile-time constant.
	*/
	*np = prenums[0];
	np->pkt[0] = val & NBMASK;
	/*CONSTANTCONDITION*/
	if (NUMBITS < 32)
	{
		if ((val >>= NUMBITS) != 0)
			np->pkt[1] = val & NBMASK;
		/*CONSTANTCONDITION*/
		if (NUMBITS < 16)
		{
			if ((val >>= NUMBITS) != 0)
				np->pkt[2] = val & NBMASK;
			/*CONSTANTCONDITION*/
			if (NUMBITS < 12)
			{
				if ((val >>= NUMBITS) != 0)
					np->pkt[3] = val & NBMASK;
				/*CONSTANTCONDITION*/
				if (NUMBITS < 8)
				{
					if ((val >>= NUMBITS) != 0)
						np->pkt[4] = val & NBMASK;
					if ((val >>= NUMBITS) != 0)
						np->pkt[5] = val & NBMASK;
					if ((val >>= NUMBITS) != 0)
						np->pkt[6] = val & NBMASK;
					if ((val >>= NUMBITS) != 0)
						np->pkt[7] = val & NBMASK;
				}
			}
		}
	}
}

void
#ifdef __STDC__
initnums(void)	/* generate pre-built numbers */
#else
initnums()
#endif
{
	Ulong i;

	/*
	* Verify parameter requirements, including that CHAR_BIT
	* is correct for the host machine.
	*/
#if BYTE_ORDER != BIG_ENDIAN && BYTE_ORDER != LITTLE_ENDIAN
	fatal("initnums():BYTE_ORDER not specified");
#endif
	/*CONSTANTCONDITION*/
	if (NUMBITS < 4 || NUMBITS > sizeof(Ulong) * CHAR_BIT
		|| TOTNUMBITS % NUMBITS != 0
		|| (1 + ((Uchar)~(Uint)0)) != BIT(CHAR_BIT))
	{
		fatal("initnums():bad numeric package parameters");
	}
	/*
	* Fill in rest of mask array, if any.
	*/
	for (i = 32; i < sizeof(mask) / sizeof(mask[0]); i++)
		mask[i] = MASK(i);
	/*
	* Extracting the low NUMBITS and next NUMBITS gives at least
	* 8 bits of value coverage.  Assume that the array has no
	* more than 32 bits worth of length.
	*/
	for (i = sizeof(prenums) / sizeof(prenums[0]) - 1; i != 0; i--)
		num32(&prenums[i], (Ulong)i);
	num32(&low16, (Ulong)0xffff);
	num32(&ten_9, (Ulong)1000000000);
}

	/*
	* The following functions implement fundamental operations on
	* Numbers.  Those that return void cannot overflow.  Otherwise,
	* unless it is a "pure" function [like ucompare()], these
	* functions return an int value that indicates whether an
	* overflow occurred during the operation.
	*/

static void
#ifdef __STDC__
and(Number *dst, const Number *src)	/* dst &= src */
#else
and(dst, src)Number *dst, *src;
#endif
{
	register Packet *d = dst->pkt;
	register const Packet *s = src->pkt;
	register int i;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "and(dst=%s", num2hex(dst));
		(void)fprintf(stderr, ",src=%s)", num2hex(src));
	}
#endif
	for (i = NUMLEN; --i >= 0;)
		*d++ &= *s++;
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "->%s\n", num2hex(dst));
#endif
}

static void
#ifdef __STDC__
or(Number *dst, const Number *src)	/* dst |= src */
#else
or(dst, src)Number *dst, *src;
#endif
{
	register Packet *d = dst->pkt;
	register const Packet *s = src->pkt;
	register int i;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "or(dst=%s", num2hex(dst));
		(void)fprintf(stderr, ",src=%s)", num2hex(src));
	}
#endif
	for (i = NUMLEN; --i >= 0;)
		*d++ |= *s++;
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "->%s\n", num2hex(dst));
#endif
}

static void
#ifdef __STDC__
xor(Number *dst, const Number *src)	/* dst ^= src */
#else
xor(dst, src)Number *dst, *src;
#endif
{
	register Packet *d = dst->pkt;
	register const Packet *s = src->pkt;
	register int i;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "xor(dst=%s", num2hex(dst));
		(void)fprintf(stderr, ",src=%s)", num2hex(src));
	}
#endif
	for (i = NUMLEN; --i >= 0;)
		*d++ ^= *s++;
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "->%s\n", num2hex(dst));
#endif
}

static void
#ifdef __STDC__
complement(Number *dst)	/* dst = ~dst */
#else
complement(dst)Number *dst;
#endif
{
	register Packet *p = dst->pkt;
	register int i;

#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "complement(dst=%s)", num2hex(dst));
#endif
	for (i = NUMLEN; --i >= 0;)
		*p++ ^= NBMASK;
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "->%s\n", num2hex(dst));
#endif
}

#ifdef __STDC__
static int leftshift(Number *, long);
#else
static int leftshift();
#endif

static int
#ifdef __STDC__
rightshift(Number *dst, long n)	/* dst >>= n (zero fill) */
#else
rightshift(dst, n)Number *dst; long n;
#endif
{
	register Packet *p;
	register int i, k, oflow = 0;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "rightshift(dst=%s,n=%ld)",
			num2hex(dst), n);
	}
#endif
	/*
	* Shifts are done in at most two stages.
	* First, the appropriate number of NUMBITS packets are shifted.
	* Second, the remaining bits (less than NUMBITS) are shifted.
	*/
	if (n >= TOTNUMBITS)
	{
		n = TOTNUMBITS;
		oflow = 1;
	}
	else if (n < 0)	/* negative right shift is left w/overflow */
	{
		(void)leftshift(dst, -n);
		oflow = 1;
		goto ret;
	}
	if ((i = n / NUMBITS) != 0)	/* handle packets of NUMBITS */
	{
		p = dst->pkt;
		for (k = NUMLEN - i; --k >= 0; p++)
			p[0] = p[i];
		while (--i >= 0)
			*p++ = 0;
	}
	if ((i = n % NUMBITS) != 0)	/* handle remaining */
	{
		p = dst->pkt;
		p[0] >>= i;
		for (k = NUMLEN - 1; --k >= 0; p++)
		{
			p[0] |= (p[1] & mask[i]) << (NUMBITS - i);
			p[1] >>= i;
		}
	}
ret:;
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "->%s [%d]\n", num2hex(dst), oflow);
#endif
	return oflow;
}

static int
#ifdef __STDC__
leftshift(Number *dst, long n)	/* dst <<= n */
#else
leftshift(dst, n)Number *dst; long n;
#endif
{
	register Packet *p;
	register int i, k, oflow = 0;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "leftshift(dst=%s,n=%ld)",
			num2hex(dst), n);
	}
#endif
	/*
	* Shifts are done in at most two stages.
	* First, the appropriate number of NUMBITS packets are shifted.
	* Second, the remaining (less than NUMBITS) are shifted.
	*/
	if (n >= TOTNUMBITS)
	{
		n = TOTNUMBITS;
		oflow = 1;
	}
	else if (n < 0)	/* negative left shift is right w/overflow */
	{
		(void)rightshift(dst, -n);
		oflow = 1;
		goto ret;
	}
	if ((i = n / NUMBITS) != 0)	/* handle packets of NUMBITS */
	{
		if (oflow == 0)	/* check for overflow */
		{
			k = NUMLEN - i;
			for (p = dst->pkt + k; k < NUMLEN; k++)
			{
				if (*p++ != 0)
				{
					oflow = 1;
					break;
				}
			}
		}
		/*
		* Shift the bits.
		*/
		k = NUMLEN - i;
		for (p = dst->pkt + k; --k >= 0;)
		{
			--p;
			p[i] = p[0];
		}
		p = dst->pkt;
		while (--i >= 0)
			*p++ = 0;
	}
	if ((i = n % NUMBITS) != 0)	/* handle remaining */
	{
		k = NUMLEN - 1;
		p = dst->pkt + NUMLEN - 1;
		if ((p[0] <<= i) & ~NBMASK)
		{
			p[0] &= NBMASK;
			oflow = 1;
		}
		while (--k >= 0)
		{
			p--;
			p[1] |= p[0] >> (NUMBITS - i);
			p[0] = (p[0] << i) & NBMASK;
		}
	}
ret:;
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "->%s [%d]\n", num2hex(dst), oflow);
#endif
	return oflow;
}

static int
#ifdef __STDC__
ucompare(const Number *n1, const Number *n2)	/* return unsigned <0,0,>0 */
#else
ucompare(n1, n2)Number *n1, *n2;
#endif
{
	register const Packet *p1, *p2;
	register int i;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "ucompare(n1=%s", num2hex(n1));
		(void)fprintf(stderr, ",n2=%s)", num2hex(n2));
	}
#endif
	p1 = n1->pkt + NUMLEN;
	p2 = n2->pkt + NUMLEN;
	for (i = NUMLEN; --i >= 0;)
	{
		if (*--p1 != *--p2)
		{
			if (*p1 < *p2)
			{
				i = -1;
				goto ret;
			}
			i = 1;
			goto ret;
		}
	}
	i = 0;
ret:;
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "->%d\n", i);
#endif
	return i;
}

static int
#ifdef __STDC__
scompare(const Number *n1, const Number *n2)	/* return signed <0,0,>0 */
#else
scompare(n1, n2)Number *n1, *n2;
#endif
{
	register const Packet *p1, *p2;
	register int i;
	register Ulong high;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "scompare(n1=%s", num2hex(n1));
		(void)fprintf(stderr, ",n2=%s)", num2hex(n2));
	}
#endif
	high = NUMSIGN(n1->pkt);
	p1 = n1->pkt + NUMLEN;
	p2 = n2->pkt + NUMLEN;
	for (i = NUMLEN; --i >= 0;)
	{
		if (*--p1 != *--p2)
		{
			if (*p1 < *p2)
			{
				if (high != 0)
				{
					i = 1;
					goto ret;
				}
				i = -1;
				goto ret;
			}
			if (high != 0)
			{
				i = -1;
				goto ret;
			}
			i = 1;
			goto ret;
		}
	}
	i = 0;
ret:;
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "->%d\n", i);
#endif
	return i;
}

static int
#ifdef __STDC__
uadd(Number *dst, const Number *src)	/* dst += src (unsigned) */
#else
uadd(dst, src)Number *dst, *src;
#endif
{
	register Packet *d = dst->pkt;
	register const Packet *s = src->pkt;
	register Ulong carry = 0;
	register int i;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "uadd(dst=%s", num2hex(dst));
		(void)fprintf(stderr, ",src=%s)", num2hex(src));
	}
#endif
	/*
	* Overflow occurs if carry is nonzero at the end of the loop.
	*/
	for (i = NUMLEN; --i >= 0; d++)
	{
		*d += *s++ + carry;
		if ((carry = *d & ~NBMASK) != 0)
		{
			*d &= NBMASK;
			carry = 1;
		}
	}
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "->%s [%d]\n", num2hex(dst), (int)carry);
#endif
	
	return carry;
}

static int
#ifdef __STDC__
sadd(Number *dst, const Number *src)	/* dst += src (signed) */
#else
sadd(dst, src)Number *dst, *src;
#endif
{
	register Packet *d = dst->pkt;
	register const Packet *s = src->pkt;
	register Ulong carry = 0;
	register int i;
	register Ulong high;	/* only for overflow check */

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "sadd(dst=%s", num2hex(dst));
		(void)fprintf(stderr, ",src=%s)", num2hex(src));
	}
#endif
	/*
	* Overflow occurs when the two sources have the same
	* high order bit value and the result is different.
	*/
	high = NUMSIGN(dst->pkt);
	for (i = NUMLEN; --i >= 0; d++)
	{
		*d += *s++ + carry;
		if ((carry = *d & ~NBMASK) != 0)
		{
			*d &= NBMASK;
			carry = 1;
		}
	}
	/*
	* Set carry depending on overflow.
	*/
	if (high != NUMSIGN(src->pkt))
		carry = 0;
	else
		carry = (high != NUMSIGN(dst->pkt));
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "->%s [%d]\n", num2hex(dst), (int)carry);
#endif
	return carry;
}

static int
#ifdef __STDC__
negate(Number *dst)	/* dst = -dst */
#else
negate(dst)Number *dst;
#endif
{
	int oflow;

#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "negate(dst=%s)...\n", num2hex(dst));
#endif
	complement(dst);
	oflow = sadd(dst, &prenums[1]);
#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "...negate()->%s [%d]\n",
			num2hex(dst), oflow);
	}
#endif
	return oflow;
}

static void		/* i860-only */
#ifdef __STDC__
low(Number *dst)	/* dst &= 0xffff */
#else
low(dst)Number *dst;
#endif
{
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "low(dst=%s)...\n", num2hex(dst));
#endif
	and(dst, &low16);
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "...low()->%s\n", num2hex(dst));
#endif
}

static void		/* i860-only */
#ifdef __STDC__
highadjust(Number *dst)	/* dst >>= 16; [dst += 1;] dst &= 0xffff */
#else
highadjust(dst)Number *dst;
#endif
{
	Ulong bit15;

#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "highadjust(dst=%s)...\n", num2hex(dst));
#endif
	bit15 = ISSET(dst->pkt, 15);
	(void)rightshift(dst, 16L);
	if (bit15 != 0)
		(void)uadd(dst, &prenums[1]);
	and(dst, &low16);
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "...highadjust()->%s\n", num2hex(dst));
#endif
}

static int
#ifdef __STDC__
umultiply(Number *dst, const Number *src)	/* dst *= src (unsigned) */
#else
umultiply(dst, src)Number *dst, *src;
#endif
{
	Packet res[2 * NUMLEN];	/* result */
	register Packet *rp, *dp;
	register const Packet *sp;
	register Ulong carry;
	register int i, j;
	int oflow = 0;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "umultiply(dst=%s", num2hex(dst));
		(void)fprintf(stderr, ",src=%s)", num2hex(src));
	}
#endif
	/*
	* Algorithm used is "Algorithm M" from Knuth Vol. 2, 4.3.1.
	*/
	(void)memset((void *)res, 0, sizeof(Packet) * NUMLEN);
	sp = src->pkt;
	for (j = 0; j < NUMLEN; j++, sp++)
	{
		if (*sp == 0)	/* don't bother with following loop */
		{
			res[j + NUMLEN] = 0;
			continue;
		}
		carry = 0;
		dp = dst->pkt;
		rp = res + j;
		for (i = NUMLEN; --i >= 0; dp++)
		{
			*rp += *sp * *dp + carry;
			carry = *rp >> NUMBITS;
			*rp++ &= NBMASK;
		}
		*rp = carry;
	}
	/*
	* Copy result into dst.  Check for result too big.
	*/
	(void)memcpy((void *)dst->pkt, (void *)res, sizeof(dst->pkt));
	rp = res + NUMLEN;
	for (i = NUMLEN; --i >= 0;)
	{
		if (*rp++ != 0)
		{
			oflow = 1;
			break;
		}
	}
#ifdef DEBUG
	if (DEBUG('n') > 1)
		(void)fprintf(stderr, "->%s [%d]\n", num2hex(dst), oflow);
#endif
	return oflow;
}

static int
#ifdef __STDC__
smultiply(Number *dst, const Number *src)	/* dst *= src (signed) */
#else
smultiply(dst, src)Number *dst, *src;
#endif
{
	Number tmp;		/* copy of src */
	int sign, oflow = 0;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "smultiply(dst=%s", num2hex(dst));
		(void)fprintf(stderr, ",src=%s)...\n", num2hex(src));
	}
#endif
	sign = 0;
	/*
	* The multiplicands must have nonnegative values.
	* sign != 0 => negate result.
	*/
	if (NUMSIGN(dst->pkt))
	{
		sign = 1;
		if (negate(dst) != 0)
			oflow = 1;
	}
	if (NUMSIGN(src->pkt))
	{
		sign ^= 1;
		tmp = *src;
		if (negate(&tmp) != 0)
			oflow = 1;
		src = &tmp;
	}
	oflow |= umultiply(dst, src);	/* do the multiply */
	/*
	* Check for nonzero sign bit (overflow).
	* Negate result if appropriate.
	*/
	if (oflow == 0 && NUMSIGN(dst->pkt))
		oflow = 1;
	if (sign != 0 && negate(dst) != 0)
		oflow = 1;
#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "...smultiply()->%s [%d]\n",
			num2hex(dst), oflow);
	}
#endif
	return oflow;
}

static int
#ifdef __STDC__
highbitnum(const Number *src)	/* return number of highest order 1 */
#else
highbitnum(src)Number *src;
#endif
{
	register const Packet *p = &src->pkt[NUMLEN];
	register int nb = TOTNUMBITS - 1;

#ifdef DEBUG
	if (DEBUG('n') > 2)
		(void)fprintf(stderr, "highbitnum(src=%s)", num2hex(src));
#endif
	do	/* for each packet */
	{
		if (*--p != 0)
		{
			register Ulong bit = BIT(NUMBITS - 1);

			do	/* for each bit */
			{
				if ((*p & bit) != 0)
					goto ret;
				nb--;
			} while ((bit >>= 1) != 0);
			fatal("highorderbit():nonzero packet is all zeros");
		}
	} while ((nb -= NUMBITS) > 0);
ret:;
#ifdef DEBUG
	if (DEBUG('n') > 2)
		(void)fprintf(stderr, "->%d\n", nb);
#endif
	return nb;
}

static int
#ifdef __STDC__
udivrem(Number *quot, Number *rem, const Number *num, const Number *den)
#else
udivrem(quot, rem, num, den)Number *quot, *rem, *num, *den;
#endif
{
	Number negden;
	int oflow = 0;
	int nb, n;

	/*
	* Algorithm derived from "Algorithm D" from Knuth Vol. 2, 4.3.1.
	*
	* Two special cases:
	*	denom == 0	oflow	quot = ~0>>1, rem = num
	*	denom == 1		quot =   num, rem =   0
	*
	* Detect the above two special cases while locating the highest
	* order set bit in denominator.
	*/
#ifdef DEBUG
	if (DEBUG('n') > 2)
	{
		(void)fprintf(stderr, "udivrem(quot,rem,num=%s", num2hex(num));
		(void)fprintf(stderr, ",den=%s)...\n", num2hex(den));
	}
#endif
	*quot = prenums[0];
	*rem = *num;
	if ((nb = highbitnum(den)) < 0)	/* den == 0 */
	{
		oflow = 1;
		complement(quot);
		quot->pkt[NUMLEN - 1] &= ~BIT(NUMBITS - 1); /* "infinity" */
		goto ret;
	}
	if (nb == 0)	/* den == 1 */
	{
		*quot = *num;
		*rem = prenums[0];
		goto ret;
	}
	/*
	* Where the highest order bit is set in the numerator
	* determines the location of the first possibility for
	* a nonzero quotient bit.
	*/
	if ((n = highbitnum(num) + 1 - nb) <= 0)	/* num < den */
		goto ret;
	(void)rightshift(rem, (long)n);
	negden = *den;
	(void)negate(&negden);
	do
	{
		n--;
		(void)leftshift(rem, 1L);
		if (ISSET(num->pkt, n))
			rem->pkt[0] |= 1;
		(void)leftshift(quot, 1L);
		if (ucompare(rem, den) >= 0)
		{
			(void)uadd(rem, &negden);
			quot->pkt[0] |= 1;
		}
	} while (n != 0);
ret:;
#ifdef DEBUG
	if (DEBUG('n') > 2)
	{
		(void)fprintf(stderr, "...udivrem()->quot=%s", num2hex(quot));
		(void)fprintf(stderr, ",rem=%s [%d]\n", num2hex(rem), oflow);
	}
#endif
	return oflow;
}

static int
#ifdef __STDC__
udivide(Number *dst, const Number *src)		/* dst /= src (unsigned) */
#else
udivide(dst, src)Number *dst, *src;
#endif
{
	Number tdst, trem;
	int oflow;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "udivide(dst=%s", num2hex(dst));
		(void)fprintf(stderr, ",src=%s)...\n", num2hex(src));
	}
#endif
	tdst = *dst;
	oflow = udivrem(dst, &trem, &tdst, src);
#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "...udivide()->%s [%d]\n",
			num2hex(dst), oflow);
	}
#endif
	return oflow;
}

static int
#ifdef __STDC__
uremainder(Number *dst, const Number *src)	/* dst %= src (unsigned) */
#else
uremainder(dst, src)Number *dst, *src;
#endif
{
	Number tquot, tdst;
	int oflow;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "uremainder(dst=%s", num2hex(dst));
		(void)fprintf(stderr, ",src=%s)...\n", num2hex(src));
	}
#endif
	tdst = *dst;
	oflow = udivrem(&tquot, dst, &tdst, src);
#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "...uremainder()->%s [%d]\n",
			num2hex(dst), oflow);
	}
#endif
	return oflow;
}

static int
#ifdef __STDC__
sdivrem(Number *quot, Number *rem, const Number *den)	/* signed /% */
#else
sdivrem(quot, rem, den)Number *quot, *rem, *den;
#endif
{
	Number tden, trem;
	int qneg = 0, rneg = 0, oflow = 0;

	/*
	* There are two methods of handling signed sources when the
	* remainder is nonzero.  This algorithm uses the one implemented
	* by the great majority of existing machines: in which quot is
	* the integer of lesser magnitude that is nearest to the actual
	* algebraic quotient.  This is summarized by the following.
	*
	*	 3/2 ==  1.5 == -3/-2
	*	-3/2 == -1.5 ==  3/-2
	*
	*		   |   3  |  -3  |
	*		---+------+------+
	*		 2 | Q  1 | Q -1 |
	*		   | R  1 | R -1 |
	*		---+------+------+
	*		-2 | Q -1 | Q  1 |
	*		   | R  1 | R -1 |
	*		---+------+------+
	*/
#ifdef DEBUG
	if (DEBUG('n') > 2)
	{
		(void)fprintf(stderr, "sdivrem(quot,rem/num=%s", num2hex(rem));
		(void)fprintf(stderr, ",den=%s)...\n", num2hex(den));
	}
#endif
	if (NUMSIGN(rem->pkt))
	{
		rneg = 1;
		qneg = 1;
		if (negate(rem) != 0)
			oflow = 1;
	}
	trem = *rem;
	if (NUMSIGN(den->pkt))
	{
		qneg ^= 1;
		tden = *den;
		if (negate(&tden) != 0)
			oflow = 1;
		den = &tden;
	}
	oflow |= udivrem(quot, rem, &trem, den);
	if (rneg && negate(rem) != 0)
		oflow = 1;
	if (qneg && negate(quot) != 0)
		oflow = 1;
#ifdef DEBUG
	if (DEBUG('n') > 2)
	{
		(void)fprintf(stderr, "...sdivrem()->quot=%s", num2hex(quot));
		(void)fprintf(stderr, ",rem=%s [%d]\n", num2hex(rem), oflow);
	}
#endif
	return oflow;
}

static int
#ifdef __STDC__
sdivide(Number *dst, const Number *src)		/* dst /= src (signed) */
#else
sdivide(dst, src)Number *dst, *src;
#endif
{
	Number tdst;
	int oflow;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "sdivide(dst=%s", num2hex(dst));
		(void)fprintf(stderr, ",src=%s)...\n", num2hex(src));
	}
#endif
	tdst = *dst;
	oflow = sdivrem(dst, &tdst, src);
#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "...sdivide()->%s [%d]\n",
			num2hex(dst), oflow);
	}
#endif
	return oflow;
}

static int
#ifdef __STDC__
sremainder(Number *dst, const Number *src)	/* dst %= src (signed) */
#else
sremainder(dst, src)Number *dst, *src;
#endif
{
	Number tquot;
	int oflow;

#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "sremainder(dst=%s", num2hex(dst));
		(void)fprintf(stderr, ",src=%s)...\n", num2hex(src));
	}
#endif
	oflow = sdivrem(&tquot, dst, src);
#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "...sremainder()->%s [%d]\n",
			num2hex(dst), oflow);
	}
#endif
	return oflow;
}

static int
#ifdef __STDC__
lcm(Number *dst, const Number *src)	/* dst = LCM(dst, src) */
#else
lcm(dst, src)Number *dst, *src;
#endif
{
	Number d, s, t;
	int flag, n, nbits;
	int oflow;

	/*
	* Adapted from "Algorithm B" from Knuth Vol. 2, 4.5.2.,
	* using a flag instead of negative values.
	*/
#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "lcm(dst=%s", num2hex(dst));
		(void)fprintf(stderr, ",src=%s)...\n", num2hex(src));
	}
#endif
	d = *dst;
	s = *src;
	n = 0;
	for (nbits = 0; ((d.pkt[0] | s.pkt[0]) & BIT(n)) == 0; nbits++)
	{
		if (++n == NUMBITS)
		{
			(void)rightshift(&d, (long)NUMBITS);
			(void)rightshift(&s, (long)NUMBITS);
			n = 0;
		}
	}
	if (n > 0)
	{
		(void)rightshift(&d, (long)n);
		(void)rightshift(&s, (long)n);
	}
	if ((flag = d.pkt[0] & 0x1) != 0)
		t = s;
	else
		t = d;
	for (;;)
	{
		n = 0;
		while ((t.pkt[0] & BIT(n)) == 0)
		{
			if (++n == NUMBITS)
			{
				(void)rightshift(&t, (long)NUMBITS);
				n = 0;
			}
		}
		if (n > 0)
			(void)rightshift(&t, (long)n);
		if (flag)
			d = t;
		else
			s = t;
		if ((flag = ucompare(&d, &s)) > 0)
		{
			t = s;
			(void)negate(&t);
			(void)sadd(&t, &d);
			flag = 0;
		}
		else if (flag == 0)
			break;		/* only way out */
		else
		{
			t = d;
			(void)negate(&t);
			(void)sadd(&t, &s);
			flag = 1;
		}
	}
	/*
	* At this point, d << nbits is GCD(dst, src).
	* LCM(dst, src) is (dst * src) / GCD(dst, src).
	*/
	(void)leftshift(&d, (long)nbits);
	(void)udivide(dst, &d);
	oflow = umultiply(dst, src);
#ifdef DEBUG
	if (DEBUG('n') > 1)
	{
		(void)fprintf(stderr, "...lcm()->%s [%d]\n",
			num2hex(dst), oflow);
	}
#endif
	return oflow;
}

Number *
#ifdef __STDC__
newnum(const Number *np, Ulong val)	/* allocate copy of number */
#else
newnum(np, val)Number *np; Ulong val;
#endif
{
	static Number *avail, *endavail;
	register Number *new;

#ifdef DEBUG
	if (DEBUG('n') > 0)
		(void)fprintf(stderr, "new number: %s\n", num2hex(np));
#endif
	/*
	* First, check the precalculated range.
	*/
	if (val < sizeof(prenums) / sizeof(prenums[0]))
		return &prenums[val];
	/*
	* Otherwise, simply make a new copy.  Studies indicate that
	* the extra space necessary to manage a reuse scheme averages
	* slightly better than this approach, but this is smaller,
	* easier to manage, and is faster.
	*/
#ifndef ALLOC_NUMB_CHUNK
#define ALLOC_NUMB_CHUNK 300
#endif
	if ((new = avail) == endavail)
	{
#ifdef DEBUG
		if (DEBUG('a') > 0)
		{
			static Ulong total;

			(void)fprintf(stderr, "Total Numbers=%lu @%lu ea.\n",
				total += ALLOC_NUMB_CHUNK,
				(Ulong)sizeof(Number));
		}
#endif
		avail = new = (Number *)alloc((void *)0,
			ALLOC_NUMB_CHUNK * sizeof(Number));
		endavail = new + ALLOC_NUMB_CHUNK;
	}
	avail = new + 1;
	*new = *np;
	return new;
}

int
#ifdef __STDC__
num2ulong(Ulong *dst, register const Number *np) /* Ulong value for number */
#else
num2ulong(dst, np)Ulong *dst; register Number *np;
#endif
{
	register Ulong res;
	register int i, n;

#ifdef DEBUG
	if (DEBUG('n') > 0)
	{
		(void)fprintf(stderr, "num2ulong(dst=%#lx,np=%s)\n",
			(Ulong)dst, num2hex(np));
	}
#endif
	/*
	* Quick check for precomputed number.
	*/
	if (np >= &prenums[0]
		&& np < &prenums[sizeof(prenums) / sizeof(prenums[0])])
	{
		*dst = np - prenums;
		return 0;
	}
	res = 0;
	n = 0;
	i = 0;
	do	/* Ulong has at least NUMBITS */
	{
		res |= np->pkt[i] << n;
		n += NUMBITS;
	} while (++i < sizeof(Ulong) * CHAR_BIT / NUMBITS);
	/*
	* Handle extra bits--only if the number of bits in a
	* Ulong is not a multiple of NUMBITS.  Hopefully in
	* the usual case (that the number of bits in a Ulong
	* is a multiple of NUMBITS), the unreachable code
	* will be eliminated.
	*/
	/*CONSTANTCONDITION*/
	if (sizeof(Ulong) * CHAR_BIT % NUMBITS == 0)
		*dst = res;
	else
	{
		register int k;

		if ((k = sizeof(Ulong) * CHAR_BIT - n) == 0)
			*dst = res;
		else
		{
			res |= (mask[k] & np->pkt[i]) << n;
			*dst = res;
			if (np->pkt[i++] & ~mask[k])	/* overflow check */
				return 1;
		}
	}
	/*
	* Check for overflow on remaining packets.
	*/
	while (i < NUMLEN)
	{
		if (np->pkt[i++] != 0)
			return 1;
	}
	return 0;
}

Number *
#ifdef __STDC__
ulong2num(Ulong val)	/* make internal from Ulong; uses static data */
#else
ulong2num(val)Ulong val;
#endif
{
	static Number res;

	if (val < sizeof(prenums) / sizeof(prenums[0]))	/* easy */
		return &prenums[val];
	/*
	* Convert 32 bits at each shot.  Bypass the whole loop
	* if a Ulong is not larger than 32 bits (and thus never
	* reaching the unspecified result of shifting a 32 bit
	* quantity by 32 bits).  Because the "protect the user"
	* compiler refuses to believe that this code is correct,
	* the macro THIRTY_TWO causes a shift by 32 NOT to occur
	* if the code cannot be reached!!
	*/
	num32(&res, val);
	/*CONSTANTCONDITION*/
	if (sizeof(Ulong) * CHAR_BIT > 32)
	{
		Number tmp;
		register long cnt = 0;

#define THIRTY_TWO ((sizeof(Ulong) * CHAR_BIT > 32) ? 32 : 1) /*ugh*/
		while ((val >>= THIRTY_TWO) != 0)
		{
			num32(&tmp, val);
			(void)rightshift(&tmp, cnt += 32);
			or(&res, &tmp);
		}
#undef THIRTY_TWO
	}
	return &res;
}

static int
#ifdef __STDC__
collect(register Number *p, int base, int dig) /* p = p * base + dig */
#else
collect(p, base, dig)register Number *p; int base, dig;
#endif
{
	register int oflow;
	Number num;

	switch (base)	/* handle bases specially */
	{
	default:
		fatal("collect():inappropriate base: %d", base);
		/*NOTREACHED*/
	case 16:
		oflow = leftshift(p, 4L);
		break;
	case 10:
		/*
		* p * 10 == p << 3 + p << 1
		*	 == p <<= 1; num = p; p <<= 2; p += num
		*/
		oflow = leftshift(p, 1L);
		num = *p;
		oflow |= leftshift(p, 2L);
		oflow |= uadd(p, &num);
		break;
	case 8:
		oflow = leftshift(p, 3L);
		break;
	case 2:
		oflow = leftshift(p, 1L);
		break;
	}
	/*
	* It is assumed that 0 <= dig <= 15, and that
	* prenums[] has at least 16 elements.
	*/
	oflow |= uadd(p, &prenums[dig]);
	return oflow;
}

Number *
#ifdef __STDC__
mknum(const Uchar *str, size_t len)	/* convert to internal form */
#else
mknum(str, len)Uchar *str; size_t len;
#endif
{
	static const char MSGhex[] = "hexadecimal";
	static const char MSGdec[] = "decimal";
	static const char MSGoct[] = "octal";
	static const char MSGbin[] = "binary";
	static const char MSGdig[] = "invalid digit for %s number: %s";
	static const char MSGpre[] = "invalid %s number: 0%c";
	static const char MSG8o9[] = "decimal number cannot start with 0: %s";
	static const char MSGunk[] = "mknum():unknown number char: %s";
	Number tmp;
	const char *basestr;
	register const Uchar *s = str;
	register Ulong val;
	register size_t n = len;
	register int i, base;

#ifdef DEBUG
	if (DEBUG('n') > 0)
		(void)fprintf(stderr, "mknum(%s)\n", prtstr(str, len));
#endif
	if (n == 0)	/* remove this test if too expensive */
		fatal("mknum():zero-length number");
	/*
	* Handle numbers in three stages:
	* 1. Start out by determining base and handling digits in
	*    that base (specific code for each) until either there
	*    are no more digits, or the next digit could compute a
	*    value that does not fit in 32 bits (the minimum number
	*    of bits in a Ulong).
	* 2. If no more digits remain, either return one of the
	*    prebuilt values or a (quickly) built value.
	* 3. Finally, if there are still more digits, convert the
	*    rest in a general manner using the internal form.
	*/
	switch (*s)
	{
	default:
		fatal(MSGunk, prtstr(s, (size_t)1));
		/*NOTREACHED*/
	case '0':
		/*
		* Hex, Octal, or Binary--the next character
		* determines the base.  If the number is only
		* a single 0, return that number.
		*/
		if (--n == 0)
			return &prenums[0];
		switch (*++s)
		{
		default:
			fatal(MSGunk, prtstr(s, (size_t)1));
			/*NOTREACHED*/
		case '0':
			/*
			* Keep looking for an appropriate digit.
			* Ignore leading zeroes.  If the first
			* non-zero is 8 or 9, jump to decimal loop.
			*/
			for (;;)
			{
				if (--n == 0)
					return &prenums[0];
				if (!isdigit(*++s))
				{
					error(MSGdig, MSGoct,
						prtstr(s, (size_t)1));
				}
				else if (*s > '7')
				{
					error(MSG8o9, prtstr(str, len));
					goto decimal;
				}
				else if (*s != '0')
					break;
			}
			/*FALLTHROUGH*/
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			val = *s - '0';		/* first nonzero digit */
			if ((i = 10) > n)	/* room for 10 digits */
				i = n;
			n -= i;
			while (--i > 0)
			{
				val <<= 3;
				if (isdigit(*++s) && *s <= '7')
					val += *s - '0';
				else
				{
					error(MSGdig, MSGoct,
						prtstr(s, (size_t)1));
				}
			}
			basestr = MSGoct;
			base = 8;
			break;
		case '8':
		case '9':
			error(MSG8o9, prtstr(str, len));
			goto decimal;
		case 'b':
		case 'B':
			if (--n == 0)	/* only 0[bB] */
			{
				error(MSGpre, MSGbin, *s);
				return &prenums[0];
			}
			while (*++s == '0') /* ignore leading zeroes */
			{
				if (--n == 0)
					return &prenums[0];
			}
			if (*s == '1')	/* first nonzero digit */
				val = 1;
			else
			{
				error(MSGdig, MSGbin, *s);
				val = 0;
			}
			if ((i = 32) > n)	/* room for 32 digits */
				i = n;
			n -= i;
			while (--i > 0)	/* guaranteed to be [0-9a-fA-F] */
			{
				val <<= 1;
				if (*++s == '0' || *s == '1')
					val += *s - '0';
				else
				{
					error(MSGdig, MSGbin,
						prtstr(s, (size_t)1));
				}
			}
			basestr = MSGbin;
			base = 2;
			break;
		case 'x':
		case 'X':
			if (--n == 0)	/* only 0[xX] */
			{
				error(MSGpre, MSGhex, *s);
				return &prenums[0];
			}
			while (*++s == '0') /* ignore leading zeroes */
			{
				if (--n == 0)
					return &prenums[0];
			}
			/*
			* First nonzero digit.
			* Guaranteed to be [0-9a-fA-F].
			*/
			if (isdigit(*s))
				val = *s - '0';
			else if (islower(*s))
				val = *s - 'a' + 10;
			else
				val = *s - 'A' + 10;
			if ((i = 8) > n)	/* room for 8 digits */
				i = n;
			n -= i;
			while (--i > 0)	/* guaranteed to be [0-9a-fA-F] */
			{
				val <<= 4;
				if (isdigit(*++s))
					val += *s - '0';
				else if (islower(*s))
					val += *s - 'a' + 10;
				else
					val += *s - 'A' + 10;
			}
			basestr = MSGhex;
			base = 16;
			break;
		}
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	decimal:;
		val = *s - '0';		/* first nonzero digit */
		if ((i = 9) > n)	/* room for 9 digits */
			i = n;
		n -= i;
		while (--i > 0)
		{
			val *= 10;
			if (isdigit(*++s))
				val += *s - '0';
			else
				error(MSGdig, MSGdec, prtstr(s, (size_t)1));
		}
		basestr = MSGdec;
		base = 10;
		break;
	}
	/*
	* Have "close to" 32 bits of value.  If no more digits left
	* to convert, return the value.
	*/
	if (val < sizeof(prenums) / sizeof(prenums[0]))
	{
		if (n == 0)
			return &prenums[val];
		tmp = prenums[val];	/* generally should not happen */
	}
	else
	{
		num32(&tmp, val);
		if (n == 0)
			return newnum(&tmp, val);
	}
	/*
	* Collect the rest of the digits into tmp.
	* Digits guaranteed to be [0-9a-fA-FxX].
	*/
	for (;;)
	{
		if (!isxdigit(*++s))
			error(MSGdig, basestr, prtstr(s, (size_t)1));
		else
		{
			if (isdigit(*s))
				i = *s - '0';
			else if (islower(*s))
				i = *s - 'a' + 10;
			else
				i = *s - 'A' + 10;
			if (i >= base)
				error(MSGdig, basestr, prtstr(s, (size_t)1));
			else if (collect(&tmp, base, i) != 0)
				break;	/* overflow */
		}
		if (--n == 0)
			return newnum(&tmp, (Ulong)~0);
	}
	/*
	* Only here if overflow occurred.
	*/
	error("numeric constant out of range: %s", prtstr(str, len));
	return newnum(&tmp, (Ulong)~0);
}

char *
#ifdef __STDC__
num2hex(const Number *np) /* return hex form for number; uses static data */
#else
num2hex(np)Number *np;
#endif
{
	static const char digs[] = "0123456789abcdef";
	static char res[2 + TOTNUMBITS / 4 + 1] = "0x";	/* 0x+digits+\0 */
	register Packet pkt;
	register int d;
	register char *s = res + 1;
	register const Packet *p;
	register int i, n;

	i = NUMLEN;
	for (p = np->pkt + NUMLEN; --i >= 0;)
	{
		pkt = *--p;
		n = NUMBITS - 4;
		do
		{
			if ((d = (pkt >> n) & MASK(4)) != 0 || s > res + 1)
				*++s = digs[d];
		} while ((n -= 4) >= 0);
	}
	if (s <= res + 1)
		*++s = '0';
	*++s = '\0';
	return res;
}

char *
#ifdef __STDC__
num2dec(const Number *np) /* return decimal form for number; uses static data */
#else
num2dec(np)Number *np;
#endif
{
	static const char digs[] = "0123456789";
	static char res[1 + ((TOTNUMBITS + 28) / 29) * 9 + 1];
	Number cur, quot, rem;
	int i, n;
	Ulong val;
	char *s;

	/*
	* Compute the value in 9 decimal digit bunches.  Each udivrem()
	* call reduces the current value by at least 29 bits.  Each
	* remainder is guaranteed to generate 9 decimal digits.  After
	* all digits are filled in, eliminate the leading zeroes and
	* add a minus sign, if appropriate.
	*/
	s = &res[sizeof(res) / sizeof(res[0]) - 1];
	cur = *np;
	if (NUMSIGN(np->pkt))
		(void)negate(&cur);
	for (n = TOTNUMBITS; n >= 0; n -= 29)
	{
		(void)udivrem(&quot, &rem, &cur, &ten_9);
		(void)num2ulong(&val, &rem);
		for (i = 0; i < 9; i++)
		{
			*--s = digs[val % 10];
			val /= 10;
		}
		cur = quot;
	}
	while (*s == '0')
		s++;
	if (*s == '\0')
		s--;
	if (NUMSIGN(np->pkt))
		*--s = '-';
	return s;
}

static void
#ifdef __STDC__
valsize(Value *vp)	/* fill in val_*bits members */
#else
valsize(vp)Value *vp;
#endif
{
	static const Uchar posbit[16] = "\1\2\3\3\4\4\4\4\5\5\5\5\5\5\5\5";
	static const Uchar negbit[16] = "\5\5\5\5\5\5\5\5\4\4\4\4\3\3\2\1";
	register const Packet *p = vp->val_num->pkt;
	register int nb = TOTNUMBITS - NUMBITS;
	register int n = NUMBITS - 4;
	register int val;

	/*
	* The highest-order nonzero bit determines the number of
	* bits needed for an unsigned interpretation.
	* The highest-order bit that differs from the next bit
	* determines the number for signed.
	*/
	if (NUMSIGN(p))		/* "sign bit" set */
	{
		vp->val_unbits = TOTNUMBITS;	/* unsigned easy here */
		p += NUMLEN;
		do
		{
			if (*--p != NBMASK)
				break;
		} while ((nb -= NUMBITS) >= 0);
		if (nb < 0)
		{
			vp->val_snbits = 1;	/* -1 needs one bit */
			vp->val_minbits = 1;
			return;
		}
		do	/* check 4 bits at a time */
		{
			if ((val = (*p >> n) & MASK(4)) != MASK(4))
			{
				nb += n + negbit[val];
				vp->val_snbits = nb;
				vp->val_minbits = nb;
				return;
			}
		} while ((n -= 4) >= 0);
	}
	else
	{
		p += NUMLEN;
		do
		{
			if (*--p != 0)
				break;
		} while ((nb -= NUMBITS) >= 0);
		if (nb < 0)
		{
			vp->val_snbits = 1;	/* 0 needs one bit */
			vp->val_unbits = 1;
			vp->val_minbits = 1;
			return;
		}
		do	/* check 4 bits at a time */
		{
			if ((val = (*p >> n) & MASK(4)) != 0)
			{
				nb += n + posbit[val];
				vp->val_snbits = nb;
				vp->val_unbits = --nb;
				vp->val_minbits = nb;
				return;
			}
		} while ((n -= 4) >= 0);
	}
	fatal("valsize():unfilled/nonzero packet has no zero/one bits");
}

static void
#ifdef __STDC__
flt2data(Uchar *bp, Uint n, Value *vp, Uint form) /* put floating into data */
#else
flt2data(bp, n, vp, form)Uchar *bp; Uint n, form; Value *vp;
#endif
{
#ifdef FLOATING_EXPRS
	static const char MSGrng[] =
		"number out of range for floating conversion: %s";
	struct _fp_x_t ext_prec;
	struct _fp_d_t double_prec;
	struct _fp_f_t float_prec;
	Uchar *src;
	size_t sz;

	if (!(vp->val_flags & VAL_FLOAT)) /* convert integral to floating */
	{
		if (vp->val_minbits > sizeof(Ulong) * CHAR_BIT
			|| vp->val_flags & VAL_OFLOW)
		{
			exprerror(vp->val_expr, MSGrng, num2hex(vp->val_num));
		}
		if (NUMSIGN(vp->val_num->pkt))
			ext_prec = fp_neg(fp_ultox(1 + ~vp->val_ulong));
		else
			ext_prec = fp_ultox(vp->val_ulong);
		vp->val_flt = &ext_prec;
	}
	errno = 0;
	switch (form)
	{
	case CodeForm_Float:
		float_prec = fp_xtof(*vp->val_flt);
		sz = sizeof(float_prec);
		src = (Uchar *)&float_prec;
		break;
	case CodeForm_Double:
		double_prec = fp_xtod(*vp->val_flt);
		sz = sizeof(double_prec);
		src = (Uchar *)&double_prec;
		break;
	case CodeForm_Extended:
		sz = sizeof(ext_prec);
		src = (Uchar *)vp->val_flt;
		break;
	}
	if (errno != 0)
	{
		errno = 0;	/* neither EDOM nor ERANGE is appropriate */
		exprerror(vp->val_expr, MSGrng, fp_xtoa(*vp->val_flt));
	}
	if (n > sz)
	{
		fatal("flt2data():data size (%u) > object size (%lu)",
			n, (Ulong)sz);
	}
	(void)memcpy((void *)bp, (void *)src, (size_t)n);
#else
	fatal("flt2data():setting of floating data value attempted");
#endif /*FLOATING_EXPRS*/
}

void
#ifdef __STDC__
val2data(Value *vp, const Code *cp, Section *secp) /* put value into data */
#else
val2data(vp, cp, secp)Value *vp; Code *cp; Section *secp;
#endif
{
	register Uchar *bp = secp->sec_data + cp->code_addr;
	register Ulong val;
	register Uint i, n = cp->code_size;
	Number num;
	Ulong tmp;

	/*
	* Set data bytes according to the target form.
	* Handle the most common case below.
	*/
	switch (cp->info.code_form)
	{
	default:
		fatal("val2data():unknown code form: %u",
			(Uint)cp->info.code_form);
		/*NOTREACHED*/
	case CodeForm_Integral:
		break;
	case CodeForm_Float:
	case CodeForm_Double:
	case CodeForm_Extended:
		flt2data(bp, n, vp, (Uint)cp->info.code_form);
		return;
	case CodeForm_BCD:
		bcd2data(bp, n, vp);
		return;
	}
	/*
	* Only here for CodeForm_Integral.  This code assumes that the
	* host and target byte sizes (CHAR_BIT) are the same.
	*/
	if (vp->val_minbits > n * CHAR_BIT || vp->val_flags & VAL_OFLOW)
	{
		exprwarn(vp->val_expr, "value does not fit in %u bytes: %s",
			n, num2hex(vp->val_num));
	}
	if (n > sizeof(Ulong))		/* only copy number if necessary */
		num = *vp->val_num;
#if BYTE_ORDER == BIG_ENDIAN
	bp += n;
#endif
	for (val = vp->val_ulong;; val = tmp)
	{
		if ((i = n) > sizeof(Ulong))
			i = sizeof(Ulong);
		n -= i;
		do
		{
#if BYTE_ORDER == BIG_ENDIAN
			*--bp = val & MASK(CHAR_BIT);
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
			*bp++ = val & MASK(CHAR_BIT);
#endif
			val >>= CHAR_BIT;
		} while (--i > 0);
		if (n == 0)
			break;
		(void)rightshift(&num, (long)(sizeof(Ulong) * CHAR_BIT));
		(void)num2ulong(&tmp, &num);
	}
}

static void
#ifdef __STDC__
evalflt(Value *vp)	/* produce value of floating expr */
#else
evalflt(vp)Value *vp;
#endif
{
#ifdef FLOATING_EXPRS
	static struct _fp_x_t extended;
	register const Expr *ep;
	int sign = 0;

	vp->val_flt = &extended;
	vp->val_flags |= VAL_FLOAT;
	/*
	* Floating expressions can only be [+-]constant.
	* Note number of -'s and negate if necessary.
	*/
	for (ep = vp->val_expr;; ep = ep->left.ex_ptr)
	{
		switch (ep->ex_op)
		{
		default:
			fatal("evalflt():inappropriate operator: %s",
				exopstr(ep->ex_op));
			/*NOTREACHED*/
		case ExpOp_Negate:
			sign ^= 1;
			break;
		case ExpOp_UnaryAdd:
			break;
		case ExpOp_LeafFloat:
			errno = 0;
			extended = fp_atox((const char *)ep->right.ex_flt);
			if (errno != 0)
			{
				errno = 0; /* ERANGE sometime misleading */
				exprerror(vp->val_expr,
					"floating constant out of range: %s",
					(const char *)ep->right.ex_flt);
			}
			if (sign != 0)
				extended = fp_neg(extended);
			return;
		}
	}
#else
	fatal("evalflt():evaluation of floating expression attempted");
#endif /*FLOATING_EXPRS*/
}

	/*
	* evalgen() overrides the "const-ness" of vp->val_num.  This
	* guarantees that the caller of evalgen() [mainly evalgen()]
	* knows where the result is placed.  The intent of the
	* "const" is so that the users of an evaluation will not
	* overwrite the resulting number.  [Especially since that
	* number may be a prenums[] entry.  See evalexpr() below.]
	*/
static void
#ifdef __STDC__
evalgen(const Expr *ep, Value *vp)	/* produce value of general expr */
#else
evalgen(ep, vp)Expr *ep; Value *vp;
#endif
{
	static const char MSGsub[] =
		"defined relocatable values from same section required, op -";
	Symbol *sp;
	Value right;	/* value of right child */
	Number rnum;	/* number pointed to by right.val_num */
	Ulong cnt;

	if (ep == 0)
		fatal("evalgen():null expression");
	right.val_flags = 0;
	if ((int)ep->ex_op >= ExpOp_OPER_BinaryFirst)
	{
		if ((int)ep->ex_op < ExpOp_OPER_BinaryLast)
		{
			right.val_num = &rnum;
			evalgen(ep->right.ex_ptr, &right);
			vp->val_flags |=
				right.val_flags & (VAL_OFLOW | VAL_LDIFF);
		}
		evalgen(ep->left.ex_ptr, vp);
	}
	/*
	* Evaluate this operation.
	* Only those operators that should check for relocatable
	* operands (because they are invalid) should use "break".
	*/
	switch (ep->ex_op)
	{
	default:
		fatal("evalgen():unknown operator: %s", exopstr(ep->ex_op));
		/*NOTREACHED*/
	case ExpOp_LeafRegister:
	case ExpOp_LeafString:
	case ExpOp_LeafFloat:
		fatal("evalgen():invalid leaf operator: %s",
			exopstr(ep->ex_op));
		/*NOTREACHED*/
	case ExpOp_LeafName:
		/*
		* Symbol kind determines how to compute the value.
		*/
		switch ((sp = ep->right.ex_sym)->sym_kind)
		{
		case SymKind_Dot:
			/*
			* Current value of . for the current section.
			*/
			vp->val_sym = 0;
			vp->val_sec = cursect();
			vp->val_dot = vp->val_sec->sec_last;
			*(Number *)vp->val_num
				= *ulong2num(vp->val_sec->sec_size);
			vp->val_flags |= VAL_RELOC;
			break;
		case SymKind_Set:
			/*
			* Value of the .set symbol.
			*/
			evalgen(sp->addr.sym_oper->oper_expr, vp);
			/*
			* Change the val_sym member in a special case.
			* If the parent operator is @GOT or @PLT, then
			* relocation must have a handle on this symbol.
			*/
			if ((vp->val_flags & VAL_RELOC)
				&& ep->ex_cont == Cont_Expr)
			{
				int op = ep->parent.ex_ptr->ex_op;

				if (op == ExpOp_Pic_GOT || op == ExpOp_Pic_PLT)
					vp->val_sym = sp;
			}
			break;
		default:
			/*
			* Regular (or common) symbol value.
			*/
			if ((ep = sp->addr.sym_expr) != 0)
			{
				if (ep->ex_op != ExpOp_LeafCode)
				{
					fatal("evalgen():nonlabel symbol: %s",
						(const char *)sp->sym_name);
				}
				vp->val_sym = sp;
				goto codeval;
			}
			else if (sp->sym_exty != ExpTy_Relocatable)
			{
				fatal("evalgen():null expr symbol: %s",
					(const char *)sp->sym_name);
			}
			else	/* (as yet) undefined symbol referenced */
			{
				vp->val_sec = 0;
				vp->val_dot = 0;
				*(Number *)vp->val_num = prenums[0];
				vp->val_flags |= VAL_RELOC;
				vp->val_sym = sp;
			}
			break;
		}
		return;
	case ExpOp_LeafCode:
		vp->val_sym = 0;
	codeval:;
		vp->val_dot = ep->right.ex_code;
		vp->val_sec = ep->left.ex_sect;
		*(Number *)vp->val_num
			= *ulong2num(ep->right.ex_code->code_addr);
		vp->val_flags |= VAL_RELOC;
		return;
	case ExpOp_LeafNumber:
		*(Number *)vp->val_num = *ep->right.ex_num;
		return;
	case ExpOp_Multiply:	/* use umultiply() instead? */
		if (smultiply((Number *)vp->val_num, &rnum) != 0)
			vp->val_flags |= VAL_OFLOW;
		break;
	case ExpOp_Divide:	/* use udivide() instead? */
		if (sdivide((Number *)vp->val_num, &rnum) != 0)
			vp->val_flags |= VAL_OFLOW;
		break;
	case ExpOp_Remainder:	/* use uremainder() instead? */
		if (sremainder((Number *)vp->val_num, &rnum) != 0)
			vp->val_flags |= VAL_OFLOW;
		break;
	case ExpOp_Subtract:
		if (right.val_flags & VAL_RELOC)
		{
			if (!(vp->val_flags & VAL_RELOC))
			{
			badsub:;
				exprerror(ep, MSGsub);
			}
			else if (vp->val_sec == 0 || right.val_sec == 0)
			{
				if (vp->val_sym != right.val_sym)
					goto badsub;
				else if (vp->val_sym == 0)
					fatal("evalgen():RELOC w/out base");
			}
			else if (vp->val_sec != right.val_sec)
				goto badsub;
			right.val_flags &= ~VAL_RELOC;
			vp->val_flags &= ~VAL_RELOC;
			vp->val_flags |= VAL_LDIFF;
			vp->val_sec = 0;
			vp->val_dot = 0;
			vp->val_sym = 0;
		}
		if (negate(&rnum) != 0)
			vp->val_flags |= VAL_OFLOW;
		/*FALLTHROUGH*/
	case ExpOp_Add:		/* use uadd() instead? */
		if (sadd((Number *)vp->val_num, &rnum) != 0)
			vp->val_flags |= VAL_OFLOW;
		if (right.val_flags & VAL_RELOC)
		{
			vp->val_flags |= VAL_RELOC;
			vp->val_sec = right.val_sec;
			vp->val_dot = right.val_dot;
			vp->val_sym = right.val_sym;
		}
		return;
	case ExpOp_Nand:
		complement(&rnum);
		/*FALLTHROUGH*/
	case ExpOp_And:
		and((Number *)vp->val_num, &rnum);
		break;
	case ExpOp_Or:
		or((Number *)vp->val_num, &rnum);
		break;
	case ExpOp_Xor:
		xor((Number *)vp->val_num, &rnum);
		break;
	case ExpOp_LeftShift:
		if (num2ulong(&cnt, &rnum) != 0
			|| leftshift((Number *)vp->val_num, (long)cnt) != 0)
		{
			vp->val_flags |= VAL_OFLOW;
		}
		break;
	case ExpOp_RightShift:
		if (num2ulong(&cnt, &rnum) != 0
			|| rightshift((Number *)vp->val_num, (long)cnt) != 0)
		{
			vp->val_flags |= VAL_OFLOW;
		}
		break;
	case ExpOp_Maximum:	/* use scompare() instead? */
		if (ucompare(vp->val_num, &rnum) < 0)
			*(Number *)vp->val_num = rnum;
		break;
	case ExpOp_LCM:
		if (lcm((Number *)vp->val_num, &rnum) != 0)
			vp->val_flags |= VAL_OFLOW;
		break;
	case ExpOp_Complement:
		complement((Number *)vp->val_num);
		break;
	case ExpOp_UnaryAdd:
		break;
	case ExpOp_Negate:
		if (negate((Number *)vp->val_num) != 0)
			vp->val_flags |= VAL_OFLOW;
		break;
	case ExpOp_SegmentPart:		/* i386 only */
	case ExpOp_OffsetPart:		/* i386 only */
		fatal("evalgen():what do <s> and <o> mean, anyway?");
		/*NOTREACHED*/
	case ExpOp_Pic_PC:
	case ExpOp_Pic_GOT:
	case ExpOp_Pic_PLT:
	case ExpOp_Pic_GOTOFF:
		/*
		* Don't do any calculation because these only
		* affect relocation operators: the choice of
		* relocation should include checks of val_pic.
		*/
		if (vp->val_flags & VAL_RELOC)
			vp->val_pic = ep->ex_op;
		else
		{
			exprerror(ep, "relocatable value required, op %s",
				exopstr((int)ep->ex_op));
		}
		return;
	/*
	* Only evaluate nonrelocatable expressions--the relocatable
	* uses of the mask operators only affects the choice of
	* relocation operation.  Only set val_mask for relocatables.
	*/
	case ExpOp_LowMask:		/* i860-only */
		if (vp->val_flags & VAL_RELOC)
			vp->val_mask = ExpOp_LowMask;
		else
			low((Number *)vp->val_num);
		return;
	case ExpOp_HighMask:		/* i860-only */
		if (vp->val_flags & VAL_RELOC)
			vp->val_mask = ExpOp_HighMask;
		else
		{
			(void)rightshift((Number *)vp->val_num, 16L);
			low((Number *)vp->val_num);
		}
		return;
	case ExpOp_HighAdjustMask:	/* i860-only */
		if (vp->val_flags & VAL_RELOC)
			vp->val_mask = ExpOp_HighAdjustMask;
		else
			highadjust((Number *)vp->val_num);
		return;
	}
	/*
	* Only here if need to check for relocatable operands.
	*/
	if (right.val_flags & VAL_RELOC || vp->val_flags & VAL_RELOC)
	{
		exprerror(ep, "invalid use of relocatable value, op %s",
			exopstr((int)ep->ex_op));
		vp->val_flags &= ~VAL_RELOC;
	}
}

Value *
#ifdef __STDC__
evalexpr(const Expr *ep)	/* evaluate general or floating expression */
#else
evalexpr(ep)Expr *ep;
#endif
{
	static Number num;
	static const Value empty = {&num};
	static Value val;

#ifdef DEBUG
	if (DEBUG('n') > 0)
	{
		(void)fputs("evalexpr(", stderr);
		printexpr(ep);
		(void)fputs(")\n", stderr);
	}
#endif
	val = empty;
	val.val_expr = ep;
	switch (ep->ex_type)
	{
	default:
		fatal("evalexpr():inappropriate expression type: %u",
			(Uint)ep->ex_type);
		/*NOTREACHED*/
	case ExpTy_Floating:
		evalflt(&val);
		return &val;
	case ExpTy_Integral:
		if (ep->ex_op == ExpOp_LeafNumber)	/* shortcut */
		{
			val.val_num = ep->right.ex_num;
			break;
		}
		/*FALLTHROUGH*/
	case ExpTy_Relocatable:
		evalgen(ep, &val);
		break;
	}
	if (num2ulong(&val.val_ulong, val.val_num) != 0)
		val.val_flags |= VAL_TRUNC;
	valsize(&val);
#ifdef DEBUG
	if (DEBUG('n') > 2)
	{
		(void)fprintf(stderr,
			"evalexpr():%s;ul=%lu;bits:%d,s:%d,u:%d;flags:%x\n",
			num2hex(val.val_num), val.val_ulong,
			val.val_minbits, val.val_snbits, val.val_unbits,
			val.val_flags);
	}
#endif
	return &val;
}

void
#ifdef __STDC__
subvalue(register Value *vp, Ulong sub)	/* vp -= sub */
#else
subvalue(vp, sub)register Value *vp; Ulong sub;
#endif
{
	static Number res;
	Number num;

	vp->val_expr = 0;	/* no longer an expression's evaluation */
	res = *vp->val_num;
	vp->val_num = &res;
	vp->val_flags &= ~VAL_TRUNC;
	num = *ulong2num(sub);
	if (negate(&num) != 0)
		vp->val_flags |= VAL_OFLOW;
	if (sadd(&res, &num) != 0)
		vp->val_flags |= VAL_OFLOW;
	if (num2ulong(&vp->val_ulong, &res) != 0)
		vp->val_flags |= VAL_TRUNC;
	valsize(vp);
}

static struct reeval	/* one "to be reevaluated" expression's result */
{
	Number		num;	/* the calculated result */
	const Expr	*expr;
	Uint		flags;	/* VAL_RELOC kept, VAL_LDIFF added */
	struct reeval	*next;
} *reeval_list;

void
#ifdef __STDC__
delayeval(register Value *vp)	/* put Value on list to be reeval at end */
#else
delayeval(vp)register Value *vp;
#endif
{
	static struct reeval *avail, *endavail;
	register struct reeval *rp;

	if (vp->val_expr == 0)
		fatal("delayeval():Value not derived from expr eval");
	else if (vp->val_flags & VAL_FLOAT)
		fatal("delayeval():floating Value cannot be reevaluated");
#ifndef ALLOC_EVAL_CHUNK
#define ALLOC_EVAL_CHUNK 100
#endif
	if ((rp = avail) == endavail)
	{
#ifdef DEBUG
		if (DEBUG('a') > 0)
		{
			static Ulong total;

			(void)fprintf(stderr,
				"Total struct reevals=%lu @%lu ea.\n",
				total += ALLOC_EVAL_CHUNK,
				(Ulong)sizeof(struct reeval));
		}
#endif
		avail = rp = (struct reeval *)alloc((void *)0,
			ALLOC_EVAL_CHUNK * sizeof(struct reeval));
		endavail = rp + ALLOC_EVAL_CHUNK;
	}
	avail = rp + 1;
	rp->next = reeval_list;
	reeval_list = rp;
	/*
	* Since a Value contains a pointer to a static Number,
	* the reeval object keeps a copy of one of these.  Also,
	* only the VAL_RELOC flag matters in regards the
	* reevaluation.  VAL_RELOC is a special case since the
	* base value for the expression can move, but the offset
	* from the base value shouldn't.  Thus, if val_dot is
	* nonzero (not based on an undefined label), the offset
	* from val_dot is stored instead of val_num, and
	* VAL_LDIFF is placed in flags to note this occurred.
	*/
	rp->num = *vp->val_num;
	if (!(vp->val_flags & VAL_RELOC))
		rp->flags = 0;
	else if (vp->val_dot == 0)
		rp->flags = VAL_RELOC;
	else
	{
		Number num;

		num = *ulong2num(vp->val_dot->code_addr);
		(void)negate(&num);
		(void)sadd(&rp->num, &num);
		rp->flags = VAL_RELOC | VAL_LDIFF;
	}
	rp->expr = vp->val_expr;
}

void
#ifdef __STDC__
reevaluate(void)	/* reevaluate all registered with delayeval() */
#else
reevaluate()
#endif
{
	static const char MSGdiff[] = "expression reevaluation differs";
	register const Packet *old, *new;
	register struct reeval *rp;
	register Value *vp;
	register int len;
	const char *msg;

	for (rp = reeval_list; rp != 0; rp = rp->next)
	{
		vp = evalexpr(rp->expr);
		/*
		* First, check the basic expression type for old and
		* new evaluations, including whether val_dot was
		* subtracted from the old value.
		*/
		if (rp->flags & VAL_RELOC)
		{
			msg = 0;
			if (!(vp->val_flags & VAL_RELOC))
				msg = "%s: used to be relocatable";
			else if (vp->val_dot != 0 && !(rp->flags & VAL_LDIFF))
				msg = "%s: used to be based on undefined name";
			if (msg != 0)	/* emit diagnostic */
			{
				exprerror(rp->expr, msg, MSGdiff);
				continue;
			}
		}
		/*
		* Second, check the old and new numbers.
		*/
		if (rp->flags & VAL_LDIFF)
		{
			if (vp->val_dot == 0)
				fatal("reevaluate():reloc now undef");
			subvalue(vp, vp->val_dot->code_addr);
		}
		len = NUMLEN;
		old = &rp->num.pkt[0];
		new = &vp->val_num->pkt[0];
		/*
		* Compare packet by packet.
		*/
		do
		{
			char was[2 + TOTNUMBITS / 4 + 1]; /* 0x+digits+\0 */

			if (*old++ != *new++)
			{
				/*
				* Need to copy the first result of num2hex
				* as it uses a static buffer for the result.
				*/
				(void)strcpy(was, num2hex(&rp->num));
				exprerror(rp->expr, "%s: was %s, now %s",
					MSGdiff, was, num2hex(vp->val_num));
				break;
			}
		} while (--len > 0);
	}
}
