/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.

 * Copyright (c) 1984, 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ident	"@(#)cron:atq.c	1.7.3.1"
/*
 *
 *	Synopsis:  atq [ -c ] [ -n ] [ name ... ]
 *
 *
 *	Print the queue of files waiting to be executed. These files 
 *	were created by using the "at" command and are located in the 
 *	directory defined by ATDIR.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <dirent.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <ctype.h>
#include <unistd.h>
#include <locale.h>
#include "cron.h"
 
extern char	*errmsg();
extern char	*strchr();

extern int errno;

/*
 * Months of the year
 */
static char *mthnames[12] = {
	"Jan","Feb","Mar","Apr","May","Jun","Jul",
	"Aug","Sep","Oct","Nov","Dec",
};

int numentries;				/* number of entries in spooling area */
int namewanted = 0;			/* only print jobs belonging to a certain person */
struct dirent **queue;			/* the queue itself */

#define INVALIDUSER	"you are not a valid user (no entry in /etc/passwd)"
#define NOTALLOWED	"you are not authorized to use at.  Sorry."


main(argc,argv)
int argc;
char **argv;
{

	register struct passwd *pp;	/* password file entry pointer */
	struct passwd pr;
	register int i;
	int cflag = 0;			/* print in order of creation time */
	int nflag = 0;			/* just print the number of jobs in 
					   queue */
	int usage();			/* print usage info and exit */
	extern int creation();		/* sort jobs by date of creation */
	extern int execution();		/* sort jobs by date of execution */
	int filewanted();	/* should a file be included in queue?*/
	int printqueue();		/* print the queue */
	int countfiles();		/* count the number of files in queue
					   for a given person */
	uid_t *uidlist = NULL;	/* array of specific owner ID(s) requested*/
	int argnum = 0;			/* number of names passed as arg't */
	int badarg = 0;
	char *c;


	--argc, ++argv;

	(void)setlocale(LC_ALL, "");
	pp = getpwuid(getuid());
	pr.pw_uid = pp->pw_uid;
	if ((pr.pw_name = (char *)malloc(strlen(pp->pw_name))) == NULL) 
		atabortperror("can't allocate memory");
	strcpy(pr.pw_name, pp->pw_name);

	if (pp == NULL)
		atabort(INVALIDUSER);
	if (!allowed(pp->pw_name,ATALLOW,ATDENY))
		atabort(NOTALLOWED);

	/*
	 * Interpret command line flags if they exist.
	 */
	while (argc > 0 && **argv == '-') {
		(*argv)++;
		while (**argv) switch (*(*argv)++) {

			case 'c' :	cflag++; 
					break;

			case 'n' :	nflag++; 
					break;

			default	 :	usage();

		}
		--argc, ++argv;
	}

	/*
	 * If a certain name (or names) is requested, set a pointer to the
	 * beginning of the list.
	 */
	if (argc > 0) {
		++namewanted;
		uidlist = (uid_t *) malloc(argc * sizeof(uid_t));
		if (uidlist == NULL)
			atabortperror("can't allocate list of users");
		for (i = 0; i < argc; i++) {
			if (pr.pw_uid == 0 || 
					strcmp(pr.pw_name, argv[i]) == 0) {
				if ((pp = getpwnam(argv[i])) == NULL) {
					(void) fprintf(stderr,"atq: No such user %s\n",
					argv[i]);
					exit(1);
				}
				uidlist[argnum] = pp->pw_uid;
				argnum++;
			}
			else
				badarg++;
		}
		if (badarg)
			if (argnum)
				printf("Printing queue information only for %s:\n", pr.pw_name);
			else {
				printf("atq: Non-priviledged user cannot request information regarding other users\n");
				exit(1);
			}
	}
	else if (pr.pw_uid != 0) {
		/* no argument specified and the invoker is not root */
		++namewanted;
		argnum = 1;
		if ((uidlist = (uid_t *)malloc(sizeof(uid_t))) == NULL)
			atabortperror("can't allocate list of users");
		*uidlist = pr.pw_uid;		
	}

	/*
	 * Move to the spooling area and scan the directory, placing the
	 * files in the queue structure. The queue comes back sorted by
	 * execution time or creation time.
	 */
	if (chdir(ATDIR) == -1)
		atabortperror(ATDIR);
	if ((numentries = ascandir(".", &queue, filewanted,
	    (cflag) ? creation : execution)) < 0)
		atabortperror(ATDIR);


	/*
	 * Either print a message stating:
	 *
	 *	1) that the spooling area is empty.
	 *	2) the number of jobs in the spooling area.
	 *	3) the number of jobs in the spooling area belonging to 
	 *	   a certain person.
	 *	4) that the person requested doesn't have any files in the
	 *	   spooling area.
	 *
	 * or send the queue off to "printqueue" for printing.
	 *
	 * This whole process might seem a bit elaborate, but it's worthwhile
	 * to print some informative messages for the user.
	 *
	 */
	if ((numentries == 0) && (!nflag)) {
		printf("no files in queue.\n");
		exit(0);
	}
	if (nflag) {
		printf("%d\n",(namewanted) ? countfiles(uidlist,argnum) : numentries);
		exit(0);
	}
	if ((namewanted) && (countfiles(uidlist,argnum) == 0)) {
		if (argnum == 1)
			if (argnum != argc) c = pr.pw_name;
			else c = *argv;
		printf("no files for %s.\n", (argnum == 1) ?
					c : "specified users");
		exit(0);
	}
	printqueue(uidlist,argnum);
	exit(0);
}

