/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bnu:gename.c	2.9.3.1"

#include "uucp.h"

static struct {
	char	sys[NAMESIZE];
	int	job;
	int	subjob;
} syslst[30];		/* no more than 30 systems per job */

static int nsys = 0;
static int sysseq();

 /* generate file name
  *	pre	-> file prefix
  *	sys	-> system name
  *	grade	-> service grade 
  *	file	-> buffer to return filename must be of size MAXBASENAME+1
  * return:
  *	none
  */
void
gename(pre, sys, grade, file)
char pre, *sys, grade, *file;
{
	int	n;

	DEBUG(9, "gename(%c, ", pre);
	DEBUG(9, "%s, ", sys);
	DEBUG(9, "%c)\n", grade);
	if (*sys == '\0') {
		sys = Myname;
		DEBUG(9, "null sys -> %s\n", sys);
	}
	n = sysseq(sys);
	if (pre == CMDPRE || pre == XQTPRE) {
		(void) sprintf(file, "%c.%.*s%c%.4x",
			pre, SYSNSIZE, sys, grade, syslst[n].job); 
	} else
		(void) sprintf(file, "%c.%.5s%.4x%.3x",
			pre, sys, syslst[n].job & 0xffff,
				++syslst[n].subjob & 0xfff); 
	DEBUG(4, "file - %s\n", file);
	return;
}


#define SLOCKTIME 10
#define SLOCKTRIES 5
#define SEQLEN 4

 /*
  * get next sequence number
  * returns:  
  *	number between 1 and 0xffff
  *
  * sequence number 0 is reserved for polling
  */
static int
getseq(sys)
char	*sys;
{
	register FILE *fp;
	register int i;
	unsigned int n;
	long	seed;
	char seqlock[MAXFULLNAME], seqfile[MAXFULLNAME];

	ASSERT(nsys < sizeof (syslst)/ sizeof (syslst[0]),
	    "SYSLST OVERFLOW", "", sizeof (syslst));

	(void) time(&seed);	/* crank up the sequence initializer */
	srand((unsigned)seed);

	(void) sprintf(seqlock, "%s%s", SEQLOCK, sys);
	BASENAME(seqlock, '/')[MAXBASENAME] = '\0';
	for (i = 1; i < SLOCKTRIES; i++) {
		if ( mklock(seqlock) == SUCCESS )
			break;
		sleep(5);
	}

	ASSERT(i < SLOCKTRIES, Ct_LOCK, seqlock, 0);

	(void) sprintf(seqfile, "%s/%s", SEQDIR, sys);
	if ((fp = fopen(seqfile, "r")) != NULL) {
		/* read sequence number file */
		if (fscanf(fp, "%4x", &n) != 1) {
		    n = rand();
		    clearerr(fp);
		}
		fp = freopen(seqfile, "w", fp);
		ASSERT(fp != NULL, Ct_OPEN, seqfile, errno);
		(void) chmod(seqfile, PUB_FILEMODE);
	} else {
		/* can not read file - create a new one */
		ASSERT((fp = fopen(seqfile, "w")) != NULL,
		    Ct_CREATE, seqfile, errno);
		(void) chmod(seqfile, PUB_FILEMODE);
		n = rand();
	}

	n++;
	n &= 0xffff;	/* 4 byte sequence numbers */
	(void) fprintf(fp, "%.4x\n", n);
	ASSERT(ferror(fp) == 0, Ct_WRITE, seqfile, errno);
	(void) fclose(fp);
	ASSERT(ferror(fp) == 0, Ct_CLOSE, seqfile, errno);
	rmlock(seqlock);
	DEBUG(6, "%s seq ", sys); DEBUG(6, "now %x\n", n);
	(void) strcpy(syslst[nsys].sys, sys);
	syslst[nsys].job = n;
	syslst[nsys].subjob = rand() & 0xfff;	/* random initial value */
	return(nsys++);
}

/*
 *	initSeq() exists because it is sometimes important to forget any
 *	cached work files.  for example, when processing a bunch of spooled X.
 *	files, we must not re-use any C. files used to send back output.
 */

void
initSeq()
{
	nsys = 0;
	return;
}

/* 
 * 	retseq() is used to get the sequence number of a job
 *	for functions outside of this file.
 *
 *	returns
 *
 *		the sequence number of the job for the system.
 */

int
retseq(sys)
char *sys;
{
	int i;

	for (i = 0; i < nsys; i++)
		if (EQUALSN(syslst[i].sys, sys, MAXBASENAME))
			break;

	return(syslst[i].job);
}

static int
sysseq(sys)
char	*sys;
{
	int	i;

	for (i = 0; i < nsys; i++)
		if (strncmp(syslst[i].sys, sys, MAXBASENAME) == SAME)
			return(i);

	return(getseq(sys));
}
