/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)valtools:ckyorn.c	1.2.3.1"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "usage.h"

extern int	optind, ckquit, ckindent, ckwidth;
extern char	*optarg;

extern long	atol();
extern void	exit(), ckyorn_err(), ckyorn_hlp();
extern int	getopt(), ckyorn(), ckyorn_val();

#define BADPID	(-2)

char	*prog,
	*deflt,
	*prompt,
	*error,
	*help;
int	kpid = BADPID;
int	signo;

char	husage[] = "Wh";
char	eusage[] = "We";

void
usage()
{
	switch(*prog) {
	  default:
		(void) fprintf(stderr, "usage: %s [options]\n", prog);
		(void) fputs(OPTMESG, stderr);
		(void) fputs(STDOPTS, stderr);
		break;

	  case 'v':
		(void) fprintf(stderr, "usage: %s input\n", prog);
		break;

	  case 'h':
		(void) fprintf(stderr, "usage: %s [options]\n", prog);
		(void) fputs(OPTMESG, stderr);
		(void) fputs("\t-W width\n\t-h help\n", stderr);
		break;

	  case 'e':
		(void) fprintf(stderr, "usage: %s [options]\n", prog);
		(void) fputs(OPTMESG, stderr);
		(void) fputs("\t-W width\n\t-e error\n", stderr);
		break;
	}
	exit(1);
}

main(argc, argv)
int argc;
char *argv[];
{
	int c, n;
	char ynval[64]; /* just in case a bogus default is sent in */

	prog = strrchr(argv[0], '/');
	if(!prog++)
		prog = argv[0];

	while((c=getopt(argc, argv, "d:p:e:h:k:s:QW:?")) != EOF) {
		/* check for invalid option */
		if(*prog == 'v')
			usage();
		if((*prog == 'e') && !strchr(eusage, c))
			usage();
		if((*prog == 'h') && !strchr(husage, c))
			usage();

		switch(c) {
		  case 'Q':
			ckquit = 0;
			break;

		  case 'W':
			ckwidth = atol(optarg);
			if(ckwidth < 0) {
				progerr("negative display width specified");
				exit(1);
			}
			break;

		  case 'd':
			deflt = optarg;
			break;

		  case 'p':
			prompt = optarg;
			break;

		  case 'e':
			error = optarg;
			break;

		  case 'h':
			help = optarg;
			break;

		  case 'k':
			kpid = atol(optarg);
			break;
			
		  case 's':
			signo = atol(optarg);
			break;

		  default:
			usage();
		}
	}
		
	if(signo) {
		if(kpid == BADPID)
			usage();
	} else
		signo = SIGTERM;

	if(*prog == 'v') {
		if(argc != (optind+1))
			usage();
		if (ckyorn_val(argv[optind]))
			exit(1);
		else
			exit(0);
	}

	if(optind != argc)
		usage();

	if(*prog == 'e') {
		ckindent = 0;
		ckyorn_err(error);
		exit(0);
	} else if(*prog == 'h') {
		ckindent = 0;
		ckyorn_hlp(help);
		exit(0);
	}

	n = ckyorn(ynval, deflt, error, help, prompt);
	if(n == 3) {
		if(kpid > -2) {
			if(kill(kpid, signo)) {
				fprintf(stderr, 
				"Failed to send signal %d to process %d\n", 
				signo, kpid);
			}
		}
		(void) puts("q");
	} else if(n == 0)
		(void) fputs(ynval, stdout);
	exit(n);
}
