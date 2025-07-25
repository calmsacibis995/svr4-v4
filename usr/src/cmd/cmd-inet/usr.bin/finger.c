/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:usr.bin/finger.c	1.8.8.1"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 * 	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


/*
 * This is a finger program.  It prints out useful information about users
 * by digging it up from various system files.
 *
 * There are three output formats, all of which give login name, teletype
 * line number, and login time.  The short output format is reminiscent
 * of finger on ITS, and gives one line of information per user containing
 * in addition to the minimum basic requirements (MBR), the user's full name,
 * idle time and location.
 * The quick style output is UNIX who-like, giving only name, teletype and
 * login time.  Finally, the long style output give the same information
 * as the short (in more legible format), the home directory and shell
 * of the user, and, if it exits, a copy of the file .plan in the users
 * home directory.  Finger may be called with or without a list of people
 * to finger -- if no list is given, all the people currently logged in
 * are fingered.
 *
 * The program is validly called by one of the following:
 *
 *	finger			{short form list of users}
 *	finger -l		{long form list of users}
 *	finger -b		{briefer long form list of users}
 *	finger -q		{quick list of users}
 *	finger -i		{quick list of users with idle times}
 *	finger namelist		{long format list of specified users}
 *	finger -s namelist	{short format list of specified users}
 *	finger -w namelist	{narrow short format list of specified users}
 *
 * where 'namelist' is a list of users login names.
 * The other options can all be given after one '-', or each can have its
 * own '-'.  The -f option disables the printing of headers for short and
 * quick outputs.  The -b option briefens long format outputs.  The -p
 * option turns off plans for long format outputs.
 */

#include <sys/types.h>
#include <sys/stat.h>
#ifdef SYSV
#include <utmpx.h>
#else
#include <utmp.h>
#endif /* SYSV */
#include <sys/signal.h>
#include <pwd.h>
#include <stdio.h>
#include <lastlog.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#ifndef SYSV
#include <ttyent.h>

#else

#define	rindex	strrchr
#define	bcopy(a,b,c)	memcpy(b,a,c)
#endif /* SYSV */

#define ASTERISK	'*'		/* ignore this in real name */
#define COMMA		','		/* separator in pw_gecos field */
#define COMMAND		'-'		/* command line flag char */
#define SAMENAME	'&'		/* repeat login name in real name */
#define TALKABLE	0220		/* tty is writable if this mode */

#ifdef SYSV
struct utmpx user;
#else
struct utmp user;
#endif /* SYSV */
#define NMAX sizeof(user.ut_name)
#define LMAX sizeof(user.ut_line)
#define HMAX sizeof(user.ut_host)

struct person {			/* one for each person fingered */
	char *name;			/* name */
	char tty[LMAX+1];		/* null terminated tty line */
	char host[HMAX+1];		/* null terminated remote host name */
	char *ttyloc;			/* location of tty line, if any */
	long loginat;			/* time of (last) login */
	long idletime;			/* how long idle (if logged in) */
	char *realname;			/* pointer to full name */
	struct passwd *pwd;		/* structure of /etc/passwd stuff */
	char loggedin;			/* person is logged in */
	char writable;			/* tty is writable */
	char original;			/* this is not a duplicate entry */
	struct person *link;		/* link to next person */
};

char LASTLOG[] = "/var/adm/lastlog";	/* last login info */
#ifdef SYSV
char USERLOG[] = "/etc/utmpx";		/* who is logged in */
#else
char USERLOG[] = "/etc/utmp";		/* who is logged in */
#endif /* SYSV */
char PLAN[] = "/.plan";			/* what plan file is */
char PROJ[] = "/.project";		/* what project file */
	
int unbrief = 1;			/* -b option default */
int header = 1;				/* -f option default */
int hack = 1;				/* -h option default */
int idle = 0;				/* -i option default */
int large = 0;				/* -l option default */
int match = 1;				/* -m option default */
int plan = 1;				/* -p option default */
int unquick = 1;			/* -q option default */
int small = 0;				/* -s option default */
int wide = 1;				/* -w option default */

