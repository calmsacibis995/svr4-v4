/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:pkg_lcs/stricmp.c	1.1"
/* SCCSID(@(#)stricmp.c	4.1	LCC)	* Modified: 18:14:03 8/15/89 */
/*
 *  stricmp function
 */

#include <ctype.h>


stricmp(s1, s2)
register char *s1, *s2;
{
	if (s1 == s2)
		return 0;
	while (tolower(*s1) == tolower(*s2)) {
		if (*s1++ == '\0')
			return 0;
		s2++;
	}
	return *s1 - *s2;
}
