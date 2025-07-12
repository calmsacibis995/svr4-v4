/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kdb:cmd/kdb.c	1.1.2.1"

/* This program causes a kernel debugger,
   if any is installed, to be invoked.

   Alternatively, it can be used to set the character
   which is used to invoke the debugger from the console
   (for this systems which allow this to be configurable),
   by invoking it with the arguments "char" and the
   ASCII character code. */

#include <stdio.h>
#include <sys/types.h>
#include <sys/sysi86.h>
#include <errno.h>

#ifdef E425M
#include <sys/ioctl.h>
#define EDENCHG (TIOC|35)    /* IOCTL number for EDEN change character case */
#define CHAR_OPT
#endif

extern long strtol();

char *_Progname;


main(argc, argv)
	int	argc;
	char	*argv[];
{
	_Progname = argv[0];

#ifdef CHAR_OPT
	if (argc == 3 && strcmp(argv[1], "char") == 0)
		return set_kdb_char((char)strtol(argv[2], NULL, 0));
	else
#endif
	if (argc == 1)
		return invoke_kdb();

#ifdef CHAR_OPT
	fprintf(stderr, "Usage: %s [\"char\" ASCII_code]\n", _Progname);
#else
	fprintf(stderr, "Usage: %s\n", _Progname);
#endif
	return 1;
}


int
invoke_kdb()
{
	if (sysi86(SI86TODEMON) < 0) {
		if (errno == EPERM || errno == EACCES)
			fprintf(stderr, "%s: Permission denied\n", _Progname);
		else
			perror(_Progname);
		return 1;
	}
	return 0;
}


#ifdef CHAR_OPT

int
set_kdb_char(char_code)
	char	char_code;
{
#ifdef E425M
	if (ioctl(2, EDENCHG, char_code) < 0) {
		perror(_Progname);
		return 1;
	}
	return 0;
#endif /* not E425M */
}

#endif /* CHAR_OPT */
