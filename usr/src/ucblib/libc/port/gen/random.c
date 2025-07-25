/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ucblibc:port/gen/random.c	1.3.3.1"

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

#include	<stdio.h>

/*
 * random.c:
 * An improved random number generation package.  In addition to the standard
 * rand()/srand() like interface, this package also has a special state info
 * interface.  The initstate() routine is called with a seed, an array of
 * bytes, and a count of how many bytes are being passed in; this array is then
 * initialized to contain information for random number generation with that
 * much state information.  Good sizes for the amount of state information are
 * 32, 64, 128, and 256 bytes.  The state can be switched by calling the
 * setstate() routine with the same array as was initiallized with initstate().
 * By default, the package runs with 128 bytes of state information and
 * generates far better random numbers than a linear congruential generator.
 * If the amount of state information is less than 32 bytes, a simple linear
 * congruential R.N.G. is used.
 * Internally, the state information is treated as an array of longs; the
 * zeroeth element of the array is the type of R.N.G. being used (small
 * integer); the remainder of the array is the state information for the
 * R.N.G.  Thus, 32 bytes of state information will give 7 longs worth of
 * state information, which will allow a degree seven polynomial.  (Note: the 
 * zeroeth word of state information also has some other information stored
 * in it -- see setstate() for details).
 * The random number generation technique is a linear feedback shift register
 * approach, employing trinomials (since there are fewer terms to sum up that
 * way).  In this approach, the least significant bit of all the numbers in
 * the state table will act as a linear feedback shift register, and will have
 * period 2^deg - 1 (where deg is the degree of the polynomial being used,
 * assuming that the polynomial is irreducible and primitive).  The higher
 * order bits will have longer periods, since their values are also influenced
 * by pseudo-random carries out of the lower bits.  The total period of the
 * generator is approximately deg*(2**deg - 1); thus doubling the amount of
 * state information has a vast influence on the period of the generator.
 * Note: the deg*(2**deg - 1) is an approximation only good for large deg,
 * when the period of the shift register is the dominant factor.  With deg
 * equal to seven, the period is actually much longer than the 7*(2**7 - 1)
 * predicted by this formula.
 */



/*
 * For each of the currently supported random number generators, we have a
 * break value on the amount of state information (you need at least this
 * many bytes of state info to support this random number generator), a degree
 * for the polynomial (actually a trinomial) that the R.N.G. is based on, and
 * the separation between the two lower order coefficients of the trinomial.
 */

#define		TYPE_0		0		/* linear congruential */
#define		BREAK_0		8
#define		DEG_0		0
#define		SEP_0		0

#define		TYPE_1		1		/* x**7 + x**3 + 1 */
#define		BREAK_1		32
#define		DEG_1		7
#define		SEP_1		3

#define		TYPE_2		2		/* x**15 + x + 1 */
#define		BREAK_2		64
#define		DEG_2		15
#define		SEP_2		1

#define		TYPE_3		3		/* x**31 + x**3 + 1 */
#define		BREAK_3		128
#define		DEG_3		31
#define		SEP_3		3

#define		TYPE_4		4		/* x**63 + x + 1 */
#define		BREAK_4		256
#define		DEG_4		63
#define		SEP_4		1


/*
 * Array versions of the above information to make code run faster -- relies
 * on fact that TYPE_i == i.
 */

#define		MAX_TYPES	5		/* max number of types above */

