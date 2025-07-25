/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rpcgen:rpc_main.c	1.8.3.2"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
#ifndef lint
static char sccsid[] = "@(#)rpc_main.c 1.30 89/03/30 (C) 1987 SMI";
#endif

/*
 * rpc_main.c, Top level of the RPC protocol compiler. 
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include "rpc_util.h"
#include "rpc_parse.h"
#include "rpc_scan.h"

#define EXTEND	1		/* alias for TRUE */

struct commandline {
	int cflag;		/* xdr C routines */
	int hflag;		/* header file */
	int lflag;		/* client side stubs */
	int mflag;		/* server side stubs */
	int nflag;		/* netid flag */
	int sflag;		/* server stubs for the given transport */
	int tflag;		/* dispatch table file */
	char *infile;		/* input module name */
	char *outfile;		/* output module name */
};

static char *cmdname;
static char *svcclosetime = "120";
static char CPP[] = "/usr/ccs/lib/cpp";
static char CPPFLAGS[] = "-C";
static char *allv[] = {
	"rpcgen", "-s", "udp", "-s", "tcp",
};
static int allc = sizeof(allv)/sizeof(allv[0]);
static char *allnv[] = {
	"rpcgen", "-s", "netpath",
};
static int allnc = sizeof(allnv)/sizeof(allnv[0]);

/*
 * machinations for handling expanding argument list
 */
static void addarg();		/* add another argument to the list */

#define ARGLISTLEN	20
static char *arglist[ARGLISTLEN] = {
	CPP, CPPFLAGS,
};
static int argcount = 2;

int nonfatalerrors;	/* errors */
int inetdflag;		/* Support for inetd */
int pmflag;		/* Support for port monitors */
int logflag;		/* Use syslog instead of fprintf for errors */
int tblflag;		/* Support for dispatch table file */
int indefinitewait;	/* If started by port monitors, hang till it wants */
int exitnow;		/* If started by port monitors, exit after the call */
int timerflag;		/* TRUE if !indefinite && !exitnow */

main(argc, argv)
	int argc;
	char *argv[];
{
	struct commandline cmd;

	(void) memset((char *)&cmd, 0, sizeof (struct commandline));
	if (!parseargs(argc, argv, &cmd))
		usage();

	if (cmd.cflag) {
		c_output(cmd.infile, "-DRPC_XDR", !EXTEND, cmd.outfile);
	} else if (cmd.hflag) {
		h_output(cmd.infile, "-DRPC_HDR", !EXTEND, cmd.outfile);
	} else if (cmd.lflag) {
		l_output(cmd.infile, "-DRPC_CLNT", !EXTEND, cmd.outfile);
	} else if (cmd.sflag || cmd.mflag || (cmd.nflag)) {
		s_output(argc, argv, cmd.infile, "-DRPC_SVC", !EXTEND, 
			 cmd.outfile, cmd.mflag, cmd.nflag);
	} else if (cmd.tflag) {
		t_output(cmd.infile, "-DRPC_TBL", !EXTEND, cmd.outfile);
	} else {
		/* the rescans are required, since cpp may effect input */
		c_output(cmd.infile, "-DRPC_XDR", EXTEND, "_xdr.c");
		reinitialize();
		h_output(cmd.infile, "-DRPC_HDR", EXTEND, ".h");
		reinitialize();
		l_output(cmd.infile, "-DRPC_CLNT", EXTEND, "_clnt.c");
		reinitialize();
		if (inetdflag)
			s_output(allc, allv, cmd.infile, "-DRPC_SVC", EXTEND, 
					"_svc.c", cmd.mflag, cmd.nflag);
		else
			s_output(allnc, allnv, cmd.infile, "-DRPC_SVC", EXTEND, 
					"_svc.c", cmd.mflag, cmd.nflag);
		if (tblflag) {
			reinitialize();
			t_output(cmd.infile, "-DRPC_TBL", EXTEND, "_tbl.i");
		}
	}
	exit(nonfatalerrors);
	/* NOTREACHED */
}

