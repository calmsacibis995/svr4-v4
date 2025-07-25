/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sco:ar_xout.c	1.1"

/*
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */

/* Enhanced Application Compatibility Support */
/* ar: UNIX Archive Maintainer */


#define FLEXNAMES
#define OLDAR

#include <stdio.h>
#include <signal.h>
#include <values.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ar.h"
#include "filehdr.h"
#include "syms.h"
#include <string.h>
#include "paths.h"


#define	SKIP	1
#define	IODD	2
#define	OODD	4
#define	HEAD	8

#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000

#define TABSZ	4000

#ifdef PORTAR
#define SYMDIRNAME	"/               "	/* symbol directory filename */
#else
#define SYMDIRNAME	"__.SYMDEF"		/* symbol directory filename */
#endif

struct stat	stbuf;

struct ar_hdr	ariobuf;       /* input/output copy of archive member header */

struct		/* usable copy of archive file member header */
{
	char	ar_name[16];
	long	ar_date;
	int	ar_uid;
	int	ar_gid;
	long	ar_mode;
	long	ar_size;
} arbuf;

long	sym_ptr[TABSZ];		/* offsets for symbol directory */
char	sym_offsets[TABSZ][4];	/* sputl'd version of the offsets */
char	*sym_name[TABSZ];	/* names for symbols in directory */
char	sym_nsyms[4];		/* sputl'd number of symbols */

struct filehdr	ml_filhdr;	/* COFF machine language file header */

char	*man	=	{ "mrxtdpq" };
char	*opt	=	{ "uvcbails" };

int	signum[] = {SIGHUP, SIGINT, SIGQUIT, 0};
char	flg[26];
char	**namv;
int	namc;
char	*arnam;
char	*ponam;
char	*tmpdir = TMPDIR; /* default temp file directory to environment */
char	*tfnam;
char	*tf1nam;
char	*tf2nam;
char	*file;
char	name[16];
int	af;
int	tf;
int	tf1;
int	tf2;
int	qf;

int	bastate;
char	buf[BUFSIZ];


int	update;		/* was archive written or updated */
int	nsyms; 		/* nbr of symbol directory entries */
long	mem_ptr;	/* position of archive member in the archive */
long	mem_skip;	/* adjustment to add to mem_ptr for "real" position */

int	filenum = 0;	/* incremented by ml_file() for getname()'s benefit */

char	*str_base,	/* start of string table for names */
	*str_top;	/* pointer to next available location */


int	m1[] = { 1, ROWN, 'r', '-' };
int	m2[] = { 1, WOWN, 'w', '-' };
int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int	m4[] = { 1, RGRP, 'r', '-' };
int	m5[] = { 1, WGRP, 'w', '-' };
int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int	m7[] = { 1, ROTH, 'r', '-' };
int	m8[] = { 1, WOTH, 'w', '-' };
int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};


	/* declare all archive functions */
	
int	setcom(),	rcmd(),		dcmd(),		xcmd(),
	pcmd(),		mcmd(),		tcmd(),		qcmd(),
	init(),		getaf(),	getqf(),	usage(),
	noar(),		sigdone(),	done(),		notfound(),
	morefil(),	cleanup(),	install(),	movefil(),
	stats(),	copyfil(),	getdir(),	match(),
	bamatch(),	phserr(),	mesg(),		longt(),
	pmode(),	select(),	wrerr(),	mksymtab(),
	ml_file(),	sputl();
char	*trim(),	*trimslash(),	*getname();

char	*tempnam();
char	*ctime();
long	time();

int	(*comfun)();