int unshort;
int lf;					/* LASTLOG file descriptor */
struct person *person1;			/* list of people */
long tloc;				/* current time */

struct passwd *pwdcopy();
char *strcpy();
char *malloc();
char *ctime();

main(argc, argv)
	int argc;
	register char **argv;
{
	FILE *fp;
	register char *s;

	/* parse command line for (optional) arguments */
	while (*++argv && **argv == COMMAND)
		for (s = *argv + 1; *s; s++)
			switch (*s) {
			case 'b':
				unbrief = 0;
				break;
			case 'f':
				header = 0;
				break;
			case 'h':
				hack = 0;
				break;
			case 'i':
				idle = 1;
				unquick = 0;
				break;
			case 'l':
				large = 1;
				break;
			case 'm':
				match = 0;
				break;
			case 'p':
				plan = 0;
				break;
			case 'q':
				unquick = 0;
				break;
			case 's':
				small = 1;
				break;
			case 'w':
				wide = 0;
				break;
			default:
				fprintf(stderr, "Usage: finger [-bfhilmpqsw] [login1 [login2 ...] ]\n");
				exit(1);
			}
	if (unquick || idle)
		time(&tloc);
	/*
	 * *argv == 0 means no names given
	 */
	if (*argv == 0)
		doall();
	else
		donames(argv);
	if (person1)
		print();
	exit(0);
	/* NOTREACHED */
}

doall()
{
	register struct person *p;
	register struct passwd *pw;
	int uf;
	char name[NMAX + 1];

	unshort = large;
	if ((uf = open(USERLOG, 0)) < 0) {
		fprintf(stderr, "finger: error opening %s\n", USERLOG);
		exit(2);
	}
	if (unquick) {
		setpwent();
		fwopen();
	}
	while (read(uf, (char *)&user, sizeof user) == sizeof user) {
		if (user.ut_name[0] == 0 
#ifndef SYSV
		    || nonuser(user)
#endif /* !SYSV */
		    || user.ut_type != USER_PROCESS)
			continue;
		if (person1 == 0)
			p = person1 = (struct person *) malloc(sizeof *p);
		else {
			p->link = (struct person *) malloc(sizeof *p);
			p = p->link;
		}
		bcopy(user.ut_name, name, NMAX);
		name[NMAX] = 0;
		bcopy(user.ut_line, p->tty, LMAX);
		p->tty[LMAX] = 0;
		bcopy(user.ut_host, p->host, HMAX);
		p->host[HMAX] = 0;
#ifdef SYSV
		p->loginat = user.ut_tv.tv_sec;
#else
		p->loginat = user.ut_time;
#endif /* SYSV */
		p->pwd = 0;
		p->loggedin = 1;
		if (unquick && (pw = getpwnam(name))) {
			p->pwd = pwdcopy(pw);
			decode(p);
			p->name = p->pwd->pw_name;
		} else
			p->name = strcpy(malloc(strlen(name) + 1), name);
		p->ttyloc = NULL;
#ifndef SYSV
		if (unquick) getloc(p);
#endif /* !SYSV */
	}
	if (unquick) {
		fwclose();
		endpwent();
	}
	close(uf);
	if (person1 == 0) {
		printf("No one logged on\n");
		return;
	}
	p->link = 0;
}

