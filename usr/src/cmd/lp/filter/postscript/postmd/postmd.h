/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:filter/postscript/postmd/postmd.h	1.1.2.1"

/*
 *
 * Definitions used by the matrix display program.
 *
 * An interval list used to map matrix elements into integers in the range 0 to
 * 254 representing shades of gray on a PostScript printer. The list can be given
 * using the -i option or can be set in the optional header that can preceed each
 * matrix. The list should be a comma or space separated list that looks like,
 *
 *		num1,num2, ... ,numn
 *
 * where each num is a floating point number. The list must be given in increasing
 * numerical order. The n numbers in the list partion the real line into 2n+1
 * regions given by,
 *
 *		region1		element < num1
 *		region2		element = num1
 *		region3		element < num2
 *		region4		element = num3
 *		   .		     .
 *		   .		     .
 *		   .		     .
 *		region2n	element = numn
 *		region2n+1	element > numn
 *
 * Every number in a given region is mapped into an integer in the range 0 to 254
 * and that number, when displayed on a PostScript printer using the image operator,
 * prints as a square filled with a gray scale that reflects the integer that was
 * chosen. 0 maps to black and 255 white (that's why 255 is normally omitted).
 *
 * The shades of gray chosen by the program are normally generated automatically,
 * but can be reassigned using the -g option or by including a grayscale line in
 * the optional header. The grayscale list is comma or space separated list of
 * integers between 0 and 255 that's used to map individual regions into arbitray
 * shade of gray, thus overriding the default choice made in the program. The list
 * should look like,
 *
 *		color1,color2, ... ,color2n+1
 *
 * where color1 applies to region1 and color2n+1 applies to region2n+1. If less
 * than 2n+1 numbers are given the default assignments will be used for the missing
 * regions. Each color must be an integer in the range 0 to 255.
 *
 * The default interval list is given below. The default grayscale maps 254 (almost
 * white) into the first region and 0 (black) into the last.
 *
 */

#define DFLTILIST	"-1,0,1"

/*
 *
 * The active interval list is built from an interval string and stored in an array
 * whose elements are of type Ilist.
 *
 */

typedef struct  {

	double	val;			/* only valid in kind is ENDPOINT */
	int	color;			/* gray scale color */
	long	count;			/* statistics for each region */

} Ilist;

/*
 *
 * Non-integer function declarations.
 *
 */

char	*savestring();

