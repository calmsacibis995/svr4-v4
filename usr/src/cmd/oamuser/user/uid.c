/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/uid.c	1.5.5.1"



#include	<sys/types.h>
#include	<stdio.h>
#include	<userdefs.h>

#include	<sys/param.h>
#ifndef	MAXUID
#include	<limits.h>
#ifdef UID_MAX
#define	MAXUID	UID_MAX
#else 
#define	MAXUID	60000
#endif
#endif

extern pid_t getpid();

static char cmdbuf[ 128 ];

uid_t
findnextuid()
{
	FILE *fptr;
	char fname[ 15 ];
	uid_t last, next;

	/*
		Sort the used UIDs in decreasing order to return
		MAXUSED + 1
	*/
	(void) sprintf( fname, "/tmp/%ld", getpid() );
	(void) sprintf( cmdbuf,
		"sh -c \"cut -f3 -d: </etc/passwd|sort -nr|uniq > %s\"",
		fname );

	if( system( cmdbuf ) )
		return( -1 );

	if( (fptr = fopen( fname, "r" )) == NULL )
		return( -1 );

	if( fscanf( fptr, "%ld\n", &next ) == EOF )
		return( DEFRID + 1 );

	/* Still some UIDs left between next and MAXUID */
	if( next < MAXUID )
		return( next <= DEFRID? DEFRID + 1: next + 1 );

	/* Look for the next unused one */
	for( last = next; fscanf( fptr, "%ld\n", &next ) != EOF; last = next ) {
		if( next <= DEFRID ) {
			if( last != DEFRID + 1 )
				return( DEFRID + 1 );
			else
				return( -1 );
		}
		if( (last != (next + 1)) && ((next + 1) < MAXUID) )
			return( next + 1 );
	}

	/* None left */
	return( -1 );
}