static struct _randomjunk {
	int	degrees[MAX_TYPES];
	int	seps[MAX_TYPES];
	long	randtbl[ DEG_3 + 1 ];
/*
 * fptr and rptr are two pointers into the state info, a front and a rear
 * pointer.  These two pointers are always rand_sep places aparts, as they cycle
 * cyclically through the state information.  (Yes, this does mean we could get
 * away with just one pointer, but the code for random() is more efficient this
 * way).  The pointers are left positioned as they would be from the call
 *			initstate( 1, randtbl, 128 )
 * (The position of the rear pointer, rptr, is really 0 (as explained above
 * in the initialization of randtbl) because the state table pointer is set
 * to point to randtbl[1] (as explained below).
 */
	long	*fptr, *rptr;
/*
 * The following things are the pointer to the state information table,
 * the type of the current generator, the degree of the current polynomial
 * being used, and the separation between the two pointers.
 * Note that for efficiency of random(), we remember the first location of
 * the state information, not the zeroeth.  Hence it is valid to access
 * state[-1], which is used to store the type of the R.N.G.
 * Also, we remember the last location, since this is more efficient than
 * indexing every time to find the address of the last element to see if
 * the front and rear pointers have wrapped.
 */
	long	*state;
	int	rand_type, rand_deg, rand_sep;
	long	*end_ptr;
} *__randomjunk, *_randomjunk(), _randominit = {
	/*
	 * Initially, everything is set up as if from :
	 *		initstate( 1, &randtbl, 128 );
	 * Note that this initialization takes advantage of the fact
	 * that srandom() advances the front and rear pointers 10*rand_deg
	 * times, and hence the rear pointer which starts at 0 will also
	 * end up at zero; thus the zeroeth element of the state
	 * information, which contains info about the current
	 * position of the rear pointer is just
	 *	MAX_TYPES*(rptr - state) + TYPE_3 == TYPE_3.
	 */
	{ DEG_0, DEG_1, DEG_2, DEG_3, DEG_4 },
	{ SEP_0, SEP_1, SEP_2, SEP_3, SEP_4 },
	{ TYPE_3,
	    0x9a319039, 0x32d9c024, 0x9b663182, 0x5da1f342, 
	    0xde3b81e0, 0xdf0a6fb5, 0xf103bc02, 0x48f340fb, 
	    0x7449e56b, 0xbeb1dbb0, 0xab5c5918, 0x946554fd, 
	    0x8c2e680f, 0xeb3d799f, 0xb11ee0b7, 0x2d436b86, 
	    0xda672e2a, 0x1588ca88, 0xe369735d, 0x904f35f7, 
	    0xd7158fd6, 0x6fa6f051, 0x616e6b96, 0xac94efdc, 
	    0x36413f93, 0xc622c298, 0xf5a42ab8, 0x8a88d77b, 
			0xf5ad9d0e, 0x8999220b, 0x27fb47b9 },
	&_randominit.randtbl[ SEP_3 + 1 ],
	&_randominit.randtbl[ 1 ],
	&_randominit.randtbl[ 1 ],
	TYPE_3, DEG_3, SEP_3,
	&_randominit.randtbl[ DEG_3 + 1]
};

static struct _randomjunk *
_randomjunk()
{
	register struct _randomjunk *rp = __randomjunk;

	if (rp == 0) {
		rp = (struct _randomjunk *)malloc(sizeof (*rp));
		if (rp == 0)
			return (0);
		*rp = _randominit;
		__randomjunk = rp;
	}
	return (rp);
}

/*
 * srandom:
 * Initialize the random number generator based on the given seed.  If the
 * type is the trivial no-state-information type, just remember the seed.
 * Otherwise, initializes state[] based on the given "seed" via a linear
 * congruential generator.  Then, the pointers are set to known locations
 * that are exactly rand_sep places apart.  Lastly, it cycles the state
 * information a given number of times to get rid of any initial dependencies
 * introduced by the L.C.R.N.G.
 * Note that the initialization of randtbl[] for default usage relies on
 * values produced by this routine.
 */

srandom( x )

    unsigned		x;
{
	register struct _randomjunk *rp = _randomjunk();
    	register  int		i, j;
	extern long random();

	if (rp == 0)
		return;
	if(  rp->rand_type  ==  TYPE_0  )  {
	    rp->state[ 0 ] = x;
	}
	else  {
	    j = 1;
	    rp->state[ 0 ] = x;
	    for( i = 1; i < rp->rand_deg; i++ )  {
		rp->state[i] = 1103515245*rp->state[i - 1] + 12345;
	    }
	    rp->fptr = &rp->state[ rp->rand_sep ];
	    rp->rptr = &rp->state[ 0 ];
	    for( i = 0; i < 10*rp->rand_deg; i++ )  (void)random();
	}
}



/*
 * initstate:
 * Initialize the state information in the given array of n bytes for
 * future random number generation.  Based on the number of bytes we
 * are given, and the break values for the different R.N.G.'s, we choose
 * the best (largest) one we can and set things up for it.  srandom() is
 * then called to initialize the state information.
 * Note that on return from srandom(), we set state[-1] to be the type
 * multiplexed with the current value of the rear pointer; this is so
 * successive calls to initstate() won't lose this information and will
 * be able to restart with setstate().
 * Note: the first thing we do is save the current state, if any, just like
 * setstate() so that it doesn't matter when initstate is called.
 * Returns a pointer to the old state.
 */

