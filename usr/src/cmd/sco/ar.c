/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sco:ar.c	1.1"

/*
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/* Enhanced Application Compatibility Support */
#include <stdio.h>
#include "argtype.h"

#define UTILNAME "ar"

main(argc, argv)
int argc;
char *argv[];
{
	register char **av;
	int ac;

	av = &argv[1];		/* save argc and argv for exec */
	ac = argc;

	while ( --ac > 0 )	/* check all args except argv[0] */
	{
	    switch ( argtype( *av++ ) )
	    {
		case XARCH:		/* if any xenix archive */
		case OMF:		/* or any omf file */
		case XOUT:		/* or x.out */
		    executil( XOUT, UTILNAME, argv);	/* run x.out util */
		    break;
	    }
	}
	executil( COFF, UTILNAME, argv);	/* otherwise run coff util */
}
/* End Enhanced Application Compatibility Support */