/*
 * add extension to filename 
 */
static char *
extendfile(file, ext)
	char *file;
	char *ext;
{
	char *res;
	char *p;

	res = alloc(strlen(file) + strlen(ext) + 1);
	if (res == NULL) {
		abort();
	}
	p = strrchr(file, '.');
	if (p == NULL) {
		p = file + strlen(file);
	}
	(void) strcpy(res, file);
	(void) strcpy(res + (p - file), ext);
	return (res);
}

/*
 * Open output file with given extension 
 */
static
open_output(infile, outfile)
	char *infile;
	char *outfile;
{
	if (outfile == NULL) {
		fout = stdout;
		add_warning();
		return;
	}
	if (infile != NULL && streq(outfile, infile)) {
		f_print(stderr, "%s: output would overwrite %s\n", cmdname,
			infile);
		crash();
	}
	fout = fopen(outfile, "w");
	if (fout == NULL) {
		f_print(stderr, "%s: unable to open ", cmdname);
		perror(outfile);
		crash();
	}
	record_open(outfile);
	add_warning();
}

static
add_warning()
{
	f_print(fout, "/*\n");
	f_print(fout, " * Please do not edit this file.\n");
	f_print(fout, " * It was generated using rpcgen.\n");
	f_print(fout, " */\n\n");
}

/*
 * Open input file with given define for C-preprocessor 
 */
static
open_input(infile, define)
	char *infile;
	char *define;
{
	int pd[2];

	infilename = (infile == NULL) ? "<stdin>" : infile;
	(void) pipe(pd);
	switch (fork()) {
	case 0:
		addarg(define);
		addarg(infile);
		addarg((char *)NULL);
		(void) close(1);
		(void) dup2(pd[1], 1);
		(void) close(pd[0]);
		execv(arglist[0], arglist);
		perror("execv");
		exit(1);
	case -1:
		perror("fork");
		exit(1);
	}
	(void) close(pd[1]);
	fin = fdopen(pd[0], "r");
	if (fin == NULL) {
		f_print(stderr, "%s: ", cmdname);
		perror(infilename);
		crash();
	}
}

/*
 * Compile into an XDR routine output file
 */
static
c_output(infile, define, extend, outfile)
	char *infile;
	char *define;
	int extend;
	char *outfile;
{
	definition *def;
	char *include;
	char *outfilename;
	long tell;

	open_input(infile, define);	
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	f_print(fout, "#include <rpc/rpc.h>\n");
	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	}
	tell = ftell(fout);
	while (def = get_definition()) {
		emit(def);
	}
	if (extend && tell == ftell(fout)) {
		(void) unlink(outfilename);
	}
}

char rpcgen_table_dcl[] = "struct rpcgen_table {\n\
	char	*(*proc)();\n\
	xdrproc_t	xdr_arg;\n\
	unsigned	len_arg;\n\
	xdrproc_t	xdr_res;\n\
	unsigned	len_res;\n\
};\n";

/*
 * Compile into an XDR header file
 */
static
h_output(infile, define, extend, outfile)
	char *infile;
	char *define;
	int extend;
	char *outfile;
{
	definition *def;
	char *outfilename;
	long tell;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	tell = ftell(fout);
	f_print(fout, "#include <rpc/types.h>\n\n");
	while (def = get_definition()) {
		print_datadef(def);
	}
	if (extend && tell == ftell(fout)) {
		(void) unlink(outfilename);
	} else if (tblflag) {
		f_print(fout, rpcgen_table_dcl);
	}
}

/*
 * Compile into an RPC service
 */
