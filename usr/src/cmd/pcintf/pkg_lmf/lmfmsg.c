/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:pkg_lmf/lmfmsg.c	1.1"
/* SCCSID(@(#)lmfmsg.c	4.2	LCC)	/* Modified: 16:36:36 8/18/89 */

/*
 *  LMF Message printer
 */

#include <stdio.h>
#include "lmf.h"


main(argc, argv)
int argc;
char **argv;
{
	char *msg;
	int raw;

	raw = 0;
	if (argc > 3 && !strcmp(argv[1], "-r")) {
		argc--;
		argv++;
		raw++;
	}
	if (argc < 3) {
		fprintf(stderr, "usage: lmfmsg msg_file msg_id [arg] ...\n");
		exit(1);
	}
	if (lmf_open_file(argv[1], "En", "%N") < 0) {
		fprintf(stderr, "lmfmsg: Couldn't open message file %s\n",
			argv[1]);
		exit(2);
	}
	if ((msg = lmf_get_message(argv[2], NULL)) == NULL) {
		fprintf(stderr, "lmfmsg: Couldn't get message %s: %d\n",
			argv[2], lmf_errno);
		exit(3);
	}
	argc -= 3;
	argv += 3;
	while (lmf_message_length > 0) {
		if (!raw && *msg == '%') {
			if (*++msg > '0' && *msg <= '9') {
				if ((*msg - '1') < argc)
					printf("%s", argv[*msg - '1']);
				msg++;
				lmf_message_length -= 2;
				continue;
			}
			lmf_message_length--;
		}
		putchar(*msg);
		msg++;
		lmf_message_length--;
	}
	fflush(stdout);
	exit(0);
}
