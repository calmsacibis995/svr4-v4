/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:cmd/comb.c	6.8"
# include	"../hdr/defines.h"
# include	"../hdr/had.h"


struct stat Statbuf;
char Error[128];

struct packet gpkt;
struct sid sid;
int	num_files;
int	Do_prs;
char	*clist;
char	*Val_ptr;
char	Blank[]    =    " ";
int	*Cvec;
int	Cnt;
FILE	*iop;

void	chksid(), setsig(), do_file(), exit(), dolist(), fmterr();
void	finduser(), doflags(), ffreeall(), fredck(), escdodelt();
void	enter(), clean_up(), sinit(), comb();

int	fatal(), setjmp(), stat();
int	sidtoser(), getpred();

char	*fmalloc();

main(argc,argv)
int argc;
register char *argv[];
{
	register int i;
	register char *p;
	char c;
	char *sid_ab();
	int testmore;
	extern int Fcnt;

	Fflags = FTLEXIT | FTLMSG | FTLCLN;
	for(i = 1; i < argc; i++)
		if(argv[i][0] == '-' && (c=argv[i][1])) {
			p = &argv[i][2];
			testmore = 0;
			switch (c) {

			case 'p':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				chksid(sid_ab(p,&sid),&sid);
				break;
			case 'c':
				clist = p;
				break;
			case 'o':
				testmore++;
				break;
			case 's':
				testmore++;
				break;
			default:
				fatal("unknown key letter (cm1)");
			}

			if (testmore) {
				testmore = 0;
				if (*p) {
					sprintf(Error,
					  "value after %c arg (cm7)",c);
					fatal(Error);
				}
			}
			if (had[c - 'a']++)
				fatal("key letter twice (cm2)");
			argv[i] = 0;
		}
		else num_files++;

	if(num_files == 0)
		fatal("missing file arg (cm3)");
	if (HADP && HADC)
		fatal("can't have both -p and -c (cb2)");
	setsig();
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	iop = stdout;
	for (i = 1; i < argc; i++)
		if (p=argv[i])
			do_file(p,comb);
	fclose(iop);
	exit(Fcnt ? 1 : 0);
}

