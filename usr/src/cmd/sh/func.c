/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sh:func.c	1.4.3.1"

/*
 * UNIX shell
 */

#include	"defs.h"

freefunc(n)
	struct namnod 	*n;
{
	freetree((struct trenod *)(n->namenv));
}


freetree(t)
	register struct trenod *t;
{
	if (t)
	{
		register int type;

		if (t->tretyp & CNTMSK)
		{
			t->tretyp--;
			return;
		}

		type = t->tretyp & COMMSK;

		switch (type)
		{
			case TFND:
				free(fndptr(t)->fndnam);
				freetree(fndptr(t)->fndval);
				break;

			case TCOM:
				freeio(comptr(t)->comio);
				free_arg(comptr(t)->comarg);
				free_arg(comptr(t)->comset);
				break;

			case TFORK:
				freeio(forkptr(t)->forkio);
				freetree(forkptr(t)->forktre);
				break;

			case TPAR:
				freetree(parptr(t)->partre);
				break;

			case TFIL:
			case TLST:
			case TAND:
			case TORF:
				freetree(lstptr(t)->lstlef);
				freetree(lstptr(t)->lstrit);
				break;

			case TFOR:
			{
				struct fornod *f = (struct fornod *)t;

				free(f->fornam);
				freetree(f->fortre);
				if (f->forlst)
				{
					freeio(f->forlst->comio);
					free_arg(f->forlst->comarg);
					free_arg(f->forlst->comset);
					free(f->forlst);
				}
			}
			break;

			case TWH:
			case TUN:
				freetree(whptr(t)->whtre);
				freetree(whptr(t)->dotre);
				break;

			case TIF:
				freetree(ifptr(t)->iftre);
				freetree(ifptr(t)->thtre);
				freetree(ifptr(t)->eltre);
				break;

			case TSW:
				free(swptr(t)->swarg);
				freereg(swptr(t)->swlst);
				break;
		}
		free(t);
	}
}

free_arg(argp)
	register struct argnod 	*argp;
{
	register struct argnod 	*sav;

	while (argp)
	{
		sav = argp->argnxt;
		free(argp);
		argp = sav;
	}
}


freeio(iop)
	register struct ionod *iop;
{
	register struct ionod *sav;

	while (iop)
	{
		if (iop->iofile & IODOC)
		{

#ifdef DEBUG
			prs("unlinking ");
			prs(iop->ioname);
			newline();
#endif

			unlink(iop->ioname);

			if (fiotemp == iop)
				fiotemp = iop->iolst;
			else
			{
				struct ionod *fiop = fiotemp;

				while (fiop->iolst != iop)
					fiop = fiop->iolst;
	
				fiop->iolst = iop->iolst;
			}
		}
		free(iop->ioname);
		free(iop->iolink);
		sav = iop->ionxt;
		free(iop);
		iop = sav;
	}
}


freereg(regp)
	register struct regnod 	*regp;
{
	register struct regnod 	*sav;

	while (regp)
	{
		free_arg(regp->regptr);
		freetree(regp->regcom);
		sav = regp->regnxt;
		free(regp);
		regp = sav;
	}
}


static nonl = 0;

prbgnlst()
{
	if (nonl)
		prc_buff(SP);
	else
		prc_buff(NL);
}

prendlst()
{
	if (nonl) {
		prc_buff(';');
		prc_buff(SP);
	}
	else
		prc_buff(NL);
}

prcmd(t)
{
	nonl++;
	prf(t);
	nonl = 0;
}