#ifndef SYSV
getloc(p)
	register struct person *p;
{
    register int times;
    register struct ttyent *ttyentp;

	/*
	 * We assume here that 1) "gettynam" does not use
	 * an index and 2) the order of entries in the "utmp[x]"
	 * file is the same as the order of entries in the
	 * "ttytab" file, so scanning through them in
	 * parallel is the most efficient way to work.
	 */
	for (times = 0; times < 2; times++) {
		while ((ttyentp = getttyent()) != NULL) {
			if (strcmp(p->tty, ttyentp->ty_name) == 0)
				goto found;
		}
		setttyent();
	}
	return;
found:
	if (ttyentp->ty_comment != NULL) {
		p->ttyloc = malloc(strlen(ttyentp->ty_comment)+1);
		if (p->ttyloc != NULL)
			strcpy(p->ttyloc, ttyentp->ty_comment);
	}
}
#endif /* !SYSV */


donames(argv)
	char **argv;
{
	register struct person *p;
	register struct passwd *pw;
	int uf, count;

	/*
	 * get names from command line and check to see if they're
	 * logged in
	 */
	unshort = !small;
	count = 0;
	for (; *argv != 0; argv++) {
		if (netfinger(*argv))
			continue;
		count++;
		if (person1 == 0)
			p = person1 = (struct person *) malloc(sizeof *p);
		else {
			p->link = (struct person *) malloc(sizeof *p);
			p = p->link;
		}
		p->name = *argv;
		p->loggedin = 0;
		p->original = 1;
		p->pwd = 0;
	}
	if (person1 == 0)
		return;
	p->link = 0;
	/*
	 * if we are doing it, read /etc/passwd for the useful info
	 */
	if (unquick) {
		setpwent();
		if (!match) {
			for (p = person1; p != 0; p = p->link)
				if (pw = getpwnam(p->name))
					p->pwd = pwdcopy(pw);
		} else while ((pw = getpwent()) != 0 && count) {
			for (p = person1; p != 0; p = p->link) {
				if (!p->original)
					continue;
				if (strcmp(p->name, pw->pw_name) != 0 &&
				    !matchcmp(pw->pw_gecos, pw->pw_name, p->name))
					continue;
				if (p->pwd == 0) {
					p->pwd = pwdcopy(pw);
					if (strcmp(p->name, pw->pw_name) == 0)
						count--;
				}
				else {
					struct person *new;
					/*
					 * handle multiple login names, insert
					 * new "duplicate" entry behind
					 */
					new = (struct person *)
						malloc(sizeof *new);
					new->pwd = pwdcopy(pw);
					new->name = p->name;
					new->original = 1;
					new->loggedin = 0;
					new->link = p->link;
					p->original = 0;
					p->link = new;
					p = new;
				}
			}
		}
		endpwent();
	}
	/* Now get login information */
	if ((uf = open(USERLOG, 0)) < 0) {
		fprintf(stderr, "finger: error opening %s\n", USERLOG);
		exit(2);
	}
	while (read(uf, (char *)&user, sizeof user) == sizeof user) {
		if (*user.ut_name == 0)
			continue;
		for (p = person1; p != 0; p = p->link) {
			p->ttyloc = NULL;
			if (p->loggedin == 2)
				continue;
			if (strncmp(p->pwd ? p->pwd->pw_name : p->name,
				    user.ut_name, NMAX) != 0)
				continue;
			if (p->loggedin == 0) {
				bcopy(user.ut_line, p->tty, LMAX);
				p->tty[LMAX] = 0;
				bcopy(user.ut_host, p->host, HMAX);
				p->host[HMAX] = 0;
#ifdef SYSV
				p->loginat = user.ut_tv.tv_sec;
#else
				getloc(p);
				p->loginat = user.ut_time;
#endif /* SYSV */
				p->loggedin = 1;
			} else {	/* p->loggedin == 1 */
				struct person *new;
				new = (struct person *) malloc(sizeof *new);
				new->name = p->name;
				bcopy(user.ut_line, new->tty, LMAX);
				new->tty[LMAX] = 0;
				bcopy(user.ut_host, new->host, HMAX);
				new->host[HMAX] = 0;
#ifdef SYSV
				new->loginat = user.ut_tv.tv_sec;
#else
				new->loginat = user.ut_time;
#endif /* SYSV */
				new->pwd = p->pwd;
				new->loggedin = 1;
				new->original = 0;
				new->link = p->link;
				p->loggedin = 2;
				p->link = new;
				p = new;
			}
		}
	}
	close(uf);
	if (unquick) {
		fwopen();
		for (p = person1; p != 0; p = p->link)
			decode(p);
		fwclose();
	}
}