/*
 * Count the number of jobs in the spooling area owned by a certain person(s).
 */
countfiles(uidlist,nuids)
uid_t *uidlist;
int nuids;
{
	register int i, j;			/* for loop indices */
	int entryfound;				/* found file owned by user(s)*/
	int numfiles = 0;			/* number of files owned by a
						   certain person(s) */
	register uid_t *ptr;			/* scratch pointer */
	struct stat stbuf;			/* buffer for file stats */


	/*
	 * For each file in the queue, see if the user(s) own the file. We
	 * have to use "entryfound" (rather than simply incrementing "numfiles")
	 * so that if a person's name appears twice on the command line we 
	 * don't double the number of files owned by him/her.
	 */
	for (i = 0; i < numentries ; i++) {
		if ((stat(queue[i]->d_name, &stbuf)) < 0) {
			continue;
		}
		ptr = uidlist;
		entryfound = 0;

		for (j = 0; j < nuids; j++) {
			if (*ptr == stbuf.st_uid)
				++entryfound;
			++ptr;
		}
		if (entryfound)
			++numfiles;
	}
	return(numfiles);
}

/*
 * Print the queue. If only jobs belonging to a certain person(s) are requested,
 * only print jobs that belong to that person(s).
 */
printqueue(uidlist,nuids)
uid_t *uidlist;
int nuids;
{
	register int i, j;			/* for loop indices */
	int rank;				/* rank of a job */
	int entryfound;				/* found file owned by user(s)*/
	int printrank();			/* print the rank of a job */
	int powner();				/* print the name of the owner
						   of the job */
	char *getname();
	register uid_t *ptr;			/* scratch pointer */
	struct stat stbuf;			/* buffer for file stats */
	char curqueue;			/* queue of current job */
	char lastqueue;				/* queue of previous job */

	/*
	 * Print the header for the queue.
	 */
	printf(" Rank	  Execution Date     Owner     Job         Queue   Job Name\n");

	/*
	 * Print the queue. If a certain name(s) was requested, print only jobs
	 * belonging to that person(s), otherwise print the entire queue.
	 * Once again, we have to use "entryfound" (rather than simply 
	 * comparing each command line argument) so that if a person's name 
	 * appears twice we don't print each file owned by him/her twice.
	 *
	 *
	 * "printrank", "printdate", and "printjobname" all take existing 
	 * data and display it in a friendly manner.
	 *
	 */
	lastqueue = '\0';
	for (i = 0; i < numentries; i++) {
		if ((stat(queue[i]->d_name, &stbuf)) < 0) {
			continue;
		}
		curqueue = *(strchr(queue[i]->d_name, '.') + 1);
		if (curqueue != lastqueue) {
			rank = 1;
			lastqueue = curqueue;
		}
		if (namewanted) {
			ptr = uidlist;
			entryfound = 0;

			for (j = 0; j < nuids; j++) {
				if (*ptr == stbuf.st_uid)
					++entryfound;
				++ptr;
			}
			if (!entryfound)
				continue;
		}
		printrank(rank++);
		printdate(queue[i]->d_name);
		printf("%-8s",getname(stbuf.st_uid));
		printf("%-8s",queue[i]->d_name);
		printf("     %c",curqueue);
		printjobname(queue[i]->d_name);
	}
	++ptr;
}

/*
 * Print the owner of the job. This is the owner of the spoolfile.
 * If we run into trouble getting the name, we'll just print "???".
 */
powner(file)
char *file;
{
	struct stat statb;
	char *getname();

	if (stat(file,&statb) < 0) {
		printf("%-10.9s","???");
		(void) fprintf(stderr,"atq: Couldn't stat spoolfile %s: %s\n",
		    errmsg(errno));
		return;
	}

	printf("%-10.9s",getname(statb.st_uid));
}
	

/*
 * Get the uid of a person using his/her login name. Return -1 if no
 * such account name exists.
 */
uid_t
getid(name)
char *name;
{

	struct passwd *pwdinfo;			/* password info structure */


	if ((pwdinfo = getpwnam(name)) == 0)
		return((uid_t)-1);

	return(pwdinfo->pw_uid);
}

/*
 * Get the full login name of a person using his/her user id.
 */
char *
getname(uid)
uid_t uid;
{
	register struct passwd *pwdinfo;	/* password info structure */
	

	if ((pwdinfo = getpwuid(uid)) == 0)
		return("???");
	return(pwdinfo->pw_name);
}

/*
 * Print the rank of a job. (I've got to admit it, I stole it from "lpq")
 */
