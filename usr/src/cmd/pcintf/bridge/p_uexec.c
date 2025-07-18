/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_uexec.c	1.1.1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)p_uexec.c	4.5	LCC);	/* Modified: 19:48:12 10/2/90 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/* uexec.c						*/
/*							*/
/*							*/
/* Created	August 8, 1985	Niket K. Patwardhan	*/

/*               **** Sleaze Alert ****			*
 * Down where uexec() does it's fork, it is possible	*
 * for the child to die before the pid gets assigned	*	 
 * into the "ux" table.  If this happens the status	*	 
 * never gets saved and a future uwait will cause a 	*	 
 * hang on the PC.  The correct solution is to inhibit	* 
 * SIGCLD around the fork, but this requires different	*	 
 * approaches on different unix systems.  AT&T also 	*	 
 * makes vague warnings about expecting SIGCLD to 	*	 
 * continue operating as it currently does in future	*	 
 * versions of unix.  So I made a sleazy, but generic,	*	 
 * fix using the variables deadkid and deadstat.  This	*	 
 * fix does not close the hole completely, but makes it	*	 
 * very small.						*	 
 *			jta  4/29/86			*/	 
	 
#include "pci_types.h"
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include "table.h"

/*** TUNABLE PARAMETERS ***/

#define	NARG	29	/* Maximum number of arguments */
#define	NENV	29	/* Maximum number of environment strings */
#define NUXL	16	/* Maximum number of unwaited for uexecs */


/* Pointer arrays for execve */
static char *argp[NARG+3];
static char *envp[NENV+3];
static char *buf, *bufp, *ep;
static int buf_size;
static int n_uexec = 0;

extern char **environ;		/* Server's environment pointer */

/* Log of uexeced tasks */
static struct ux
{
	long pid;
	long stat;
	char cmd[55];
	char flag;
} ux[NUXL];
long deadkid, deadstat;
#define	GOOD	-1
#define BAD	0
#define	DONE	1
#define	WORKING	2

extern void sig_catcher();
extern char *malloc();
extern char *realloc();
extern FILE *logFile;

#ifndef SIGNAL_KNOWN
#if	!defined(BSD43) && !defined(RIDGE)
#if	defined(ICT) || defined(CCI) || defined(UPORT)
extern int (*signal())();
#else
extern void (*signal())();
#endif /* ICT */
#endif /* !BSD43 */
#endif /* SIGNAL_KNOWN */

extern unsigned alarm();