print()
{
	register FILE *fp;
	register struct person *p;
	register char *s;
	register c;

	/*
	 * print out what we got
	 */
	if (header) {
		if (unquick) {
			if (!unshort)
				if (wide)
					printf("Login       Name               TTY          Idle    When    Where\n");
				else
					printf("Login    TTY Idle    When    Where\n");
		} else {
			printf("Login      TTY                When");
			if (idle)
				printf("             Idle");
			putchar('\n');
		}
	}
	for (p = person1; p != 0; p = p->link) {
		if (!unquick) {
			quickprint(p);
			continue;
		}
		if (!unshort) {
			shortprint(p);
			continue;
		}
		personprint(p);
		if (p->pwd != 0 && !AlreadyPrinted(p->pwd->pw_uid)) {
			AnyMail(p->pwd->pw_name);
			if (hack) {
				s = malloc(strlen(p->pwd->pw_dir) +
					sizeof PROJ);
				strcpy(s, p->pwd->pw_dir);
				strcat(s, PROJ);
				if ((fp = fopen(s, "r")) != 0) {
					printf("Project: ");
					while ((c = getc(fp)) != EOF) {
						if (c == '\n')
							break;
						if (isprint(c) || isspace(c))
							putchar(c);
						else
							putchar(c ^ 100);
					}
					fclose(fp);
					putchar('\n');
				}
				free(s);
			}
			if (plan) {
				s = malloc(strlen(p->pwd->pw_dir) +
					sizeof PLAN);
				strcpy(s, p->pwd->pw_dir);
				strcat(s, PLAN);
				if ((fp = fopen(s, "r")) == 0)
					printf("No Plan.\n");
				else {
					printf("Plan:\n");
					while ((c = getc(fp)) != EOF)
						if (isprint(c) || isspace(c))
							putchar(c);
						else
							putchar(c ^ 100);
					fclose(fp);
				}
				free(s);
			}
		}
		if (p->link != 0)
			putchar('\n');
	}
}

/*
 * Duplicate a pwd entry.
 * Note: Only the useful things (what the program currently uses) are copied.
 */
struct passwd *
pwdcopy(pfrom)
	register struct passwd *pfrom;
{
	register struct passwd *pto;

	pto = (struct passwd *) malloc(sizeof *pto);
#define savestr(s) strcpy(malloc(strlen(s) + 1), s)
	pto->pw_name = savestr(pfrom->pw_name);
	pto->pw_uid = pfrom->pw_uid;
	pto->pw_gecos = savestr(pfrom->pw_gecos);
	pto->pw_dir = savestr(pfrom->pw_dir);
	pto->pw_shell = savestr(pfrom->pw_shell);
#undef savestr
	return pto;
}

/*
 * print out information on quick format giving just name, tty, login time
 * and idle time if idle is set.
 */
quickprint(pers)
	register struct person *pers;
{
	printf("%-8.8s  ", pers->name);
	if (pers->loggedin) {
		if (idle) {
			findidle(pers);
			printf("%c%-12s %-16.16s", pers->writable ? ' ' : '*',
				pers->tty, ctime(&pers->loginat));
			ltimeprint("   ", &pers->idletime, "");
		} else
			printf(" %-12s %-16.16s",
				pers->tty, ctime(&pers->loginat));
		putchar('\n');
	} else
		printf("          Not Logged In\n");
}

/*
 * print out information in short format, giving login name, full name,
 * tty, idle time, login time, and host.
 */
