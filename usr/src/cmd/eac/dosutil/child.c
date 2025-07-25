/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)eac:dosutil/child.c	1.1"
/*	@(#) child.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/*	child(path,separator)  --  returns a pointer to the last component
 *			of the path, given the component separator.
 */

#include	<stdio.h>
#include	"dosutil.h"


char *child(path,separator)
char *path, separator;
{
	char *c;

	if ((c = strrchr(path,separator)) == NULL)
		return(path);
	return(++c);
}