/***** uexec() ***********************************************************/
/*									  */
/*	ap	Pointer to text area of incoming packet			  */
/*	type	Indicator of first and last packets			  */
/*	msize	Text area size						  */
/*	asize	If first packet, size of all text areas together	  */
/*	eoff	If packet with first environment, its offset, else msize  */
/*	flag	Save status flag. (0 = don't save status)		  */
/*									  */
/*	RETURNS pid of uexec'ed process, 0 = failure, -1 = not done yet	  */
/**************************************************************************/
long
uexec(ap, type, msize, asize, eoff, flag)
char *ap;
int type, msize, asize, eoff, flag;
{
	register int i;
	register char *cp;
	char *tp;
	long j,k;
	char **environp, **envpp;

	log("uexec: %s %x %d %d %x %x\n", ap, type, msize, asize, eoff, flag);
	/* Collect strings from all packets. Assign memory on first packet */
	if(type & 1)
	{
		log("allocing memory %d\n", asize);
		buf = malloc(asize);
		if(buf <= (char *)0)
		{
			log("uexec: No buffer\n");
			return(BAD);
		}
		bufp = buf;
		ep = buf+eoff;
		buf_size = 2 * asize;
	}
	memcpy(bufp, ap, msize);
	bufp += msize;
	if (!(type & 2))
		return(GOOD);

	log("uexec: Last packet received\n");
	
	/*** Now all arguments have been collected ***/

	/*** translate strings */
	if ((cp = malloc(buf_size)) == NULL) {
		log("uexec: No buffer space\n");
		free(buf);
		return(BAD);
	}
	set_tables(D2U);
	lcs_set_options(LCS_MODE_NO_MULTIPLE, 0, country);
	while ((i = lcs_translate_block(cp, buf_size, buf, ep - buf)) < 0 &&
	       lcs_errno == LCS_ERR_NOSPACE) {
		buf_size <<= 1;
		if ((cp = realloc(cp, buf_size)) == NULL) {
			log("uexec: No buffer space\n");
			free(buf);
			return(BAD);
		}
	}
	if (i < 0) {
		log("uexec: translation failure.  lcs_errno = %d\n", lcs_errno);
		free(buf);
		free(cp);
		return(BAD);
	}
	tp = &cp[i];
	j = i;
	while ((i = lcs_translate_block(tp, buf_size, ep, bufp - ep)) < 0 &&
	       lcs_errno == LCS_ERR_NOSPACE) {
		buf_size <<= 1;
		if ((cp = realloc(cp, buf_size)) == NULL) {
			log("uexec: No buffer space\n");
			free(buf);
			return(BAD);
		}
		tp = &cp[j];
	}
	if (i < 0) {
		log("uexec: translation failure.  lcs_errno = %d\n", lcs_errno);
		free(buf);
		free(cp);
		return(BAD);
	}
	bufp = &tp[i];
	free(buf);
	ep = tp;
	buf = cp;

	cp += strlen(cp) + 1;	/* strip command to get argv[0] */
	/** Collect string pointers **/
	for(i = 0; (i < NARG+2) && (cp < ep); i++)
	{
	    log("%s\n", cp);
	    argp[i] = cp;
	    cp += strlen(cp) + 1;
	}
	log("uexec: %d arguments\n", i-2);
	if (cp != ep) {
		free(buf);
		return(BAD);
	}
	argp[i] = 0;

	/** Now get environment pointers **/
	log("uexec: Getting env pointers\n");
	envpp = envp;
	for (environp = environ, i = 0; *environp && i < NENV; i++)
		*envpp++ = *environp++;
#ifdef JANUS
	for (environp = envp; environp < envpp; environp++)
		if (envcmp(*environp, "ONUNIX="))
			break;
	if (environp == envpp) {
#ifdef MERGE386
		*envpp++ = "ONUNIX=MERGE386";
#else
		*envpp++ = "ONUNIX=MERGE286";
#endif
		i++;
	}
#endif /* JANUS */
	while (cp < bufp) {
		log("%s\n", cp);
		for (environp = envp; environp < envpp; environp++) {
			if (envcmp(*environp, cp)) {
				for (ep = cp; *ep && *ep != '='; ep++)
					;
				if (*ep && *++ep)
					*environp = cp;
				else {
					*environp-- = *--envpp;
					i--;
				}
				break;
			}
		}
		if (environp == envpp && i <= NENV) {
			*envpp++ = cp;
			i++;
		}
		cp += strlen(cp) + 1;
	}
	for (environp = envp; environp < envpp; environp++)
		if (envcmp(*environp, "PATH="))
			break;
	if (environp == envpp) {
		*envpp++ = "PATH=/bin:/usr/bin";
		i++;
	}
	log("uexec: Got %d env pointers\n", i-1);
	if (cp != bufp) {
		free(buf);
		return(BAD);
	}
	envp[i] = 0;

	/** All exec args setup, so go **/
	/*** Remember the command, and then fork and remember the PID ***/
	log("Looking for free slot\n");
	for(i=0; i<NUXL; i++)
	{
		if(ux[i].pid) continue;
		strncpy(ux[i].cmd, buf, 55);
		ux[i].flag = WORKING;
		log("Forking\n");
		if(ux[i].pid = fork())		/* Really mean an assign! */
		{
			if (deadkid == ux[i].pid) {
				ux[i].stat = deadstat;
				ux[i].flag = DONE;
				deadkid = 0;
			}
			free(buf);
			if(ux[i].pid > 0)
			{
				j = ux[i].pid;
				if(!flag) {
					ux[i].pid = 0;
				}
				else {
					n_uexec++;
				}
				log("Uexec'ed process %d\n", j);
				return(j);
			}
			log("Bad fork!\n");
			ux[i].pid = 0;
			return(BAD);
		}

		/** Child process **/
#ifdef UNX42		    
		setpgrp(getpid(),getpid());
#else	    
		setpgrp();    
#endif	    
		for (i = 0; i < uMaxDescriptors(); i++)
			close(i);
		while (open("/dev/null", 2) == -1 && errno == EINTR)
			;
		dup(0);
		dup(0);
		log("uexec: %s\n", buf);
		environ = envp;
		execvp(buf, argp);
		exit(1);
	}
	free(buf);
	return(BAD);
}

