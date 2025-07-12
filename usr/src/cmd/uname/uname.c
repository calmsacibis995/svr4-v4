/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)uname:uname.c	1.31.1.5"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

#include        <stdio.h>
#include        <errno.h>
#include        <signal.h>
#include        <stdlib.h>
#include        "sys/utsname.h"
#include	"sys/systeminfo.h"

#if u3b2 || i386
#include        <string.h>
#include        "sys/types.h"
#include        "sys/fcntl.h"
#include        "sys/stat.h"
#endif

#ifdef i386
#include        "sys/sysi86.h"
#else
#include        "sys/sys3b.h"
#endif

#define ROOTUID (uid_t)0

struct utsname  unstr, *un;
	/*  Enhanced Application Compatibility  */
struct scoutsname scostr, *sco;		/* -X option for SCO-specific info  */
	/*  End Enhanced Application Compatibility  */

extern void exit();
extern int uname();
extern char *getenv();
extern int optind;
extern char *optarg;

extern int errno;

	/*  Enhanced Application Compatibility 
	 *  If SCOMPAT is set:
	       if value is rel:ver
		   release will be set to rel and verseion will be set to ver
	       else release=3.2 and version=2
	 *  End Enhanced Application Compatibility  */
main(argc, argv)
char **argv;
int argc;
{
/* X option is for Enhanced Application Compatibility. It is not advertised */
#if u3b2 || i386
        char *nodename;
        int Sflg=0;
        char *optstring="asnrpvmS:X";
#else
        char *optstring="asnrpvmX";
#endif

        int sflg=0, nflg=0, rflg=0, vflg=0, pflg=0, mflg=0, errflg=0, Xflg=0, optlet;
	char procbuf[256], *eac_rel, *eac_ver;
	char dflt_eac_rel[]="3.2";
	char dflt_eac_ver[]="2";

	umask(~(S_IRWXU|S_IRGRP|S_IROTH) & S_IAMB);
        un = &unstr;
        uname(un);
	eac_ver = eac_rel = getenv("SCOMPAT"); 
	if (eac_rel) {
		eac_ver=strpbrk(eac_rel,  ":"); /* eac_ver points to : */
		if ( eac_ver) {
		   strcpy(eac_ver,++eac_ver);
		   *(eac_rel + (strlen(eac_rel) - strlen(eac_ver) -1)) = '\0';
		}
		   /* pattern must be release:version */
		if (*(eac_rel) == '\0' || *(eac_ver) == '\0') {
		   eac_rel=dflt_eac_rel;
		   eac_ver=dflt_eac_ver;
		}
	}

        while((optlet=getopt(argc, argv, optstring)) != EOF)
                switch(optlet) {
	/*  Enhanced Application Compatibility  */
		case 'X':
			Xflg++;
			break;
	/*  End Enhanced Application Compatibility  */
                case 'a':
                        sflg++; nflg++; rflg++; vflg++; mflg++; pflg++; 
                        break;
                case 's':
                        sflg++;
                        break;
                case 'n':
                        nflg++;
                        break;
                case 'r':
                        rflg++;
                        break;
                case 'v':
                        vflg++;
                        break;
                case 'm':
                        mflg++;
                        break;
		case 'p':
                        pflg++;
                        break;
#if u3b2 || i386
                case 'S':
                        Sflg++;
                        nodename = optarg;

#ifdef i386
			Createrc2file(nodename);
#endif

                        break;
#endif

                case '?':
                        errflg++;
                }

        if(errflg || (optind != argc))
                usage();

#if u3b2 || u3b15 || i386
        if((Sflg > 1) || 
           (Sflg && (sflg || nflg || rflg || vflg || pflg || mflg || Xflg ))) {
                usage();
        }

        /* If we're changing the system name */
        if(Sflg) {
		FILE *file;
		char curname[SYS_NMLN];
		int len = strlen(nodename);
		int curlen, i;
		
                if (getuid() != ROOTUID) {
                        if (geteuid() != ROOTUID) {
                                (void) fprintf(stderr, "uname: not super user\n");
                                exit(1);
                        }
                }

                /*
                 * The size of the node name must be less than SYS_NMLN.
                 */
                if(len > SYS_NMLN - 1) {
                        (void) fprintf(stderr, "uname: name must be <= %d letters\n",SYS_NMLN-1);
                        exit(1);
                }

		/*
		 * NOTE:
		 * The name of the system is stored in a file for use
		 * when booting because the non-volatile RAM on the
		 * porting base will not allow storage of the full
		 * internet standard nodename.
		 * If sufficient non-volatile RAM is available on
		 * the hardware, however, storing the name there would
		 * be preferable to storing it in a file.
		 */

		/* 
		 * Only modify the file if the name requested is
		 * different than the name currently stored.
		 * This will mainly be useful at boot time
		 * when 'uname -S' is called with the name stored 
		 * in the file as an argument, to change the
		 * name of the machine from the default to the
		 * stored name.  In this case only the string
		 * in the global utsname structure must be changed.
		 */
		if ((file = fopen("/etc/nodename", "r")) != NULL) {
			curlen = fread(curname, sizeof(char), SYS_NMLN, file);
			for (i = 0; i < curlen; i++) {
				if (curname[i] == '\n') {
					curname[i] = '\0';
					break;
				}
			}
			if (i == curlen) {
				curname[curlen] = '\0';
			}
			fclose(file);
		} else {
			curname[0] = '\0';
		}

		if (strcmp(curname, nodename) != 0) {
			if ((file = fopen("/etc/nodename", "w")) == NULL) {
				(void) fprintf(stderr, "uname: error in writing name\n");
				exit(1);
			} 
			if (fprintf(file, "%s\n", nodename) < 0) {
				(void) fprintf(stderr, "uname: error in writing name\n");
				exit(1);
			}
			fclose(file);
		}		
		
                /* replace name in kernel data section */
#ifdef i386
                sysi86(SETNAME, nodename, 0);
#else
                sys3b(SETNAME, nodename, 0);
#endif

                exit(0);
        }
#endif
                                                    /* "uname -s" is the default */
        if( !(sflg || nflg || rflg || vflg || pflg || mflg || Xflg ))
                sflg++;
        if(sflg)
                (void) fprintf(stdout, "%.*s", sizeof(un->sysname), un->sysname);
        if(nflg) {
                if(sflg) (void) putchar(' ');
                (void) fprintf(stdout, "%.*s", sizeof(un->nodename), un->nodename);
        }
        if(rflg) {
                if(sflg || nflg) (void) putchar(' ');
		if( eac_rel )
		     (void) fprintf(stdout, "%.*s", strlen(eac_rel), eac_rel);
                else (void) fprintf(stdout, "%.*s", sizeof(un->release), un->release);
        }
        if(vflg) {
                if(sflg || nflg || rflg) (void) putchar(' ');
		if( eac_ver )
		     (void) fprintf(stdout, "%.*s", strlen(eac_ver), eac_ver);
                else (void) fprintf(stdout,
			"%.*s", sizeof(un->version), un->version);
        }
        if(mflg) {
                if(sflg || nflg || rflg || vflg) (void) putchar(' ');
                (void) fprintf(stdout, "%.*s", sizeof(un->machine), un->machine);
        }
	if (pflg) {
		if (sysinfo(SI_ARCHITECTURE, procbuf, sizeof(procbuf)) == -1) {
			fprintf(stderr,"uname: sysinfo failed\n");
			exit(1);
		}
                if(sflg || nflg || rflg || vflg || mflg) (void) putchar(' ');
		(void) fprintf(stdout, "%.*s", strlen(procbuf), procbuf);
	}
	/*  Enhanced Application Compatibility  */
	signal((int)SIGSYS, SIG_IGN);
	if (Xflg) {		
		sco = &scostr;
		if (scoinfo(sco, sizeof(struct scoutsname)) == -1) {
		   perror("uname -X");
		   usage();
		}
		(void) putchar('\n');
		(void) fprintf(stdout,"System = %s\n",sco->sysname);
		(void) fprintf(stdout,"Node = %s\n",sco->nodename);
		if( eac_rel )
		     (void) fprintf(stdout, "Release = %s\n", eac_rel);
		else (void) fprintf(stdout,"Release = %s\n",sco->release);
		(void) fprintf(stdout,"KernelID = %s\n",sco->kernelid);
		(void) fprintf(stdout,"Machine = %s\n",sco->machine);
		(void) fprintf(stdout,"BusType = %s\n",sco->bustype);
		(void) fprintf(stdout,"Serial = %s\n",sco->sysserial);
		(void) fprintf(stdout,"Users = %s\n",sco->numuser);
		(void) fprintf(stdout,"OEM# = %d\n",sco->sysoem);
		(void) fprintf(stdout,"Origin# = %d\n",sco->sysorigin);
		(void) fprintf(stdout,"NumCPU = %d\n",sco->numcpu);
	}
	/*  End Enhanced Application Compatibility  */
        (void) putchar('\n');
        exit(0);
}


usage()
{
/*  The X option is not advertised. hence, it is missing from the 
    usage message	*/

#if u3b2 || i386
        (void) fprintf(stderr, "usage:  uname [-snrvmap]\n\tuname [-S system name]\n");
#else
        (void) fprintf(stderr, "usage:  uname [-snrvmap]\n");
#endif

        exit(1);
}



#ifdef i386
Createrc2file(nodename)
char *nodename;
{
	FILE *fp;
	
	if (strlen(nodename) > (size_t) SYS_NMLN ) {
		(void) fprintf(stderr, "uname: name must be <= %d letters\n", SYS_NMLN);
               	exit(1);
	}

	if ((fp = fopen("/etc/rc2.d/S11uname", "w")) == NULL) {
		fprintf(stderr,"uname: Cannot open /etc/rc2.d/S11uname\n");
       		exit(1);
	} 

	fprintf(fp, "uname -S %s", nodename);
}
#endif