static
s_output(argc, argv, infile, define, extend, outfile, nomain, netflag)
	int argc;
	char *argv[];
	char *infile;
	char *define;
	int extend;
	char *outfile;
	int nomain;
	int netflag;
{
	char *include;
	definition *def;
	int foundprogram = 0;
	char *outfilename;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	f_print(fout, "#include <stdio.h>\n");
	if (strcmp(svcclosetime, "-1") == 0)
		indefinitewait = 1;
	else if (strcmp(svcclosetime, "0") == 0)
		exitnow = 1;
	else if (inetdflag || pmflag) {
		f_print(fout, "#include <signal.h>\n");
		timerflag = 1;
	}
	f_print(fout, "#include <rpc/rpc.h>\n");
	f_print(fout, "#include <memory.h>\n");
	f_print(fout, "#include <stropts.h>\n");
	if (inetdflag) {
		f_print(fout, "#include <sys/socket.h>\n");
		f_print(fout, "#include <netinet/in.h>\n");
	}
	if (netflag || pmflag) {
		f_print(fout, "#include <netconfig.h>\n");
		f_print(fout, "#include <stropts.h>\n");
	}
	if (timerflag)
		f_print(fout, "#include <sys/resource.h>\n");
	if (logflag || inetdflag || pmflag) {
		f_print(fout, "#ifdef SYSLOG\n");
		f_print(fout, "#include <syslog.h>\n");
		f_print(fout, "#else\n");
		f_print(fout, "#define LOG_ERR 1\n");
		f_print(fout, "#define openlog(a, b, c)\n");
		f_print(fout, "#endif\n");
	}
	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	}

	f_print(fout, "\n#ifdef DEBUG\n#define RPC_SVC_FG\n#endif\n");
	if (timerflag)
		f_print(fout, "\n#define _RPCSVC_CLOSEDOWN %s\n", svcclosetime);
	while (def = get_definition()) {
		foundprogram |= (def->def_kind == DEF_PROGRAM);
	}
	if (extend && !foundprogram) {
		(void) unlink(outfilename);
		return;
	}
	if (nomain) {
		if (inetdflag || pmflag) {
			f_print(fout, "\nextern int _rpcpmstart;");
			f_print(fout, "\t\t/* Started by a port monitor ? */\n");
			f_print(fout, "extern int _rpcfdtype;");
			f_print(fout, "\t\t/* Whether Stream or Datagram ? */\n");
			if (timerflag) {
				f_print(fout, "static int _rpcsvcdirty;");
				f_print(fout, "\t/* Still serving ? */\n");
			}
		}
		write_programs((char *)NULL);
	} else {
		write_most(infile, netflag);
		do_registers(argc, argv);
		write_rest();
		write_programs("static");
	}
	if (inetdflag || pmflag)
		write_svc_aux();
}

/*
 * generate client side stubs
 */
static
l_output(infile, define, extend, outfile)
	char *infile;
	char *define;
	int extend;
	char *outfile;
{
	char *include;
	definition *def;
	int foundprogram = 0;
	char *outfilename;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	f_print(fout, "#include <rpc/rpc.h>\n");
	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	}
	while (def = get_definition()) {
		foundprogram |= (def->def_kind == DEF_PROGRAM);
	}
	if (extend && !foundprogram) {
		(void) unlink(outfilename);
		return;
	}
	write_stubs();
}

/*
 * generate the dispatch table
 */
static
t_output(infile, define, extend, outfile)
	char *infile;
	char *define;
	int extend;
	char *outfile;
{
	definition *def;
	int foundprogram = 0;
	char *outfilename;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	while (def = get_definition()) {
		foundprogram |= (def->def_kind == DEF_PROGRAM);
	}
	if (extend && !foundprogram) {
		(void) unlink(outfilename);
		return;
	}
	write_tables();
}

/*
 * Perform registrations for service output 
 */
static
do_registers(argc, argv)
	int argc;
	char *argv[];
{
	int i;

	for (i = 1; i < argc; i++)
		if (inetdflag && (streq(argv[i], "-s"))) {
			/* Only tcp and udp allowed with inetdflag */
			if (streq(argv[i + 1], "tcp") ||
				streq(argv[i + 1], "udp")) {
				write_inetd_register(argv[i + 1]);
				i++;
			} else {
				(void) f_print(stderr,
				"Illegal nettype with -I option\n");
				usage();
			}
		} else if (streq(argv[i], "-s")) {
			write_nettype_register(argv[i + 1]);
			i++;
		} else if (streq(argv[i], "-n")) {
			write_netid_register(argv[i + 1]);
			i++;
		}
}