main(argc, argv)
	int argc;
	char **argv;
{
	register int i;
	register char *cp;

	for (i = 0; signum[i]; i++)
		if (signal(signum[i], SIG_IGN) != SIG_IGN)
			signal(signum[i], sigdone);
	if (argc < 3)
		usage();
	cp = argv[1];
	if (*cp == '-')	/* skip a '-', make ar more like other commands */
		cp++;
	for (; *cp; cp++)
	{
		switch (*cp)
		{
		case 'l':
		case 'v':
		case 'u':
		case 'a':
		case 'b':
		case 'c':
		case 'i':
		case 's':
			flg[*cp - 'a']++;
			continue;
		case 'r':
			setcom(rcmd);
			flg[*cp - 'a']++;
			continue;
		case 'd':
			setcom(dcmd);
			flg[*cp - 'a']++;
			continue;
		case 'x':
			setcom(xcmd);
			flg[*cp - 'a']++;
			continue;
		case 't':
			setcom(tcmd);
			flg[*cp - 'a']++;
			continue;
		case 'p':
			setcom(pcmd);
			flg[*cp - 'a']++;
			continue;
		case 'm':
			setcom(mcmd);
			flg[*cp - 'a']++;
			continue;
		case 'q':
			setcom(qcmd);
			flg[*cp - 'a']++;
			continue;
		default:
			fprintf(stderr, "ar: bad option `%c'\n", *cp);
			done(1);
		}
	}
	if (flg['l' - 'a'])
		tmpdir = ".";	/* use local directory for temp files */
	if (flg['i' - 'a'])
		flg['b' - 'a']++;
	if (flg['a' - 'a'] || flg['b' - 'a'])
	{
		bastate = 1;
		ponam = trim(argv[2]);
		argv++;
		argc--;
		if (argc < 3)
			usage();
	}
	arnam = argv[2];
	namv = argv + 3;
	namc = argc - 3;
	if (comfun == 0)
	{
		if (flg['u' - 'a'] == 0)
		{
			fprintf(stderr,
				"ar: one of [%s] must be specified\n", man);
			done(1);
		}
		setcom(rcmd);
	}
	update = (flg['d' - 'a'] | flg['q' - 'a'] | flg['m' - 'a'] |
		flg['r' - 'a'] | flg['u' - 'a'] | flg['s' - 'a']);
	(*comfun)();
	if (update)	/* make archive symbol table */
		mksymtab();
	done(notfound());
}



setcom(fun)
	int (*fun)();
{

	if (comfun != 0)
	{
		fprintf(stderr, "ar: only one of [%s] allowed\n", man);
		done(1);
	}
	comfun = fun;
}



rcmd()
{
	register int f;

	init();
	getaf();
	while (!getdir())
	{
		bamatch();
		if (namc == 0 || match())
		{
			f = stats();
			if (f < 0)
			{
				if (namc)
					fprintf(stderr,
						"ar: cannot open %s\n", file);
				goto cp;
			}
			if (flg['u' - 'a'])
				if (stbuf.st_mtime <= arbuf.ar_date)
				{
					close(f);
					goto cp;
				}
			mesg('r');
			copyfil(af, -1, IODD + SKIP);
			movefil(f);
			continue;
		}
	cp:
		mesg('c');
		copyfil(af, tf, IODD + OODD + HEAD);
	}
	cleanup();
}




dcmd()
{
	init();
	if (getaf())
		noar();
	while (!getdir())
	{
		if (match())
		{
			mesg('d');
			copyfil(af, -1, IODD + SKIP);
			continue;
		}
		mesg('c');
		copyfil(af, tf, IODD + OODD + HEAD);
	}
	install();
}




xcmd()
{
	register int f;

	if (getaf())
		noar();
	while (!getdir())
	{
		if (namc == 0 || match())
		{
			f = creat(file, arbuf.ar_mode & 0777);
			if (f < 0)
			{
				fprintf(stderr, "ar: %s cannot create\n", file);
				goto sk;
			}
			mesg('x');
			copyfil(af, f, IODD);
			close(f);
			continue;
		}
	sk:
		mesg('c');
		copyfil(af, -1, IODD + SKIP);
		if (namc > 0  &&  !morefil())
			done(0);
	}
}




pcmd()
{
	if (getaf())
		noar();
	while (!getdir())
	{
		if (namc == 0 || match())
		{
			if (flg['v' - 'a'])
			{
				fprintf(stdout, "\n<%s>\n\n", file);
				fflush(stdout);
			}
			copyfil(af, 1, IODD);
			continue;
		}
		copyfil(af, -1, IODD + SKIP);
	}
}




mcmd()
{
	init();
	if (getaf())
		noar();
	tf2nam = tempnam(tmpdir, "ar");
	close(creat(tf2nam, 0600));
	tf2 = open(tf2nam, 2);
	if (tf2 < 0)
	{
		fprintf(stderr, "ar: cannot create third temp\n");
		done(1);
	}
	while (!getdir())
	{
		bamatch();
		if (match())
		{
			mesg('m');
			copyfil(af, tf2, IODD + OODD + HEAD);
			continue;
		}
		mesg('c');
		copyfil(af, tf, IODD + OODD + HEAD);
	}
	install();
}




