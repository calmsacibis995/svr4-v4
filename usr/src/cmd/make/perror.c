/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)make:perror.c	1.4"
/*	@(#)make:perror.c	1.3 of 11/22/85	*/

extern int errno;
main(ac,av)
int ac;
char **av;
{
	errno = atoi(av[1]);

	perror("");
}