shortprint(pers)
	register struct person *pers;
{
	char *p;

	if (pers->pwd == 0) {
		printf("%-15s       ???\n", pers->name);
		return;
	}
	printf("%-8s", pers->pwd->pw_name);
	if (wide) {
		if (pers->realname)
			printf(" %-20.20s", pers->realname);
		else
			printf("        ???          ");
	}
	putchar(' ');
	if (pers->loggedin && !pers->writable)
		putchar('*');
	else
		putchar(' ');
	if (*pers->tty) {
		printf("%-12.12s ", pers->tty);
	} else
		printf("             ");  /* 13 spaces */
	p = ctime(&pers->loginat);
	if (pers->loggedin) {
		stimeprint(&pers->idletime);
		printf(" %3.3s %-5.5s ", p, p + 11);
	} else if (pers->loginat == 0)
		printf(" < .  .  .  . >");
	else if (tloc - pers->loginat >= 180 * 24 * 60 * 60)
		printf(" <%-6.6s, %-4.4s>", p + 4, p + 20);
	else
		printf(" <%-12.12s>", p + 4);
	if (*pers->host)
		printf(" %-20.20s", pers->host);
	else {
		if (pers->ttyloc != NULL)
			printf(" %-20.20s", pers->ttyloc);
	}
	putchar('\n');
}


/*
 * print out a person in long format giving all possible information.
 * directory and shell are inhibited if unbrief is clear.
 */
personprint(pers)
	register struct person *pers;
{
	if (pers->pwd == 0) {
		printf("Login name: %-10s\t\t\tIn real life: ???\n",
			pers->name);
		return;
	}
	printf("Login name: %-10s", pers->pwd->pw_name);
	if (pers->loggedin && !pers->writable)
		printf("	(messages off)	");
	else
		printf("			");
	if (pers->realname)
		printf("In real life: %s", pers->realname);
	if (unbrief) {
		printf("\nDirectory: %-25s", pers->pwd->pw_dir);
		if (*pers->pwd->pw_shell)
			printf("\tShell: %-s", pers->pwd->pw_shell);
	}
	if (pers->loggedin) {
		register char *ep = ctime(&pers->loginat);
		if (*pers->host) {
			printf("\nOn since %15.15s on %-12s from %s",
				&ep[4], pers->tty, pers->host);
			ltimeprint("\n", &pers->idletime, " Idle Time");
		} else {
			printf("\nOn since %15.15s on %-12s",
				&ep[4], pers->tty);
			ltimeprint("\n", &pers->idletime, " Idle Time");
		}
	} else if (pers->loginat == 0)
		printf("\nNever logged in.");
	else if (tloc - pers->loginat > 180 * 24 * 60 * 60) {
		register char *ep = ctime(&pers->loginat);
		printf("\nLast login %10.10s, %4.4s on %s",
			ep, ep+20, pers->tty);
		if (*pers->host)
			printf(" from %s", pers->host);
	} else {
		register char *ep = ctime(&pers->loginat);
		printf("\nLast login %16.16s on %s", ep, pers->tty);
		if (*pers->host)
			printf(" from %s", pers->host);
	}
	putchar('\n');
}


/*
 * decode the information in the gecos field of /etc/passwd
 */
decode(pers)
	register struct person *pers;
{
	char buffer[256];
	register char *bp, *gp, *lp;
	int alldigits;
	int hasspace;
	int len;

	pers->realname = 0;
	if (pers->pwd == 0)
		return;
	gp = pers->pwd->pw_gecos;
	bp = buffer;
	if (*gp == ASTERISK)
		gp++;
	while (*gp && *gp != COMMA) 			/* name */
		if (*gp == SAMENAME) {
			lp = pers->pwd->pw_name;
			if (islower(*lp))
				*bp++ = toupper(*lp++);
			while (*bp++ = *lp++)
				;
			bp--;
			gp++;
		} else
			*bp++ = *gp++;
	*bp++ = 0;
	if ((len = bp - buffer) > 1)
		pers->realname = strcpy(malloc(len), buffer);
	if (pers->loggedin)
		findidle(pers);
	else
		findwhen(pers);
}