tcmd()
{
	if (getaf())
		noar();
	while (!getdir())
	{
		if (namc == 0 || match())
		{
			if (flg['v' - 'a'])
				longt();
			fprintf(stdout, "%s\n", trim(file));
		}
		copyfil(af, -1, IODD + SKIP);
	}
}




qcmd()
{
	register int i, f;
	char *tmpfilname;
	int n;
	int mode=0644;

	if (flg['a' - 'a'] || flg['b' - 'a'])
	{
		fprintf(stderr, "ar: abi not allowed with q\n");
		done(1);
	}
	getqf();
	tmpfilname = tempnam(tmpdir, "ar");
	close(creat(tmpfilname, 0600));
	tf = open(tmpfilname, 2);
	if (tf < 0)
	{
		fprintf(stderr, "ar: cannot create temp file\n");
		done(1);
	}
	lseek(qf, 0l, 0);
	while ((n = read(qf, buf, BUFSIZ)) > 0)
		if (write(tf, buf, n) != n)
			wrerr;
	for (i = 0; signum[i]; i++)
		signal(signum[i], SIG_IGN);
	lseek(tf, 0l, 2);
	for (i = 0; i < namc; i++)
	{
		file = namv[i];
		if (file == 0)
			continue;
		namv[i] = 0;
		mesg('q');
		f = stats();
		if (f < 0)
		{
			fprintf(stderr, "ar: %s cannot open\n", file);
			continue;
		}
		movefil(f);
	}
	lseek(tf, 0l, 0);
	lseek(qf, 0l, 0);
	while ((n = read(tf, buf, BUFSIZ)) > 0)
		if (write(qf, buf, n) != n)
			wrerr;
	unlink( tmpfilname );
}




init()
{
	tfnam = tempnam(tmpdir, "ar");
	close(creat(tfnam, 0600));
	tf = open(tfnam, 2);
	if (tf < 0)
	{
		fprintf(stderr, "ar: cannot create temp file\n");
		done(1);
	}
	if (write(tf, ARMAG, sizeof(char) * SARMAG) != sizeof(char) * SARMAG)
		wrerr();
}




getaf()
{
	char buf[SARMAG + 1];
	long home;

	af = open(arnam, 0);
	if (af < 0)
		return (1);
	if (read(af, buf, sizeof(char) * SARMAG) != sizeof(char) * SARMAG ||
		strncmp(buf, ARMAG, SARMAG))
	{
		fprintf(stderr, "ar: %s not in archive format\n", arnam);
		done(1);
	}
	/*
	* If the first entry is the symbol directory, skip it
	*/
	home = lseek(af, 0L, 1);
	if (!getdir() && file[0] == '\0')
	{
		copyfil(af, -1, IODD + SKIP);
		return (0);
	}
	/*
	* Otherwise, get back to beginning of first file header
	*/
	if (lseek(af, home, 0) < 0)
	{
		fprintf(stderr, "ar: %s cannot seek\n", arnam);
		done(1);
	}
	return (0);
}




getqf()
{
	char buf[SARMAG + 1];

	if ((qf = open(arnam, 2)) < 0)		/* a new archive */
	{
		if (!flg['c' - 'a'])
			fprintf(stderr, "ar: creating %s\n", arnam);
		close(creat(arnam, 0666));
		if ((qf = open(arnam, 2)) < 0)
		{
			fprintf(stderr, "ar: cannot create %s\n", arnam);
			done(1);
		}
		if (write(qf, ARMAG, sizeof(char) * SARMAG) !=
			sizeof(char) * SARMAG)
		{
			wrerr();
		}
	}
	else if (read(qf, buf, sizeof(char) * SARMAG) !=
		sizeof(char) * SARMAG || strncmp(buf, ARMAG, SARMAG))
	{
		fprintf(stderr, "ar: %s not in archive format\n", arnam);
		done(1);
	}
	return (0);
}




usage()
{
	fprintf(stderr, "usage: ar [%s][%s] [posname] archive files ...\n",
		man, opt);
	done(1);
}