/******	uwait() ***********************************************************/
/*									  */
/*	wf	Wait flag						  */
/*	addr	Pointer to output packet				  */
/*									  */
/*	RETURNS nothing!						  */
/**************************************************************************/	
void
uwait(wf, addr)
int wf;
struct output *addr;
{
	register int i,j;
	int statbuf;
#if !defined(SYS5_3) && !defined(SYS5_4)
	int (*saved)();
#else	/* SYS5_3 and SYS5_4 */
	void (*saved)();
#endif	/* SYS5_3 */
	unsigned oldtime;

	i = -1;
	if(n_uexec > 0)
	{
		i = 0;
		for(j = 0; j<NUXL; j++)
		if(ux[j].pid && (ux[j].flag == DONE))
		{
			i = ux[j].pid;
			ux[j].pid = 0;
			n_uexec--;
			statbuf = ux[j].stat;
			goto done;
		}

/* retry: */
		log("uwait: at 'retry' label %d\n", n_uexec);
		signal(SIGALRM, sig_catcher);
		saved = signal(SIG_CHILD, SIG_DFL);
		if(wf)
			oldtime = alarm(30);
		else
			oldtime = alarm(1);
		log("uwait: waiting......\n");
		u_wait(&statbuf);
		signal(SIG_CHILD, saved);
		alarm(oldtime);

		for(j = 0; j<NUXL; j++)
		if(ux[j].pid && (ux[j].flag == DONE))
		{
			i = ux[j].pid;
			ux[j].pid = 0;
			n_uexec--;
			statbuf = ux[j].stat;
			goto done;
		}
	}
done:
	log("uwait: Process %d status %x count %d\n", i, statbuf, j);
	addr->hdr.res = 0;
	addr->hdr.f_size = i;
	if((statbuf & 0xFF) == 0) addr->hdr.b_cnt = (statbuf >> 8) & 0xFF;
	else if((statbuf & 0xFF) == 0x7F)
	   addr->hdr.b_cnt = ((statbuf >> 8) & 0xFF) | 0x400;
	else if((statbuf & 0xFF00) == 0)
	   addr->hdr.b_cnt = (statbuf & 0x7F) | ((statbuf & 0x80)?0x200:0x100);
}

u_wait(statbuf)
int *statbuf;
{
	register int i,j;

	i = wait(statbuf);
	if (i == -1) 
	{
		log("u_wait: wait() failed, errno %d\n", errno);
		return(-1);
	}
	log("uwait: Process %d just died\n", i);
	if ((j = find_ux(i)) == -1) {
		deadkid = i;
		deadstat = *statbuf;
	}
	else {
		ux[j].stat = *statbuf;
		ux[j].flag = DONE;
	}
	return(i);
}

/************************************************************************
* Name: 	pci_ukill()
* 
* Purpose:	Allow PCI user to send signals to unix processes.
*
* Input:	pid	Process id for kill function
*		sig	Signal to send
*		addr	Pointer to reply packet
*
* Output:	addr->hdr.res contains code reflecting success/failure
*
*************************************************************************/

void
pci_ukill(pid, sig, addr)
long pid;
int sig;
struct output *addr;
{
    if (kill((int) pid, sig)) {
	addr->hdr.res = INVALID_FUNCTION;
    }
    else {
	addr->hdr.res = SUCCESS;
    }
}
	    

int
find_ux(procid)
int procid;
{
	register int j;
	
	for(j=0; j<NUXL; j++) 
		if (ux[j].pid == procid) return (j);
	return (-1);
}

kill_uexecs()
{
    int i;
    
    for (i=0; i<NUXL; i++) {
	if (ux[i].flag == WORKING)
	    kill(-ux[i].pid,SIGHUP);
    }
}

/*
 *  Compare two environment strings for name equality
 */

envcmp(s1, s2)
register char *s1, *s2;
{
	while (*s1 == *s2) {
		if (*s1 == '=')
			return 1;
		s1++;
		s2++;
	}
	return 0;
}
