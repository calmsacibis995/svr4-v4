/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)keyserv:util.c	1.2.2.2"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice
*
* Notice of copyright on this source code product does not indicate
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)util.c 1.1 89/03/10 Copyr 1986 Sun Micro";
#endif
/* from UCB 5.1 4/30/85 */

/* LINTLIBRARY */

#include <stdio.h>
#include "mp.h"

extern char *malloc();

_mp_move(a, b)
	MINT *a, *b;
{
	int i, j;

	xfree(b);
	b->len = a->len;
	if ((i = a->len) < 0) {
		i = -i;
	}
	if (i == 0) {
		return;
	}
	b->val = xalloc(i, "_mp_move");
	for (j = 0; j < i; j++) {
		b->val[j]= a->val[j];
	}
}

/* ARGSUSED */
/* VARARGS */
short *
xalloc(nint, s)
	int nint;
	char *s;
{
	short *i;

	i = (short *) malloc((unsigned)
	    sizeof (short)*((unsigned)nint + 2)); /* ??? 2 ??? */
	if (i == NULL) {
		_mp_fatal("mp: no free space");
	}
#ifdef MP_DEBUG
	(void) fprintf(stderr, "%s: %o\n", s, i);
#endif
	return (i);
}


_mp_fatal(s)
	char *s;
{
	(void) fprintf(stderr, "%s\n", s);
	(void) fflush(stdout);
	sleep(2);
	abort();
}


xfree(c)
	MINT *c;
{
#ifdef DBG
	(void) fprintf(stderr, "xfree ");
#endif
	if (c && c->len != 0) {
		free((char *) c->val);
		c->len = 0;
	}
}


_mp_mcan(a)
	MINT *a;
{
	int i, j;

	if ((i = a->len) == 0) {
		return;
	}
	if (i <0) {
		i = -i;
	}
	for (j = i; j > 0 && a->val[j-1] == 0; j--)
		;
	if (j == i) {
		return;
	}
	if (j == 0) {
		xfree(a);
		return;
	}
	if (a->len > 0) {
		a->len = j;
	} else {
		a->len = -j;
	}
}


MINT *
itom(n)
	int n;
{
	MINT *a;

	a = (MINT *) malloc((unsigned) sizeof (MINT));
	if (a == (MINT *)NULL)
		return ((MINT *)NULL);
	if (n > 0) {
		a->len = 1;
		a->val = xalloc(1, "itom1");
		*a->val = n;
	} else if (n < 0) {
		a->len = -1;
		a->val = xalloc(1, "itom2");
		*a->val = -n;
	} else {
		a->len = 0;
	}
	return (a);
}


mcmp(a, b)
	MINT *a, *b;
{
	MINT c;
	int res;

	_mp_mcan(a);
	_mp_mcan(b);
	if (a->len != b->len) {
		return (a->len - b->len);
	}
	c.len = 0;
	msub(a, b, &c);
	res = c.len;
	xfree(&c);
	return (res);
}

/*
 * Convert hex digit to binary value
 */
static int
xtoi(c)
	char c;
{
	if (c >= '0' && c <= '9') {
		return (c - '0');
	} else if (c >= 'a' && c <= 'f') {
		return (c - 'a' + 10);
	} else if (c >= 'A' && c <= 'F') {
		return (c - 'A' + 10);
	} else {
		return (-1);
	}
}

/*
 * Convert hex key to MINT key
 */
MINT *
xtom(key)
	char *key;
{
	int digit;
	MINT *m = itom(0);
	MINT *d;
	MINT *sixteen;

	sixteen = itom(16);
	for (; *key; key++) {
		digit = xtoi(*key);
		if (digit < 0) {
			return (NULL);
		}
		d = itom(digit);
		mult(m, sixteen, m);
		madd(m, d, m);
		mfree(d);
	}
	mfree(sixteen);
	return (m);
}

static char
itox(d)
	short d;
{
	d &= 15;
	if (d < 10) {
		return ('0' + d);
	} else {
		return ('a' - 10 + d);
	}
}

/*
 * Convert MINT key to hex key
 */
char *
mtox(key)
	MINT *key;
{
	MINT *m = itom(0);
	MINT *zero = itom(0);
	short r;
	char *p;
	char c;
	char *s;
	char *hex;
	int size;

#	define BASEBITS	(8*sizeof (short) - 1)

	if (key->len >= 0) {
		size = key->len;
	} else {
		size = -key->len;
	}
	hex = malloc((unsigned) ((size * BASEBITS + 3)) / 4 + 1);
	if (hex == NULL) {
		return (NULL);
	}
	_mp_move(key, m);
	p = hex;
	do {
		sdiv(m, 16, m, &r);
		*p++ = itox(r);
	} while (mcmp(m, zero) != 0);
	mfree(m);
	mfree(zero);

	*p = 0;
	for (p--, s = hex; s < p; s++, p--) {
		c = *p;
		*p = *s;
		*s = c;
	}
	return (hex);
}

/*
 * Deallocate a multiple precision integer
 */
void
mfree(a)
	MINT *a;
{
	xfree(a);
	free((char *)a);
}