/*
 * find the last log in of a user by checking the LASTLOG file.
 * the entry is indexed by the uid, so this can only be done if
 * the uid is known (which it isn't in quick mode)
 */

fwopen()
{
	if ((lf = open(LASTLOG, 0)) < 0)
		fprintf(stderr, "finger: %s open error\n", LASTLOG);
}

findwhen(pers)
	register struct person *pers;
{
	struct lastlog ll;
	int i;

	if (lf >= 0) {
		lseek(lf, (long)pers->pwd->pw_uid * sizeof ll, 0);
		if ((i = read(lf, (char *)&ll, sizeof ll)) == sizeof ll) {
			bcopy(ll.ll_line, pers->tty, LMAX);
			pers->tty[LMAX] = 0;
			bcopy(ll.ll_host, pers->host, HMAX);
			pers->host[HMAX] = 0;
			pers->loginat = ll.ll_time;
		} else {
			if (i != 0)
				fprintf(stderr, "finger: %s read error\n",
					LASTLOG);
			pers->tty[0] = 0;
			pers->host[0] = 0;
			pers->loginat = 0L;
		}
	} else {
		pers->tty[0] = 0;
		pers->host[0] = 0;
		pers->loginat = 0L;
	}
}

fwclose()
{
	if (lf >= 0)
		close(lf);
}

/*
 * find the idle time of a user by doing a stat on /dev/tty??,
 * where tty?? has been gotten from USERLOG, supposedly.
 */
findidle(pers)
	register struct person *pers;
{
	struct stat ttystatus;
#ifdef sun
	struct stat inputdevstatus;
#endif
	static char buffer[20] = "/dev/";
	long t;
	long lastinputtime;
#define TTYLEN 5

	strcpy(buffer + TTYLEN, pers->tty);
	buffer[TTYLEN+LMAX] = 0;
	if (stat(buffer, &ttystatus) < 0) {
		fprintf(stderr, "finger: Can't stat %s\n", buffer);
		exit(4);
	}
	lastinputtime = ttystatus.st_atime;
#ifdef sun
	if (strcmp(pers->tty, "console") == 0) {
		/*
		 * On the console, the user may be running a window system; if
		 * so, their activity will show up in the last-access times of
		 * "/dev/kbd" and "/dev/mouse", so take the minimum of the idle
		 * times on those two devices and "/dev/console" and treat that
		 * as the idle time.
		 */
		if (stat("/dev/kbd", &inputdevstatus) == 0) {
			if (lastinputtime < inputdevstatus.st_atime)
				lastinputtime = inputdevstatus.st_atime;
		}
		if (stat("/dev/mouse", &inputdevstatus) == 0) {
			if (lastinputtime < inputdevstatus.st_atime)
				lastinputtime = inputdevstatus.st_atime;
		}
	}
#endif
	time(&t);
	if (t < lastinputtime)
		pers->idletime = 0L;
	else
		pers->idletime = t - lastinputtime;
	pers->writable = (ttystatus.st_mode & TALKABLE) == TALKABLE;
}

/*
 * print idle time in short format; this program always prints 4 characters;
 * if the idle time is zero, it prints 4 blanks.
 */
stimeprint(dt)
	long *dt;
{
	register struct tm *delta;

	delta = gmtime(dt);
	if (delta->tm_yday == 0)
		if (delta->tm_hour == 0)
			if (delta->tm_min == 0)
				printf("    ");
			else
				printf("  %2d", delta->tm_min);
		else
			if (delta->tm_hour >= 10)
				printf("%3d:", delta->tm_hour);
			else
				printf("%1d:%02d",
					delta->tm_hour, delta->tm_min);
	else
		printf("%3dd", delta->tm_yday);
}

