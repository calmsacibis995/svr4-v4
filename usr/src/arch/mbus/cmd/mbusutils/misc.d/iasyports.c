/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mbus:cmd/mbusutils/misc.d/iasyports.c	1.1.1.3"

/*
 * iasyports - Display iasy device configuration
 *
 * This program displays the configuration of the iasy driver in effect
 * in the kernel.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/iasy.h>
#include <sys/mps.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <nlist.h>
#include <sys/bps.h>

static struct nlist nl[] = {
	{ "iasy_hw",		0L, 0, 0, 0, 0 },
#define	X_IASY	0					/* iasy_hw array -- describes hardware */
	{ "iasy_num",		0L, 0, 0, 0, 0 },
#define	X_NUM	1					/* size of iasy_hw[] */
	{ "rciConsole",		0L, 0, 0, 0, 0 },
#define	X_RCI	2					/*  indicates rci console is present */
	{ 0,				0L, 0, 0, 0, 0 }
};


/*
 *	Main program
*/
/* ARGSUSED */
main(argc, argv)
int argc;
char *argv[];
{
	int	kmem;				/* Kernel memory */
	int iasy_num;			/* size of iasy_hw[] */
	mb2socid_t rciConsole;	/* indicates 279 present */
	int minor, name;
	struct iasy_hw *iasy_hw;
	char 	bootunix[128] = "/unix";

	if ((kmem = open("/dev/kmem", 0)) < 0) {
		perror("kmem:");
		return(1);
	}

	/*
	 * Get the name of unix kernel that got booted.
	 */
	if (bpsopen() == 0) {
		if (bps_get_val("BL_Target_file", sizeof(bootunix), bootunix) != 0){
		perror ("bps_get_val");
		return (1);
		}
	}

	if (nlist(bootunix, nl) == -1) {
		(void) fprintf(stderr, "No namelist for %s\n", bootunix);
		return(1);
	}
	/*
	**	If iasyports is specified with an argument, we are looking for 
	**	an indication that we have a 279.  The rci driver sets the
	**	global variable rciConsole to the socket id if rci has responded
	**	to a broadcast locate.  This code is here because this routine is
	**	used only by the rc script mb2_init and it eliminates the need for
	**	yet another command.
	*/
	if (argc == 2) {
		if (lseek(kmem, nl[X_RCI].n_value, 0) == -1) {
			perror("lseek");
			return(1);
		}
		if (read(kmem, (char *)&rciConsole, sizeof(rciConsole)) == -1) {
			perror("read");
			return(1);
		}
		printf("rciConsole = %d\n", rciConsole);
		return(0);
	}

	/*
	 *	Find out about the iasy configuration.
	 */
	if (lseek(kmem, nl[X_NUM].n_value, 0) == -1) {
		perror("lseek");
		return(1);
	}
	if (read(kmem, (char *)&iasy_num, sizeof(iasy_num)) == -1) {
		perror("read");
		return(1);
	}
	iasy_hw = (struct iasy_hw *)malloc(iasy_num*sizeof(*iasy_hw));
	if (iasy_hw == NULL) {
		(void) fprintf(stderr, "No memory for iasy_hw[]\n");
		return(1);
	}
	(void) lseek(kmem, nl[X_IASY].n_value, 0);
	(void) read(kmem, (char *)iasy_hw, iasy_num*sizeof(*iasy_hw));
	name = 0;
	for (minor = 0; minor < iasy_num; minor++) {
		if (iasy_hw[minor].proc) {
			(void) printf("tty%03d %d\n", name, minor);
			name++;
		}
	}
	return(0);
}
