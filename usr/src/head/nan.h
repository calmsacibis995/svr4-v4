/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head:nan.h	1.12.4.2"

#ifndef _NAN_H
#define _NAN_H

/* Handling of Not_a_Number's (only in IEEE floating-point standard) */

#ifndef _IEEE
#include <values.h>
#endif

#if _IEEE
#if !i286 && !i386
typedef union 
{
         struct	
	 {
	    unsigned sign     : 1;
	    unsigned exponent :11;
            unsigned bits:20;
	    unsigned fraction_low:32;
         } inf_parts;
	 struct 
	 {
	    unsigned sign     : 1;
            unsigned exponent :11;
	    unsigned qnan_bit : 1;
	    unsigned bits     :19;
	    unsigned fraction_low: 32;
         } nan_parts;
         double d;

} dnan; 
#endif

#if i386
typedef union 
{
	struct {
		unsigned fraction_low:32;
		unsigned bits:20;
		unsigned exponent :11;
		unsigned sign     : 1;
	} inf_parts;
	struct {
		unsigned fraction_low: 32;
		unsigned bits     :19;
		unsigned qnan_bit : 1;
		unsigned exponent :11;
		unsigned sign     : 1;
	} nan_parts;
	double d;
} dnan;

typedef union 
{
	struct {
		unsigned fraction_low:32;
		unsigned bits:32;
		unsigned exponent :15;
		unsigned sign     : 1;
	} inf_parts;
	struct {
		unsigned fraction_low: 32;
		unsigned bits     :31;
		unsigned qnan_bit : 1;
		unsigned exponent :15;
		unsigned sign     : 1;
	} nan_parts;
	long double ld;
} ldnan;
#endif

#if i286
   /*
    * i286 has 16 bit words and cannot address the fields that
    * 32-bit machines can.
    */
typedef union 
{
	struct	
	{
		unsigned long fraction_low;
		unsigned short bits2;
		unsigned bits1	 :4;
		unsigned exponent :11;
		unsigned sign	  : 1;
	} inf_parts;
	struct 
	{
		unsigned long fraction_low;
		unsigned short bits2;
		unsigned bits1	 : 3;
		unsigned qnan_bit : 1;
		unsigned exponent :11;
		unsigned sign	  : 1;
	} nan_parts;
	double d;
} dnan; 
#endif

	/* IsNANorINF checks that exponent of double == 2047 *
	 * i.e. that number is a NaN or an infinity	     */
	
#define IsNANorINF(X)  (((dnan *)&(X))->nan_parts.exponent == 0x7ff)

	/* IsINF must be used after IsNANorINF		*
 	 * has checked the exponent 			*/

#define IsINF(X)  (((dnan *)&(X))->inf_parts.bits == 0 &&  \
                    ((dnan *)&(X))->inf_parts.fraction_low == 0)

	/* IsPosNAN and IsNegNAN can be used 		*
 	 * to check the sign of infinities too		*/

#define IsPosNAN(X)  (((dnan *)&(X))->nan_parts.sign == 0)

#define IsNegNAN(X)  (((dnan *)&(X))->nan_parts.sign == 1)

	/* GETNaNPC gets the leftmost 32 bits 		*	
	 * of the fraction part				*/

#define GETNaNPC(dval)   (((dnan *)&(dval))->inf_parts.bits << 12 | \
			  ((dnan *)&(dval))->nan_parts.fraction_low>> 20) 

#define IsNANorINFLD(X)  (((ldnan *)&(X))->nan_parts.exponent == 0x7fff)

#define IsINFLD(X)  (((ldnan *)&(X))->inf_parts.bits == 0x80000000 &&  \
                    ((ldnan *)&(X))->inf_parts.fraction_low == 0)

#define IsPosNANLD(X)  (((ldnan *)&(X))->nan_parts.sign == 0)

#define IsNegNANLD(X)  (((ldnan *)&(X))->nan_parts.sign == 1)

#define GETNaNPCLD(ldval)   (((ldnan *)&(ldval))->inf_parts.bits )

#if defined(__STDC__)
#define KILLFPE()       (void) _kill(_getpid(), 8)
#else
#define KILLFPE()       (void) kill(getpid(), 8)
#endif
#define NaN(X)  (((dnan *)&(X))->nan_parts.exponent == 0x7ff)
#define KILLNaN(X)      if (NaN(X)) KILLFPE()

#else

typedef double dnan;
#define IsNANorINF(X)	0
#define IsINF(X)   0
#define IsPINF(X)  0
#define IsNegNAN(X)  0
#define IsPosNAN(X)  0
#define IsNAN(X)   0
#define GETNaNPC(X)   0L

#define NaN(X)  0
#define KILLNaN(X)
#endif

#endif 	/* _NAN_H */
