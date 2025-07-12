/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/protprint.c	1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)protprint.c	4.1	LCC);	/* Modified: 20:07:17 8/7/89 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include <sys/types.h>
#include <utmp.h>
#include <stdio.h>

char  scratch[] = "\n# PC-Interface copyright (c) 1984, 1987 by Locus Computing Corporation.\n# All Rights Reserved.\n";

extern struct utmp *getutent();	/* sys5 utmp utilities */
extern void endutent();

#define	PROTFILE	"/usr/pci/prot"
#define	SERIALSIZE	16
#define	STRSER	((2*SERIALSIZE)+1)

struct protrec {
	char serial[SERIALSIZE];
	char check_flag;
	char check_serial[SERIALSIZE];
	time_t starttime;
};

#ifdef IXR2
char *protFile();
#endif

main(argc,argv)
int argc;
char *argv[];
{
	print_prot();
}

struct protrec rec;
print_prot()
{
	struct utmp *ut;	/* utmp entry from getutent() */
	char ser1[STRSER];
	char ser2[STRSER];
	int protfd;
	register int i;

#ifdef	IXR2
 	if ((protfd = open(protFile(),0))< 0)
 		printf("can't open %s\n",protFile());
#else
	if ((protfd = open(PROTFILE,0))< 0) {
		printf("can't open %s\n",PROTFILE);
		exit(1);
	}
#endif
 	printf("%8s:%4s:%5s:%32s:%3s:%32s:%10s\n","User","id","pid","serial",
 		"flg","checkserial", "starttime");
	while((ut=getutent())!=NULL) {
	
		if ((i=read(protfd,&rec,sizeof rec)) != sizeof rec) {
			break;
		}
		if (zeroser(rec.serial) && zeroser(rec.check_serial)){
			continue;
		}

		serial2str(rec.serial,ser1);
		serial2str(rec.check_serial,ser2);

 		printf("%.8s:%4s:%5d:%32s:  %c:%32s:%10ld\n",
			ut->ut_user,ut->ut_id,ut->ut_pid,
 			ser1,rec.check_flag,ser2,rec.starttime);

	}
	endutent();
	close(protfd);
}

serial2str(serial,serstr)
char serial[];
char serstr[];
{
	char str1[3];
	register int i;
	register char *s=serstr;

	for (i=0;i<SERIALSIZE;i++) {
		sprintf(str1,"%02x",serial[i]);
		*s++=str1[0];
		*s++=str1[1];
	}
	*s++=0;
}

zeroser(ser)
char ser[];
{
	register int i;
	for (i=0;i<SERIALSIZE;i++) {
		if (ser[i] != 0) return(0);
	}
	return(1);
}
#ifdef	IXR2
#include <sf.h>
#define PROTSUFFIX	"/pci/prot"
/*
 * protFile		This routine returns the ptr to a statically 
 *			allocated block that contains the path of the
 *			prot file for this server.
 */

char *
protFile()
{
	static char protfile[48];
	register char *cp, *pf = &protfile[0];

	if ((cp = sflocal(site(0L))) == NULL) {	/* pass it a long */
		printf("protFile: cannot get local prot file path.\n"); 
		exit(1);
	}
	endsf();

	strcpy(pf, cp);
	strcat(pf, PROTSUFFIX);
	return(&protfile[0]);
}
#endif