/*
 * print idle time in long format with care being taken not to pluralize
 * 1 minutes or 1 hours or 1 days.
 * print "prefix" first.
 */
ltimeprint(before, dt, after)
	long *dt;
	char *before, *after;
{
	register struct tm *delta;

	delta = gmtime(dt);
	if (delta->tm_yday == 0 && delta->tm_hour == 0 && delta->tm_min == 0 &&
	    delta->tm_sec <= 10)
		return (0);
	printf("%s", before);
	if (delta->tm_yday >= 10)
		printf("%d days", delta->tm_yday);
	else if (delta->tm_yday > 0)
		printf("%d day%s %d hour%s",
			delta->tm_yday, delta->tm_yday == 1 ? "" : "s",
			delta->tm_hour, delta->tm_hour == 1 ? "" : "s");
	else
		if (delta->tm_hour >= 10)
			printf("%d hours", delta->tm_hour);
		else if (delta->tm_hour > 0)
			printf("%d hour%s %d minute%s",
				delta->tm_hour, delta->tm_hour == 1 ? "" : "s",
				delta->tm_min, delta->tm_min == 1 ? "" : "s");
		else
			if (delta->tm_min >= 10)
				printf("%2d minutes", delta->tm_min);
			else if (delta->tm_min == 0)
				printf("%2d seconds", delta->tm_sec);
			else
				printf("%d minute%s %d second%s",
					delta->tm_min,
					delta->tm_min == 1 ? "" : "s",
					delta->tm_sec,
					delta->tm_sec == 1 ? "" : "s");
	printf("%s", after);
}

matchcmp(gname, login, given)
	register char *gname;
	char *login;
	char *given;
{
	char buffer[100];
	register char *bp, *lp;
	register c;

	if (*gname == ASTERISK)
		gname++;
	lp = 0;
	bp = buffer;
	for (;;)
		switch (c = *gname++) {
		case SAMENAME:
			for (lp = login; bp < buffer + sizeof buffer
					 && (*bp++ = *lp++);)
				;
			bp--;
			break;
		case ' ':
		case COMMA:
		case '\0':
			*bp = 0;
			if (namecmp(buffer, given))
				return (1);
			if (c == COMMA || c == 0)
				return (0);
			bp = buffer;
			break;
		default:
			if (bp < buffer + sizeof buffer)
				*bp++ = c;
		}
	/*NOTREACHED*/
}

namecmp(name1, name2)
	register char *name1, *name2;
{
	register c1, c2;

	for (;;) {
		c1 = *name1++;
		if (islower(c1))
			c1 = toupper(c1);
		c2 = *name2++;
		if (islower(c2))
			c2 = toupper(c2);
		if (c1 != c2)
			break;
		if (c1 == 0)
			return (1);
	}
	if (!c1) {
		for (name2--; isdigit(*name2); name2++)
			;
		if (*name2 == 0)
			return (1);
	} else if (!c2) {
		for (name1--; isdigit(*name1); name1++)
			;
		if (*name2 == 0)
			return (1);
	}
	return (0);
}

