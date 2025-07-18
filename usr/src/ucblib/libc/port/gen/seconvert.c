/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ucblibc:port/gen/seconvert.c	1.1.3.1"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

#include <fp.h>

char           *
seconvert(arg, ndigits, decpt, sign, buf)
	single         *arg;
	int             ndigits, *decpt, *sign;
	char           *buf;
{
	decimal_mode    dm;
	decimal_record  dr;
	fp_exception_field_type ef;
	int             i;
	static char     nanstring[] = "NaN";
	static char     infstring[] = "Infinity";
	char           *pc;
	int             nc;

	dm.rd = fp_direction;	/* Rounding direction. */
	dm.df = floating_form;	/* E format. */
	dm.ndigits = ndigits;	/* Number of significant digits. */
	single_to_decimal(arg, &dm, &dr, &ef);
	*sign = dr.sign;
	switch (dr.fpclass) {
	case fp_normal:
	case fp_subnormal:
		*decpt = dr.exponent + ndigits;
		for (i = 0; i < ndigits; i++)
			buf[i] = dr.ds[i];
		buf[ndigits] = 0;
		break;
	case fp_zero:
		*decpt = 1;
		for (i = 0; i < ndigits; i++)
			buf[i] = '0';
		buf[ndigits] = 0;
		break;
	case fp_infinity:
		*decpt = 0;
		pc = infstring;
		if (ndigits < 8)
			nc = 3;
		else
			nc = 8;
		goto movestring;
	case fp_quiet:
	case fp_signaling:
		*decpt = 0;
		pc = nanstring;
		nc = 3;
movestring:
		for (i = 0; i < nc; i++)
			buf[i] = pc[i];
		buf[nc] = 0;
		break;
	}
	return buf;		/* For compatibility with ecvt. */
}

char           *
sfconvert(arg, ndigits, decpt, sign, buf)
	single         *arg;
	int             ndigits, *decpt, *sign;
	char           *buf;
{
	decimal_mode    dm;
	decimal_record  dr;
	fp_exception_field_type ef;
	int             i;

	dm.rd = fp_direction;	/* Rounding direction. */
	dm.df = fixed_form;	/* F format. */
	dm.ndigits = ndigits;	/* Number of digits after point. */
	single_to_decimal(arg, &dm, &dr, &ef);
	*sign = dr.sign;
	switch (dr.fpclass) {
	case fp_normal:
	case fp_subnormal:
		if (ndigits >= 0)
			*decpt = dr.ndigits - ndigits;
		else
			*decpt = dr.ndigits;
		for (i = 0; i < dr.ndigits; i++)
			buf[i] = dr.ds[i];
		buf[dr.ndigits] = 0;
		break;
	case fp_zero:
		*decpt = 0;
		for (i = 0; i < ndigits; i++)
			buf[i] = '0';
		buf[ndigits] = 0;
		break;
	case fp_infinity:
		*decpt = 0;
		if (ndigits < 8)
			buf = "Inf";
		else
			buf = "Infinity";
		break;
	case fp_quiet:
	case fp_signaling:
		*decpt = 0;
		buf = "NaN";
		break;
	}
	return buf;		/* For compatibility with fcvt. */
}

extern char    *
_gcvt();

char           *
sgconvert(number, ndigit, trailing, buf)
	single         *number;
	int             ndigit, trailing;
	char           *buf;
{
	decimal_mode    dm;
	decimal_record  dr;
	fp_exception_field_type fef;

	dm.rd = fp_direction;
	dm.df = floating_form;
	dm.ndigits = ndigit;
	single_to_decimal(number, &dm, &dr, &fef);
	(void) _gcvt(ndigit, &dr, trailing, buf);
	return (buf);
}