prf(t)
	register struct trenod	*t;
{
	sigchk();

	if (t)
	{
		register int	type;

		type = t->tretyp & COMMSK;

		switch(type)
		{
			case TFND:
			{
				register struct fndnod *f = (struct fndnod *)t;

				prs_buff(f->fndnam);
				prs_buff("(){");
				prbgnlst();
				prf(f->fndval);
				prbgnlst();
				prs_buff("}");
				break;
			}

			case TCOM:
				if (comptr(t)->comset) {
					prarg(comptr(t)->comset);
					prc_buff(SP);
				}
				prarg(comptr(t)->comarg);
				prio(comptr(t)->comio);
				break;

			case TFORK:
				prf(forkptr(t)->forktre);
				prio(forkptr(t)->forkio);
				if (forkptr(t)->forktyp & FAMP)
					prs_buff(" &");
				break;

			case TPAR:
				prs_buff("(");
				prf(parptr(t)->partre);
				prs_buff(")");
				break;

			case TFIL:
				prf(lstptr(t)->lstlef);
				prs_buff(" | ");
				prf(lstptr(t)->lstrit);
				break;

			case TLST:
				prf(lstptr(t)->lstlef);
				prendlst();
				prf(lstptr(t)->lstrit);
				break;

			case TAND:
				prf(lstptr(t)->lstlef);
				prs_buff(" && ");
				prf(lstptr(t)->lstrit);
				break;

			case TORF:
				prf(lstptr(t)->lstlef);
				prs_buff(" || ");
				prf(lstptr(t)->lstrit);
				break;

			case TFOR:
				{
					register struct argnod	*arg;
					register struct fornod 	*f = (struct fornod *)t;

					prs_buff("for ");
					prs_buff(f->fornam);

					if (f->forlst)
					{
						arg = f->forlst->comarg;
						prs_buff(" in");

						while(arg != ENDARGS)
						{
							prc_buff(SP);
							prs_buff(arg->argval);
							arg = arg->argnxt;
						}
					}

					prendlst();
					prs_buff("do");
					prbgnlst();
					prf(f->fortre);
					prendlst();
					prs_buff("done");
				}
				break;

			case TWH:
			case TUN:
				if (type == TWH)
					prs_buff("while ");
				else
					prs_buff("until ");
				prf(whptr(t)->whtre);
				prendlst();
				prs_buff("do");
				prbgnlst();
				prf(whptr(t)->dotre);
				prendlst();
				prs_buff("done");
				break;

			case TIF:
			{
				struct ifnod *f = (struct ifnod *)t;

				prs_buff("if ");
				prf(f->iftre);
				prendlst();
				prs_buff("then");
				prendlst();
				prf(f->thtre);

				if (f->eltre)
				{
					prendlst();
					prs_buff("else");
					prendlst();
					prf(f->eltre);
				}

				prendlst();
				prs_buff("fi");
				break;
			}

			case TSW:
				{
					register struct regnod 	*swl;

					prs_buff("case ");
					prs_buff(swptr(t)->swarg);

					swl = swptr(t)->swlst;
					while(swl)
					{
						struct argnod	*arg = swl->regptr;

						if (arg)
						{
							prs_buff(arg->argval);
							arg = arg->argnxt;
						}

						while(arg)
						{
							prs_buff(" | ");
							prs_buff(arg->argval);
							arg = arg->argnxt;
						}

						prs_buff(")");
						prf(swl->regcom);
						prs_buff(";;");
						swl = swl->regnxt;
					}
				}
				break;
			} 
		} 

	sigchk();
}

prarg(argp)
	register struct argnod	*argp;
{
	while (argp)
	{
		prs_buff(argp->argval);
		argp=argp->argnxt;
		if (argp)
			prc_buff(SP);
	}
}


prio(iop)
	register struct ionod	*iop;
{
	register int	iof;
	register unsigned char	*ion;

	while (iop)
	{
		iof = iop->iofile;
		ion = iop->ioname;

		if (*ion)
		{
			prc_buff(SP);

			prn_buff(iof & IOUFD);

			if (iof & IODOC)
				prs_buff("<<");
			else if (iof & IOMOV)
			{
				if (iof & IOPUT)
					prs_buff(">&");
				else
					prs_buff("<&");

			}
			else if ((iof & IOPUT) == 0)
				prc_buff('<');
			else if (iof & IOAPP)
				prs_buff(">>");
			else
				prc_buff('>');

			prs_buff(ion);
		}
		iop = iop->ionxt;
	}
}
