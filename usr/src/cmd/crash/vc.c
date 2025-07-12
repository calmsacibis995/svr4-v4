/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:vc.c	1.1"
/*
 * This file contains code for the crash functions:  vcproc, vcdptbl
 */

#include <a.out.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/vc.h>
#include "crash.h"



struct syment *Vcproc, *Vcdptbl, *Vcmaxumdpri;	/* namelist symbol */
vcdpent_t *vcdptbl;
struct vcproc vcbuf;
struct vcproc *vcp;

/* get argumenvc for vcproc function */
int
getvcproc()
{
	int c;

	char *vcprochdg = " TMLFT CPUPRI UPRILIM UPRI UMDPRI FLAGS DISPWAIT   PROCP     NEXT     PREV\n";

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}


	if(!Vcproc)
		if(!(Vcproc = symsrch("vc_plisthead"))) 
			error("vc_plisthead not found in symbol table\n");

	readmem((long)Vcproc->n_value, 1, -1, (char *)&vcbuf,
		sizeof vcbuf, "vcproc table");

	fprintf(fp, "%s", vcprochdg);
	vcp = vcbuf.vc_next;

	for(; vcp != (vcproc_t *)Vcproc->n_value; vcp = vcbuf.vc_next) {
		readmem((long)vcp, 1, -1, (char *)&vcbuf,
			sizeof vcbuf, "vcproc table");
		prvcproc();
	}
}




/* print the VP/ix-Like process table */
int
prvcproc()
{
	fprintf(fp, "  %3d    %2d      %2d    %2d    %2d    %02x     %2d       %.8x %.8x %.8x\n",
	vcbuf.vc_timeleft, vcbuf.vc_cpupri, vcbuf.vc_uprilim,
		vcbuf.vc_upri, vcbuf.vc_umdpri, /* vcbuf.vc_nice, */
		vcbuf.vc_flags, vcbuf.vc_dispwait, vcbuf.vc_procp,
		vcbuf.vc_next, vcbuf.vc_prev);
}


/* get argumenvc for vcdptbl function */

int
getvcdptbl()
{
	int slot = -1;
	int all = 0;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	long addr = -1;
	short vcmaxumdpri;

	char *vcdptblhdg = "SLOT     GLOBAL PRIO     TIME QUANTUM     TQEXP     SLPRET     MAXWAIT     LWAIT\n\n";

	if(!Vcdptbl)
		if(!(Vcdptbl=symsrch("vc_dptbl")))
			error("vc_dptbl not found in symbol table\n");

	if(!Vcmaxumdpri)
		if(!(Vcmaxumdpri=symsrch("vc_maxumdpri")))
			error("vc_maxumdpri not found in symbol table\n");

	optind=1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	readmem((long)Vcmaxumdpri->n_value, 1, -1, (char *)&vcmaxumdpri, sizeof(short),
		"vc_maxumdpri");

	vcdptbl = (vcdpent_t *)malloc((vcmaxumdpri + 1) * sizeof(vcdpent_t));

	readmem((long)Vcdptbl->n_value, 1, -1, (char *)vcdptbl,
	    (vcmaxumdpri + 1) * sizeof(vcdpent_t), "vc_dptbl");

	fprintf(fp,"%s",vcdptblhdg);


	if(args[optind]) {
		all = 1;
		do {
			getargs(vcmaxumdpri + 1,&arg1,&arg2);
			if(arg1 == -1) 
				continue;
			if(arg2 != -1)
				for(slot = arg1; slot <= arg2; slot++)
					prvcdptbl(slot);
			else {
				if(arg1 < vcmaxumdpri + 1)
					slot = arg1;
				prvcdptbl(slot);
			}
			slot = arg1 = arg2 = -1;
		}while(args[++optind]);
	}
	else 
		for(slot = 0; slot < vcmaxumdpri + 1; slot++)
			prvcdptbl(slot);

	free(vcdptbl);
}

/* print the VP/ix-Like dispatcher parameter table */
int
prvcdptbl(slot)
int  slot;
{
	fprintf(fp,"%3d         %4d         %10ld        %3d       %3d        %5d       %3d\n",
	    slot, vcdptbl[slot].vc_globpri, vcdptbl[slot].vc_quantum,
	    vcdptbl[slot].vc_tqexp, vcdptbl[slot].vc_slpret,
	    vcdptbl[slot].vc_maxwait, vcdptbl[slot].vc_lwait);
}