noar()
{
	fprintf(stderr, "ar: %s does not exist\n", arnam);
	done(1);
}




sigdone()
{
	done(100);
}




done(c)
	int c;
{
	if (tfnam)
		unlink(tfnam);
	if (tf1nam)
		unlink(tf1nam);
	if (tf2nam)
		unlink(tf2nam);
	exit(c);
}




notfound()
{
	register int i, n;

	n = 0;
	for (i = 0; i < namc; i++)
		if (namv[i])
		{
			fprintf(stderr, "ar: %s not found\n", namv[i]);
			n++;
		}
	return (n);
}




morefil()
{
	register int i, n;

	n = 0;
	for (i = 0; i < namc; i++)
		if (namv[i])
			n++;
	return (n);
}




cleanup()
{
	register int i, f;

	for (i = 0; i < namc; i++)
	{
		file = namv[i];
		if (file == 0)
			continue;
		namv[i] = 0;
		mesg('a');
		f = stats();
		if (f < 0)
		{
			fprintf(stderr, "ar: %s cannot open\n", file);
			continue;
		}
		movefil(f);
	}
	install();
}




install()
{
	register int i;

	for (i = 0; signum[i]; i++)
		signal(signum[i], SIG_IGN);
	if (af < 0)
	{
		if (!flg['c' - 'a'])
			fprintf(stderr, "ar: creating %s\n", arnam);
	}
	close(af);
	af = creat(arnam, 0666);
	if (af < 0)
	{
		fprintf(stderr, "ar: cannot create %s\n", arnam);
		done(1);
	}
	if (tfnam)
	{
		lseek(tf, 0l, 0);
		while ((i = read(tf, buf, BUFSIZ)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	if (tf2nam)
	{
		lseek(tf2, 0l, 0);
		while ((i = read(tf2, buf, BUFSIZ)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	if (tf1nam)
	{
		lseek(tf1, 0l, 0);
		while ((i = read(tf1, buf, BUFSIZ)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
}



/*
* insert the file 'file' into the temporary file
*/
movefil(f)
	int f;
{
	(void)strcpy(arbuf.ar_name, trim(file));
	arbuf.ar_size = stbuf.st_size;
	arbuf.ar_date = stbuf.st_mtime;
	arbuf.ar_uid = stbuf.st_uid;
	arbuf.ar_gid = stbuf.st_gid;
	arbuf.ar_mode = stbuf.st_mode;
	copyfil(f, tf, OODD + HEAD);
	close(f);
}



stats()
{
	register int f;

	f = open(file, 0);
	if (f < 0)
		return(f);
	if (fstat(f, &stbuf) < 0)
	{
		close(f);
		return(-1);
	}
	return (f);
}




/*
* copy next file size given in arbuf
*/
copyfil(fin, fout, flag)
	int fin, fout, flag;
{
	register int i, o;
	int pe;
int c;

	if (flag & HEAD)
	{
#ifdef PORTAR
		char buf[sizeof(ariobuf) + 1];

		if (sprintf(buf, "%-16s%-12ld%-6u%-6u%-8o%-10ld%-2s",
			trimslash(file), arbuf.ar_date, arbuf.ar_uid,
			arbuf.ar_gid, arbuf.ar_mode, arbuf.ar_size,
			ARFMAG) != sizeof(ariobuf))
		{
			fprintf(stderr,
				"ar: %s internal header generation error\n",
				arnam);
			done(1);
		}
		(void)strncpy((char *)&ariobuf, buf, sizeof(ariobuf));
#else	/* ! PORTAR */
		strncpy(ariobuf.ar_name, file, sizeof(ariobuf.ar_name));
		ariobuf.ar_date = arbuf.ar_date;
		ariobuf.ar_uid = arbuf.ar_uid;
		ariobuf.ar_gid = arbuf.ar_gid;
		ariobuf.ar_mode = arbuf.ar_mode;
		ariobuf.ar_size = arbuf.ar_size;
#endif	/* PORTAR */
		if (write(fout, &ariobuf, sizeof(ariobuf)) != sizeof(ariobuf))
			wrerr();
	}
	pe = 0;
	while (arbuf.ar_size > 0)
	{
		i = o = BUFSIZ;
		if (arbuf.ar_size < i)
		{
			i = o = arbuf.ar_size;
			if (i & 1)
			{
				buf[i] = '\n';
				if (flag & IODD)
					i++;
				if (flag & OODD)
					o++;
			}
		}
		if (read(fin, buf, i) != i)
			pe++;
		if ((flag & SKIP) == 0)
			if (write(fout, buf, o) != o)
				wrerr();
		arbuf.ar_size -= BUFSIZ;
	}
	if (pe)
		phserr();
}




getdir()
{
	register char *cp;
	register int i;

	i = read(af, (char *)&ariobuf, sizeof(ariobuf));
	if (i != sizeof(ariobuf))
	{
		if (tf1nam)
		{
			i = tf;
			tf = tf1;
			tf1 = i;
		}
		return (1);
	}
#ifdef PORTAR
	if (strncmp(ariobuf.ar_fmag, ARFMAG, sizeof(ariobuf.ar_fmag)))
	{
		fprintf(stderr, "ar: malformed archive (at %ld)\n",
			lseek(af, 0L, 1));
		done(1);
	}
	cp = ariobuf.ar_name + sizeof(ariobuf.ar_name);
	while (*--cp == ' ')
		;
	if (*cp == '/')
		*cp = '\0';
	else
		*++cp = '\0';
#endif	/* PORTAR */
	file = strcpy(name, ariobuf.ar_name);
	(void)strncpy(arbuf.ar_name, name, sizeof(arbuf.ar_name));
#ifdef PORTAR
	if (sscanf(ariobuf.ar_date, "%ld", &arbuf.ar_date) != 1 ||
		sscanf(ariobuf.ar_uid, "%d", &arbuf.ar_uid) != 1 ||
		sscanf(ariobuf.ar_gid, "%d", &arbuf.ar_gid) != 1 ||
		sscanf(ariobuf.ar_mode, "%o", &arbuf.ar_mode) != 1 ||
		sscanf(ariobuf.ar_size, "%ld", &arbuf.ar_size) != 1)
	{
		fprintf(stderr, "ar: %s bad header layout for %s\n",
			arnam, name);
		done(1);
	}
#else	/* ! PORTAR */
	arbuf.ar_date = ariobuf.ar_date;
	arbuf.ar_uid = ariobuf.ar_uid;
	arbuf.ar_gid = ariobuf.ar_gid;
	arbuf.ar_mode = ariobuf.ar_mode;
	arbuf.ar_size = ariobuf.ar_size;
#endif	/* PORTAR */
	return (0);
}




match()
{
	register int i;

	for (i = 0; i < namc; i++)
	{
		if (namv[i] == 0)
			continue;
		if (strcmp(trim(namv[i]), file) == 0)
		{
			file = namv[i];
			namv[i] = 0;
			return (1);
		}
	}
	return (0);
}




bamatch()
{
	register int f;

	switch (bastate)
	{
	case 1:
		if (strcmp(file, ponam) != 0)
			return;
		bastate = 2;
		if (flg['a' - 'a'])
			return;
	case 2:
		bastate = 0;
		tf1nam = tempnam(tmpdir, "ar");
		close(creat(tf1nam, 0600));
		f = open(tf1nam, 2);
		if (f < 0)
		{
			fprintf(stderr, "ar: cannot create second temp\n");
			return;
		}
		tf1 = tf;
		tf = f;
	}
}




phserr()
{
	fprintf(stderr, "ar: phase error on %s\n", file);
	done(1);
}




mesg(c)
	int c;
{
	if (flg['v' - 'a'])
		if (c != 'c' || flg['v' - 'a'] > 1)
			fprintf(stdout, "%c - %s\n", c, file);
}




char *
trimslash(s)
	char *s;
{
	static char buf[sizeof(arbuf.ar_name)];

	(void)strncpy(buf, trim(s), sizeof(arbuf.ar_name) - 2);
	buf[sizeof(arbuf.ar_name) - 2] = '\0';
	return (strcat(buf, "/"));
}


char *
trim(s)
	char *s;
{
	register char *p1, *p2;

	for (p1 = s; *p1; p1++)
		;
	while (p1 > s)
	{
		if (*--p1 != '/')
			break;
		*p1 = 0;
	}
	p2 = s;
	for (p1 = s; *p1; p1++)
		if (*p1 == '/')
			p2 = p1 + 1;
	return (p2);
}




longt()
{
	register char *cp;

	pmode();
	fprintf(stdout, "%6d/%6d", arbuf.ar_uid, arbuf.ar_gid);
	fprintf(stdout, "%7ld", arbuf.ar_size);
	cp = ctime(&arbuf.ar_date);
	fprintf(stdout, " %-12.12s %-4.4s ", cp + 4, cp + 20);
}




pmode()
{
	register int **mp;

	for (mp = &m[0]; mp < &m[9];)
		select(*mp++);
}




select(pairp)
	int *pairp;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n >= 0 && (arbuf.ar_mode & *ap++) == 0)
		ap++;
	putchar(*ap);
}




wrerr()
{
	perror("ar write error");
	done(1);
}




mksymtab()
{
	int i, j, offset;
	struct syment symbol;	/* usable copy of a symbol entry */
#ifdef PORTAR
	char buf[sizeof(ariobuf) + 1];
#else
	struct ar_hdr buf;
#endif

	if (getaf())
	{
		fprintf(stderr, "ar: cannot make symbol directory\n");
		done(1);
	}
	nsyms = 0;
	mem_ptr = sizeof(char) * SARMAG;
	mem_skip = lseek(af, 0L, 1) - mem_ptr;
	while (!getdir())
	{
		/*
		* scan machine language file members for symbols
		*/
		if (i = ml_file())	/* read the symbol table */
		{
			while (i > 0)
			{
				i--;
				if (read(af, (char *)&symbol, SYMESZ) != SYMESZ)
				{
					fprintf(stderr, "ar: internal error, archive %s out of order!\n",
						arnam);
					done(1);
				}
				/*
				* check out symbol, is it
				* text, data, bss or common
				*/
				if (((symbol.n_scnum > 0 || symbol.n_scnum == N_ABS)
					&& symbol.n_sclass == C_EXT) ||
					(symbol.n_scnum == 0 &&	/* common */
					symbol.n_sclass == C_EXT &&
					symbol.n_value != 0))
				{
					sym_ptr[nsyms] = mem_ptr;
					sym_name[nsyms] = getname(&symbol);
					if (++nsyms > TABSZ)	/* too many! */
					{
						fprintf(stderr, "ar: too many external symbols\n");
						done(1);
					}
				}
				/*
				* skip over any auxilliary entries
				*/
				j = symbol.n_numaux;
				while (j)
				{
					j--;
					i--;
					if (read(af, (char *)&symbol, SYMESZ)
						!= SYMESZ)
					{
						fprintf(stderr, "ar: internal error, archive %s out of order!\n",
							arnam);
						done(1);
					}
				}
			}
		}
		/*
		* Be careful with odd length .o files (now possible)
		*/
		mem_ptr += ((arbuf.ar_size + 1) & ~01) + sizeof(struct ar_hdr);
		/*
		* Skip string table (if any)
		*/
		if (ISCOFF(ml_filhdr.f_magic)) {
			if (lseek(af, mem_ptr + mem_skip, 0) < 0)
			{
				fprintf(stderr,
				    "ar: %s cannot skip string table for %s\n",
				    arnam, file);
				done(1);
			}
		}
	}
	/*
	* rewrite the archive to include the symbol directory
	*/
	close(af);
	/*
	* clean up the garbage that may have been left around
	*/
	unlink(tfnam);
	tfnam = 0;
	unlink(tf1nam);
	tf1nam = 0;
	unlink(tf2nam);
	tf2nam = 0;
	init();		/* rewrite the archive header */
	/*
	* patch up archive pointers and write the symbol entries
	*/
	if ((str_top - str_base) & 01)	/* round up string table */
		*str_top++ = '\0';
	offset = (nsyms + 1) * sizeof(sym_offsets[0]) +
		sizeof(char) * (str_top - str_base) + sizeof(struct ar_hdr);
	for (i = 0; i < nsyms; i++)
	{
		sym_ptr[i] += offset;
		sputl(sym_ptr[i], sym_offsets[i]);
	}
	if (nsyms > 0)
	{
		sputl(nsyms, sym_nsyms);
#ifdef PORTAR
		if (sprintf(buf, "%-16s%-12ld%-6u%-6u%-8o%-10ld%-2s",
			SYMDIRNAME, time(0), 0, 0, 0,
			(long)(offset - sizeof(struct ar_hdr)), ARFMAG)
			!= sizeof(ariobuf))
		{
			fprintf(stderr,
				"ar: %s internal header generation error\n",
				arnam);
			done(1);
		}
#else	/* ! PORTAR */
		strncpy(buf.ar_name,  SYMDIRNAME, sizeof(buf.ar_name));
		buf.ar_date = time(0);
		buf.ar_uid = buf.ar_gid = buf.ar_mode = 0;
		buf.ar_size = (long)(offset-sizeof(struct ar_hdr));
#endif	/* PORTAR */
		if (write(tf, buf, sizeof(ariobuf)) != sizeof(ariobuf) ||
			write(tf, sym_nsyms, sizeof(sym_nsyms))
			!= sizeof(sym_nsyms) || write(tf, (char *)sym_offsets,
			nsyms * sizeof(sym_offsets[0]))
			!= nsyms * sizeof(sym_offsets[0]) ||
			write(tf, str_base, sizeof(char) * (str_top - str_base))
			!= sizeof(char) * (str_top - str_base))
		{
			wrerr();
		}
	}
	/*
	* copy the rest of the archive to finish up
	*/
	getaf();		/* skip past the old header */
	while (!getdir())
		copyfil(af, tf, IODD + OODD + HEAD);
	install();
}




ml_file()
{
	long save_size;

	/*
	* is this a recognizable machine language file
	* if so, then skip down to the beginning of the symbol table
	* function returns the number of symbol table entries
	* if not, then skip to the next file
	*/
	filenum++;
	if (arbuf.ar_size <= sizeof(ml_filhdr))		/* don't bother */
	{
		save_size = arbuf.ar_size;
		copyfil(af, -1, IODD + SKIP);
		arbuf.ar_size = save_size;
		return (0);
	}
	/*
	* check the "magic" number
	*/
	if (read(af, (char *)&ml_filhdr, sizeof(ml_filhdr))
		!= sizeof(ml_filhdr))
	{
		fprintf(stderr,
			"ar: internal error, archive %s out of order!\n",arnam);
		done(1);
	}
	if (ISCOFF( ml_filhdr.f_magic ))
	{
		/*
		* skip to the beginning of the symbol table and return the
		* number of symbols in the table
		*/
		save_size = arbuf.ar_size;
		if (ml_filhdr.f_symptr < 0 || ml_filhdr.f_symptr > save_size)
		{
#if unix && (u3b || u3b5 || u3b2 || u3b15)
			/*
			* Check for special case of sccs file.  This type
			* of file looks like an object file on the 3b family.
			* The special numbers are the strings '^As 0' through
			* '^As 9' which are what can be found in an sccs file
			* on these machines.  If the object file header is
			* changed, or the structure of sccs files is modified,
			* this code will no longer be valid.
			*/
			if (ml_filhdr.f_symptr >= 0x01732030 &&	/* in ascii */
				ml_filhdr.f_symptr <= 0x01732039)
			{
				goto skip_it;	/* don't bother with warning */
			}
#endif
			fprintf(stderr, "ar: (warning) file %.16s pretends to be an object file\n",
				arbuf.ar_name);
			goto skip_it;
		}
		arbuf.ar_size = ml_filhdr.f_symptr - sizeof(ml_filhdr);
		copyfil(af, -1, SKIP);
		arbuf.ar_size = save_size;
		return (ml_filhdr.f_nsyms);	/* return the nbr of entries */
	}
	else if (ml_filhdr.f_magic == 0407 || ml_filhdr.f_magic == 0410)
	{
		/* beware of old format .o files (pre-5.0) */
		fprintf(stderr, "ar: (warning) file %.16s in pre 5.0 format\n",
			arbuf.ar_name);
	}

	skip_it:;	 /* skip to the next archive file member */
		save_size = arbuf.ar_size;
		arbuf.ar_size -= sizeof(ml_filhdr);
		copyfil(af, -1, IODD + SKIP);
		arbuf.ar_size = save_size;
		return (0);
}


char *
getname(symbol)
	struct syment *symbol;
{
	extern char	*malloc();
	extern char	*realloc();
	static int str_length = BUFSIZ * 5;
#ifdef FLEXNAMES
	static int lastfilenum = 0;
	static char *strtab = NULL;
	static long strtablen = 0;
#endif

	if (str_base == (char *)0)	/* no space allocated yet */
	{
		if ((str_base = malloc((unsigned)str_length)) == NULL)
		{
			fprintf(stderr,
				"ar: %s cannot get string table space\n",
				arnam);
			done(1);
		}
		str_top = str_base;
	}
#ifdef FLEXNAMES
	if (symbol->n_zeroes == 0)	/* symbol is in string table */
	{
		char *p;
		register int i;

		if (lastfilenum != filenum || strtab == NULL)	/* read it in */
		{
			union
			{
				long l;
				char c[sizeof(long) / sizeof(char)];
			} u;
			long home = lseek(af, 0L, 1);
			long len;

			if (strtab != NULL)
				free(strtab);
			len = ml_filhdr.f_symptr + ml_filhdr.f_nsyms * SYMESZ;
			len += sizeof(struct ar_hdr) + mem_ptr + mem_skip;
			/* seek to begin of string table */
			if ( lseek(af, len, 0) < 0 )
			{   	fprintf(stderr,
					"ar: error on string table seek\n");
				done(1);
			}
			/* read ACTUAL length of string table */
			if ( read(af, u.c, sizeof(long)) != sizeof(long))
			{   	fprintf(stderr,
					"ar: error on string table read\n");
				done(1);
			}
			if ((strtablen = u.l) < 4L)/* room for string table */
			{	fprintf(stderr,
					"ar: %s missing string table for %s\n",
					arnam, file);
				done(1);
			}
			if ((strtab = malloc((unsigned)strtablen)) == NULL ||
				lseek(af, len, 0) < 0 ||
				read(af, strtab, strtablen) != strtablen ||
				lseek(af, home, 0) < 0 ||
				strtab[strtablen - 1] != '\0')
			{
				fprintf(stderr,
					"ar: %s bad strings table for %s\n",
					arnam, file);
				done(1);
			}
			lastfilenum = filenum;
		}
		if (symbol->n_offset < 4 || symbol->n_offset >= strtablen)
		{
			fprintf(stderr,
				"ar: %s bad string table offset for %s\n",
				arnam, file);
			done(1);
		}
		p = str_top;
		str_top += strlen(strtab + symbol->n_offset) + 1;
		if (str_top > str_base + str_length)
		{
			char *old_base = str_base;
			int diff;

			str_length += BUFSIZ * 2;
			if ((str_base = realloc(str_base, str_length)) == NULL)
			{
				fprintf(stderr,
					"ar: %s cannot grow string table\n",
					arnam);
				done(1);
			}
			/*
			* Re-adjust other pointers
			*/
			diff = str_base - old_base;
			p += diff;
			str_top += diff;
		}
		(void)strcpy(p, strtab + symbol->n_offset);
		return (p);
	}
	else	/* old COFF version - copy into string table */
#endif
	{
		register char *p, *s;
		register int i;

		if (str_top + SYMNMLEN >= str_base + str_length)
		{
			char *old_base = str_base;
			int diff;

			str_length += BUFSIZ * 2;
			if ((str_base = realloc(str_base, str_length)) == NULL)
			{
				fprintf(stderr,
					"ar: %s cannot grow string table\n",
					arnam);
				done(1);
			}
			/*
			* Re-adjust other pointers
			*/
			diff = str_base - old_base;
			p += diff;
			str_top += diff;
		}
		for (i = 0, p = str_top, s = symbol->n_name;
			i < SYMNMLEN && *s != '\0'; i++)
		{
			*p++ = *s++;
		}
		*p++ = '\0';
		s = str_top;
		str_top = p;
		return (s);
	}
}


/*
 * The intent here is to provide a means to make the value of
 * bytes in an io-stream correspond to the value of the long
 * in the memory while doing the io a `long' at a time.
 * Files written and read in this way are machine-independent.
 *
 */
int
sputl(w, buffer)
register long w;
register char *buffer;
{
        register int i = BITSPERBYTE * sizeof(long);

        while ((i -= BITSPERBYTE) >= 0)
                *buffer++ = (char) (w >> i);
}
/* End Enhanced Application Compatibility Support */