char  *
initstate( seed, arg_state, n )

    unsigned		seed;			/* seed for R. N. G. */
    char		*arg_state;		/* pointer to state array */
    int			n;			/* # bytes of state info */
{
	register struct _randomjunk *rp = _randomjunk();
	register  char		*ostate;

	if (rp == 0)
		return (0);
	ostate = (char *)( &rp->state[ -1 ] );

	if(  rp->rand_type  ==  TYPE_0  )  rp->state[ -1 ] = rp->rand_type;
	else  rp->state[ -1 ] =
	    MAX_TYPES*(rp->rptr - rp->state) + rp->rand_type;
	if(  n  <  BREAK_1  )  {
	    if(  n  <  BREAK_0  )  {
		fprintf( stderr, "initstate: not enough state (%d bytes) with which to do jack; ignored.\n" );
		return (0);
	    }
	    rp->rand_type = TYPE_0;
	    rp->rand_deg = DEG_0;
	    rp->rand_sep = SEP_0;
	}
	else  {
	    if(  n  <  BREAK_2  )  {
		rp->rand_type = TYPE_1;
		rp->rand_deg = DEG_1;
		rp->rand_sep = SEP_1;
	    }
	    else  {
		if(  n  <  BREAK_3  )  {
		    rp->rand_type = TYPE_2;
		    rp->rand_deg = DEG_2;
		    rp->rand_sep = SEP_2;
		}
		else  {
		    if(  n  <  BREAK_4  )  {
			rp->rand_type = TYPE_3;
			rp->rand_deg = DEG_3;
			rp->rand_sep = SEP_3;
		    }
		    else  {
			rp->rand_type = TYPE_4;
			rp->rand_deg = DEG_4;
			rp->rand_sep = SEP_4;
		    }
		}
	    }
	}
	rp->state = &(  ( (long *)arg_state )[1]  );	/* first location */
	rp->end_ptr = &rp->state[ rp->rand_deg ];	/* must set end_ptr before srandom */
	srandom( seed );
	if(  rp->rand_type  ==  TYPE_0  )  rp->state[ -1 ] = rp->rand_type;
	else  rp->state[ -1 ] = MAX_TYPES*(rp->rptr - rp->state) + rp->rand_type;
	return( ostate );
}



/*
 * setstate:
 * Restore the state from the given state array.
 * Note: it is important that we also remember the locations of the pointers
 * in the current state information, and restore the locations of the pointers
 * from the old state information.  This is done by multiplexing the pointer
 * location into the zeroeth word of the state information.
 * Note that due to the order in which things are done, it is OK to call
 * setstate() with the same state as the current state.
 * Returns a pointer to the old state information.
 */

char  *
setstate( arg_state )

    char		*arg_state;
{
	register struct _randomjunk *rp = _randomjunk();
	register  long		*new_state;
	register  int		type;
	register  int		rear;
	char			*ostate;

	if (rp == 0)
		return (0);
	new_state = (long *)arg_state;
	type = new_state[0]%MAX_TYPES;
	rear = new_state[0]/MAX_TYPES;
	ostate = (char *)( &rp->state[ -1 ] );

	if(  rp->rand_type  ==  TYPE_0  )  rp->state[ -1 ] = rp->rand_type;
	else  rp->state[ -1 ] = MAX_TYPES*(rp->rptr - rp->state) + rp->rand_type;
	switch(  type  )  {
	    case  TYPE_0:
	    case  TYPE_1:
	    case  TYPE_2:
	    case  TYPE_3:
	    case  TYPE_4:
		rp->rand_type = type;
		rp->rand_deg = rp->degrees[ type ];
		rp->rand_sep = rp->seps[ type ];
		break;

	    default:
		fprintf( stderr, "setstate: state info has been munged; not changed.\n" );
	}
	rp->state = &new_state[ 1 ];
	if(  rp->rand_type  !=  TYPE_0  )  {
	    rp->rptr = &rp->state[ rear ];
	    rp->fptr = &rp->state[ (rear + rp->rand_sep)%rp->rand_deg ];
	}
	rp->end_ptr = &rp->state[ rp->rand_deg ];	/* set end_ptr too */
	return( ostate );
}



/*
 * random:
 * If we are using the trivial TYPE_0 R.N.G., just do the old linear
 * congruential bit.  Otherwise, we do our fancy trinomial stuff, which is the
 * same in all ther other cases due to all the global variables that have been
 * set up.  The basic operation is to add the number at the rear pointer into
 * the one at the front pointer.  Then both pointers are advanced to the next
 * location cyclically in the table.  The value returned is the sum generated,
 * reduced to 31 bits by throwing away the "least random" low bit.
 * Note: the code takes advantage of the fact that both the front and
 * rear pointers can't wrap on the same call by not testing the rear
 * pointer if the front one has wrapped.
 * Returns a 31-bit random number.
 */

long
random()
{
	register struct _randomjunk *rp = _randomjunk();
	long		i;
	
	if (rp == 0)
		return (0);
	if(  rp->rand_type  ==  TYPE_0  )  {
	    i = rp->state[0] = ( rp->state[0]*1103515245 + 12345 )&0x7fffffff;
	}
	else  {
	    *rp->fptr += *rp->rptr;
	    i = (*rp->fptr >> 1)&0x7fffffff;	/* chucking least random bit */
	    if(  ++rp->fptr  >=  rp->end_ptr  )  {
		rp->fptr = rp->state;
		++rp->rptr;
	    }
	    else  {
		if(  ++rp->rptr  >=  rp->end_ptr  )  rp->rptr = rp->state;
	    }
	}
	return( i );
}