/*
 * Add another argument to the arg list
 */
static void
addarg(cp)
	char *cp;
{
	if (argcount >= ARGLISTLEN) {
		f_print(stderr, "rpcgen: too many defines\n");
		crash();
		/*NOTREACHED*/
	}
	arglist[argcount++] = cp;
}

/*
 * Parse command line arguments 
 */
static
parseargs(argc, argv, cmd)
	int argc;
	char *argv[];
	struct commandline *cmd;
{
	int i;
	int j;
	char c;
	char flag[(1 << 8 * sizeof(char))];
	int nflags;

	cmdname = argv[0];
	cmd->infile = cmd->outfile = NULL;
	if (argc < 2) {
		return (0);
	}
	flag['c'] = 0;
	flag['h'] = 0;
	flag['l'] = 0;
	flag['m'] = 0;
	flag['o'] = 0;
	flag['s'] = 0;
	flag['t'] = 0;
	flag['n'] = 0;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (cmd->infile) {
				return (0);
			}
			cmd->infile = argv[i];
		} else {
			for (j = 1; argv[i][j] != 0; j++) {
				c = argv[i][j];
				switch (c) {
				case 'c':
				case 'h':
				case 'l':
				case 'm':
				case 't':
					if (flag[c]) {
						return (0);
					}
					flag[c] = 1;
					break;
				case 'I':
					inetdflag = 1;
					break;
				case 'L':
					logflag = 1;
					break;
				case 'K':
					if (++i == argc) {
						return (0);
					}
					svcclosetime = argv[i];
					goto nextarg;
				case 'T':
					tblflag = 1;
					break;
				case 'n':
				case 'o':
				case 's':
					if (argv[i][j - 1] != '-' || 
					    argv[i][j + 1] != 0) {
						return (0);
					}
					flag[c] = 1;
					if (++i == argc) {
						return (0);
					}
					if (c == 'o') {
						if (cmd->outfile) {
							return (0);
						}
						cmd->outfile = argv[i];
					}
					goto nextarg;
				case 'D':
					if (argv[i][j - 1] != '-') {
						return (0);
					}
					(void) addarg(argv[i]);
					goto nextarg;

				default:
					return (0);
				}
			}
	nextarg:
			;
		}
	}
	/* nettype not allowed with inetdflag */
	if (inetdflag && cmd->nflag)
		return (0);
	/*
	 * Currently (pmflag | inetdflag) is always TRUE. rpcgen may
	 * be changed to provide for only self started servers
	 */
	pmflag = inetdflag ? 0 : 1;	/* the opposite */
	cmd->cflag = flag['c'];
	cmd->hflag = flag['h'];
	cmd->lflag = flag['l'];
	cmd->mflag = flag['m'];
	cmd->nflag = flag['n'];
	cmd->sflag = flag['s'];
	cmd->tflag = flag['t'];
	nflags = cmd->cflag + cmd->hflag + cmd->lflag + cmd->mflag +
			cmd->nflag + cmd->sflag + cmd->tflag;
	if (nflags == 0) {
		if (cmd->outfile != NULL || cmd->infile == NULL) {
			return (0);
		}
	} else if (nflags > 1)
		return (0);
	return (1);
}

static
usage()
{
	f_print(stderr, "usage: %s infile\n", cmdname);
	f_print(stderr, "\t%s [-Dname[=value]] [-K seconds] [-L] [-T] infile\n",
			cmdname);
	f_print(stderr, "\t%s [-c | -h | -l | -m | -t] [-o outfile] [infile]\n",
			cmdname);
	f_print(stderr, "\t%s [-s nettype]* [-o outfile] [infile]\n", cmdname);
	f_print(stderr, "\t%s [-n netid]* [-o outfile] [infile]\n", cmdname);
	exit(1);
}
