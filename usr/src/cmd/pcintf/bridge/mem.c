/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/mem.c	1.1.1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)mem.c	4.3	LCC);	/* Modified: 11:43:54 10/20/89 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include	<stdio.h>
#include	"const.h"


extern char
	*realloc(),			/* Standard C memory reallocator*/
	*malloc();			/* Standard C memory allocator	*/
extern long	ulimit();
	

char *
memory(amount)
register int	amount;
{
    register char	
	*newmem;		/* Newly allocated memory	*/

    if ((newmem = malloc((unsigned) amount)) == NULL)
	fatal(lmf_format_string((char *) NULL, 0,
		lmf_get_message("MEM1","memory: Can't get %1 bytes\n"),
		"%d", amount));

    return newmem;
}



char *
morememory(ptr, amount)
char
	*ptr;
register int
	amount;
{
register char	
	*newmem;		/* Newly allocated memory	*/

	if ((newmem = realloc(ptr, (unsigned) amount)) == NULL)
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("MEM2",
			"memory: Can't resize to %1\n"), "%d", amount));

	return newmem;
}