void
comb(file)
char *file;
{
	register int i, n;
	register struct idel *rdp;
	struct idel *dodelt();
	char *p;
	char *auxf();
	char rarg[32], *sid_ba();
	int succnt;
	struct sid *sp, *prtget();
	extern char had_dir, had_standinp;
	extern char *Sflags[];
	struct stats stats;

	if (setjmp(Fjmp))
		return;
	sinit(&gpkt, file, 1);
	gpkt.p_verbose = -1;
	gpkt.p_stdout = stderr;
	if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		fprintf(gpkt.p_stdout,"\n%s:\n",gpkt.p_file);
	if (exists(auxf(gpkt.p_file, 'p')))
		fatal("p-file exists (cb1)");

	if (dodelt(&gpkt,&stats,(struct sid *) 0,0) == 0)
		fmterr(&gpkt);

	Cvec = (int *) fmalloc((unsigned) (n = ((maxser(&gpkt) + 1) * sizeof(*Cvec))));
	zero((char *) Cvec, n);
	Cnt = 0;

	if (HADP) {
		rdp = gpkt.p_idel;
		if (!(n = sidtoser(&sid, &gpkt)))
			fatal("sid doesn't exist (cb3)");
		while (n <= maxser(&gpkt)) {
			if (rdp[n].i_sid.s_rel == 0 &&
			    rdp[n].i_sid.s_lev == 0 &&
			    rdp[n].i_sid.s_br == 0  &&
			    rdp[n].i_sid.s_seq == 0) {
				n++;
				continue;
			}
			Cvec[Cnt++] = n++;
		}
	}
	else if (HADC) {
		dolist(&gpkt, clist, 0);
	}
	else {
		rdp = gpkt.p_idel;
		for (i = 1; i <= maxser(&gpkt); i++) {
			succnt = 0;
			if (rdp[i].i_sid.s_rel == 0 &&
			    rdp[i].i_sid.s_lev == 0 &&
			    rdp[i].i_sid.s_br == 0  &&
			    rdp[i].i_sid.s_seq == 0)
				continue;
			for (n = i + 1; n <= maxser(&gpkt); n++)
				if (rdp[n].i_pred == i)
					succnt++;
			if (succnt != 1)
				Cvec[Cnt++] = i;
		}
	}
	finduser(&gpkt);
	doflags(&gpkt);
	fclose(gpkt.p_iop);
	gpkt.p_iop = 0;
	if (!Cnt)
		fatal("nothing to do (cb4)");
	rdp = gpkt.p_idel;
	Do_prs = 0;
	fprintf(iop,"trap \"rm -f COMB$$ comb$$ s.COMB$$; exit 2\" 1 2 3 15\n");
	sp = prtget(rdp, Cvec[0], iop, gpkt.p_file);
	sid_ba(sp,rarg);
	if (!(Val_ptr = Sflags[VALFLAG - 'a']))
		Val_ptr = Blank;
	fprintf(iop, "admin -iCOMB$$ -r%s -fv%s -m '-yThis was COMBined' s.COMB$$\n", rarg,Val_ptr);
	Do_prs = 1;
	fprintf(iop, "rm -f COMB$$\n");
	for (i = 1; i < Cnt; i++) {
		n = getpred(rdp, Cvec, i);
		if (HADO)
			fprintf(iop, "get -s -r%d -g -e -t s.COMB$$\n",
				rdp[Cvec[i]].i_sid.s_rel);
		else
			fprintf(iop, "get -s -a%d -r%d -g -e s.COMB$$\n",
				n + 1, rdp[Cvec[i]].i_sid.s_rel);
		prtget(rdp, Cvec[i], iop, gpkt.p_file);
		fprintf(iop, "delta -s -m\"$b\" -y\"$a\" s.COMB$$\n");
	}
	fprintf(iop, "sed -n '/^%c%c$/,/^%c%c$/p' %s >comb$$\n",
		CTLCHAR, BUSERTXT, CTLCHAR, EUSERTXT, gpkt.p_file);
	fprintf(iop, "ed - comb$$ <<\\!\n");
	fprintf(iop, "1d\n");
	fprintf(iop, "$c\n");
	fprintf(iop, "*** DELTA TABLE PRIOR TO COMBINE ***\n");
	fprintf(iop, ".\n");
	fprintf(iop, "w\n");
	fprintf(iop, "q\n");
	fprintf(iop, "!\n");
	fprintf(iop, "prs -e %s >>comb$$\n", gpkt.p_file);
	fprintf(iop, "admin -tcomb$$ s.COMB$$\\\n");
	for (i = 0; i < NFLAGS; i++)
		if (p = Sflags[i])
			fprintf(iop, " -f%c%s\\\n", i + 'a', p);
	fprintf(iop, "\n");
	fprintf(iop, "sed -n '/^%c%c$/,/^%c%c$/p' %s >comb$$\n",
		CTLCHAR, BUSERNAM, CTLCHAR, EUSERNAM, gpkt.p_file);
	fprintf(iop, "ed - comb$$ <<\\!\n");
	fprintf(iop, "v/^%c/s/.*/ -a& \\\\/\n", CTLCHAR);
	fprintf(iop, "1c\n");
	fprintf(iop, "admin s.COMB$$\\\n");
	fprintf(iop, ".\n");
	fprintf(iop, "$c\n");
	fprintf(iop, "\n");
	fprintf(iop, ".\n");
	fprintf(iop, "w\n");
	fprintf(iop, "q\n");
	fprintf(iop, "!\n");
	fprintf(iop, ". comb$$\n");
	fprintf(iop, "rm comb$$\n");
	if (!HADS) {
		fprintf(iop, "rm -f %s\n", gpkt.p_file);
		fprintf(iop, "mv s.COMB$$ %s\n", gpkt.p_file);
		if (!Sflags[VALFLAG - 'a'])
			fprintf(iop, "admin -dv %s\n", gpkt.p_file);
	}
	else {
		fprintf(iop, "set `ls -st s.COMB$$ %s`\n",gpkt.p_file);
		fprintf(iop, "c=`expr 100 - 100 '*' $1 / $3`\n");
		fprintf(iop, "echo '%s\t' ${c}'%%\t' $1/$3\n", gpkt.p_file);
		fprintf(iop, "rm -f s.COMB$$\n");
	}
}

void
enter(pkt,ch,n,sidp)
struct packet *pkt;
char ch;
int n;
struct sid *sidp;
{
	Cvec[Cnt++] = n;
}


struct sid *
prtget(idp, ser, fptr, file)
struct idel *idp;
int ser;
FILE *fptr;
char *file;
{
	char buf[32], *sid_ba();
	struct sid *sp;

	sid_ba(sp = &idp[ser].i_sid, buf);
	fprintf(fptr, "get -s -k -r%s -p %s > COMB$$\n", buf, file);
	if (Do_prs) {
		fprintf(fptr, "a=`prs -r%s -d:C: %s`\n",buf,file);
		fprintf(fptr, "b=`prs -r%s -d:MR: %s`\n",buf,file);
	}
	return(sp);
}


getpred(idp, vec, i)
struct idel *idp;
int *vec;
int i;
{
	int ser, pred, acpred;

	ser = vec[i];
	while (--i) {
		pred = vec[i];
		for (acpred = idp[ser].i_pred; acpred; acpred = idp[acpred].i_pred)
			if (pred == acpred)
				break;
		if (pred == acpred)
			break;
	}
	return(i);
}

void
clean_up()
{
	ffreeall();
}

void
escdodelt()	/* dummy for dodelt() */
{
}

void
fredck() /*dummy for dodelt() */
{
}