static 
printrank(n)
{
	static char *r[] = {
		"th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th"
	};

	if ((n/10) == 1)
		 printf("%3d%-5s", n,"th");
	else
		 printf("%3d%-5s", n, r[n%10]);
}

/*
 * Print the date that a job is to be executed. This takes some manipulation 
 * of the file name.
 */
printdate(filename)
char *filename;
{
	time_t	jobdate;
	extern time_t num();
	register struct tm *unpackeddate;
	char date[18];				/* reformatted execution date */

	/*
	 * Convert the file name to a date.
	 */
	jobdate = num(&filename);
	unpackeddate = localtime(&jobdate);

	/*
	 * Format the execution date of a job.
	 */
	sprintf(date,"%3s %2d, 19%2d %02d:%02d",mthnames[unpackeddate->tm_mon],
	    unpackeddate->tm_mday,unpackeddate->tm_year,
	    unpackeddate->tm_hour,unpackeddate->tm_min);

	/*
	 * Print the date the job will be executed.
	 */
	printf("%-21.18s",date);
}

/*
 * Print a job name. If the old "at" has been used to create the spoolfile,
 * the three line header that the new version of "at" puts in the spoolfile.
 * Thus, we just print "???".
 */
printjobname(file)
char *file;
{
	char *ptr;				/* scratch pointer */
	char jobname[28];			/* the job name */
	FILE *filename;				/* job file in spooling area */

	/*
	 * Open the job file and grab the third line.
	 */
	printf("     ");

	if ((filename = fopen(file,"r")) == NULL) {
		printf("%.27s\n", "???");
		(void) fprintf(stderr, "atq: Can't open job file %s: %s\n", file,
		    errmsg(errno));
		return;
	}
	/*
	 * Skip over the first and second lines.
	 */
	fscanf(filename,"%*[^\n]\n");

	/*
	 * Now get the job name.
	 */
	if (fscanf(filename,": jobname: %27s%*[^\n]\n",jobname) != 1) {
		printf("%.27s\n", "???");
		fclose(filename);
		return;
	}
	fclose(filename);

	/*
	 * Put a pointer at the begining of the line and remove the basename
	 * from the job file.
	 */
	ptr = jobname;
	if ((ptr = (char *)strrchr(jobname,'/')) != 0)
		++ptr;
	else 
		ptr = jobname;

	if (strlen(ptr) > 23)
		printf("%.23s ...\n",ptr);
	else
		printf("%.27s\n",ptr);
}



/*
 * Sort files by queue, time of creation, and sequence. (used by "ascandir")
 */
creation(d1, d2)
struct dirent **d1, **d2;
{
	register char *p1, *p2;
	register int i;
	struct stat stbuf1, stbuf2;
	register int seq1, seq2;

	if ((p1 = strchr((*d1)->d_name, '.')) == NULL)
		return(0);
	if ((p2 = strchr((*d2)->d_name, '.')) == NULL)
		return(0);
	p1++;
	p2++;
	if ((i = *p1++ - *p2++) != 0)
		return(i);

	if (stat((*d1)->d_name,&stbuf1) < 0)
		return(0);

	if (stat((*d2)->d_name,&stbuf2) < 0)
		return(0);

	if (stbuf1.st_ctime < stbuf2.st_ctime)
		return(-1);
	else if (stbuf1.st_ctime > stbuf2.st_ctime)
		return(1);
	p1++;
	p2++;
	seq1 = atoi(p1);
	seq2 = atoi(p2);
	return(seq1 - seq2);
}
	
/*
 * Sort files by queue, time of execution, and sequence. (used by "ascandir")
 */
execution(d1, d2)
struct dirent **d1, **d2;
{
	register char *p1, *p2;
	register int i;
	char *name1, *name2;
	register time_t time1, time2;
	register int seq1, seq2;
	extern time_t num();

	name1 = (*d1)->d_name;
	name2 = (*d2)->d_name;
	if ((p1 = strchr(name1, '.')) == NULL)
		return(1);
	if ((p2 = strchr(name2, '.')) == NULL)
		return(1);
	p1++;
	p2++;
	if ((i = *p1++ - *p2++) != 0)
		return(i);

	time1 = num(&name1);
	time2 = num(&name2);

	if (time1 < time2)
		return(-1);
	else if (time1 > time2)
		return(1);
	p1++;
	p2++;
	seq1 = atoi(p1);
	seq2 = atoi(p2);
	return(seq1 - seq2);
}


/*
 * Print usage info and exit.
 */
usage() 
{
	fprintf(stderr,"usage:	atq [-c] [-n] [name ...]\n");
	exit(1);
}

aterror(msg)
	char *msg;
{
	fprintf(stderr,"atq: %s\n",msg);
}

atperror(msg)
	char *msg;
{
	fprintf(stderr,"atq: %s: %s\n", msg, errmsg(errno));
}

atabort(msg)
	char *msg;
{
	aterror(msg);
	exit(1);
}

atabortperror(msg)
	char *msg;
{
	atperror(msg);
	exit(1);
}