netfinger(name)
	char *name;
{
	char *host;
	char fname[100];
	struct hostent *hp;
	struct servent *sp;
	struct sockaddr_in sin;
	int s;
	char *rindex();
	register FILE *f;
	register int c;
	register int lastc;

	if (name == NULL)
		return (0);
	host = rindex(name, '@');
	if (host == NULL)
		return (0);
	*host++ = 0;
	hp = gethostbyname(host);
	if (hp == NULL) {
		static struct hostent def;
		static struct in_addr defaddr;
		static char *alist[1];
		static char namebuf[128];
		int inet_addr();

		defaddr.s_addr = inet_addr(host);
		if (defaddr.s_addr == -1) {
			printf("unknown host: %s\n", host);
			return (1);
		}
		strcpy(namebuf, host);
		def.h_name = namebuf;
# ifdef h_addr
		def.h_addr_list = alist;
# endif h_addr
		def.h_addr = (char *)&defaddr;
		def.h_length = sizeof (struct in_addr);
		def.h_addrtype = AF_INET;
		def.h_aliases = 0;
		hp = &def;
	}
	printf("[%s] ", hp->h_name);
	sin.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(IPPORT_FINGER);
	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (s < 0) {
		fflush(stdout);
		perror("socket");
		return (1);
	}
	if (connect(s, (char *)&sin, sizeof (sin)) < 0) {
		fflush(stdout);
		perror("connect");
		close(s);
		return (1);
	}
	printf("\n");
	if (large) write(s, "/W ", 3);
	write(s, name, strlen(name));
	write(s, "\r\n", 2);
	f = fdopen(s, "r");
	while ((c = getc(f)) != EOF) {
		switch(c) {
		case 0210:
		case 0211:
		case 0212:
		case 0214:
			c -= 0200;
			break;
		case 0215:
			c = '\n';
			break;
		}
		lastc = c;
		if (isprint(c) || isspace(c))
			putchar(c);
		else
			putchar(c ^ 100);
	}
	if (lastc != '\n')
		putchar('\n');
	(void)fclose(f);
	return (1);
}

/*
 *	AnyMail - takes a username (string pointer thereto), and
 *	prints on standard output whether there is any unread mail,
 *	and if so, how old it is.	(JCM@Shasta 15 March 80)
 */
#ifdef SYSV
#define preamble "/var/mail/"	/* mailboxes are there */
#else
#define preamble "/usr/spool/mail/"	/* mailboxes are there */
#endif /* SYSV */
AnyMail(name)
char *name;
{
	struct stat buf;		/* space for file status buffer */
	char *mbxdir = preamble; 	/* string with path preamble */
	char *mbxpath;			/* space for entire pathname */

	char *ctime();			/* convert longword time to ascii */
	char *timestr;

	mbxpath = malloc(strlen(name) + strlen(preamble) + 1);

	strcpy(mbxpath, mbxdir);	/* copy preamble into path name */
	strcat(mbxpath, name);		/* concatenate user name to path */

	if (stat(mbxpath, &buf) == -1 || buf.st_size == 0) {
	    /* Mailbox is empty or nonexistent */
	    printf("No unread mail\n");
        } else {
	    if (buf.st_mtime == buf.st_atime) {
		/* There is something in the mailbox, but we can't really
		 *   be sure whether it is mail held there by the user
		 *   or a (single) new message that was placed in a newly
		 *   recreated mailbox, so we punt and call it "unread mail."
		 */
		printf("Unread mail since ");
	        printf(ctime(&buf.st_mtime));
	    } else {
		/* New mail has definitely arrived since the last time
		 *   mail was read.  mtime is the time the most recent
		 *   message arrived; atime is either the time the oldest
		 *   unread message arrived, or the last time the mail
		 *   was read.
		 */
		printf("New mail received ");
		timestr = ctime(&buf.st_mtime);	/* time last modified */
		timestr[24] = '\0';		/* suppress newline (ugh) */
		printf(timestr);
		printf(";\n  unread since ");
	        printf(ctime(&buf.st_atime));	/* time last accessed */
	    }
	}
	
	free(mbxpath);
}

/*
 * return true iff we've already printed project/plan for this uid;
 * if not, enter this uid into table (so this function has a side-effect.)
 */
#define	PPMAX	200		/* assume no more than 200 logged-in users */
uid_t	PlanPrinted[PPMAX+1];
int	PPIndex = 0;		/* index of next unused table entry */

AlreadyPrinted(uid)
uid_t uid;
{
	int i = 0;
	
	while (i++ < PPIndex) {
	    if (PlanPrinted[i] == uid)
		return(1);
	}
	if (i < PPMAX) {
	    PlanPrinted[i] = uid;
	    PPIndex++;
	}
	return(0);
}
