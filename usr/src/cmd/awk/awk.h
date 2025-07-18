/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)awk:awk.h	2.13"

typedef double	Awkfloat;
typedef	unsigned char uchar;

#define	xfree(a)	{ if ((a) != NULL) { free(a); a = NULL; } }

#define	DEBUG
#ifdef	DEBUG
			/* uses have to be doubly parenthesized */
#	define	dprintf(x)	if (dbg) printf x
#else
#	define	dprintf(x)
#endif

extern	char	errbuf[200];
#define	ERROR	sprintf(errbuf,
#define	FATAL	), error(1, errbuf)
#define	WARNING	), error(0, errbuf)
#define	SYNTAX	), yyerror(errbuf)

extern int	compile_time;	/* 1 if compiling, 0 if running */

#define	RECSIZE	(3 * 1024)	/* sets limit on records, fields, etc., etc. */

extern uchar	**FS;
extern uchar	**RS;
extern uchar	**ORS;
extern uchar	**OFS;
extern uchar	**OFMT;
extern Awkfloat *NR;
extern Awkfloat *FNR;
extern Awkfloat *NF;
extern uchar	**FILENAME;
extern uchar	**SUBSEP;
extern Awkfloat *RSTART;
extern Awkfloat *RLENGTH;

extern uchar	*record;
extern int	dbg;
extern int	lineno;
extern int	errorflag;
extern int	donefld;	/* 1 if record broken into fields */
extern int	donerec;	/* 1 if record is valid (no fld has changed */

#define	CBUFLEN	400
extern uchar	cbuf[CBUFLEN];	/* miscellaneous character collection */

extern	uchar	*patbeg;	/* beginning of pattern matched */
extern	int	patlen;		/* length.  set in b.c */

/* Cell:  all information about a variable or constant */

typedef struct Cell {
	uchar	ctype;		/* OCELL, OBOOL, OJUMP, etc. */
	uchar	csub;		/* CCON, CTEMP, CFLD, etc. */
	uchar	*nval;		/* name, for variables only */
	uchar	*sval;		/* string value */
	Awkfloat fval;		/* value as number */
	unsigned tval;		/* type info: STR|NUM|ARR|FCN|FLD|CON|DONTFREE */
	struct Cell *cnext;	/* ptr to next if chained */
} Cell;

typedef struct {		/* symbol table array */
	int	nelem;		/* elements in table right now */
	int	size;		/* size of tab */
	Cell	**tab;		/* hash table pointers */
} Array;

#define	NSYMTAB	50	/* initial size of a symbol table */
extern Array	*symtab, *makesymtab();
extern Cell	*setsymtab(), *lookup();

extern Cell	*recloc;	/* location of input record */
extern Cell	*nrloc;		/* NR */
extern Cell	*fnrloc;	/* FNR */
extern Cell	*nfloc;		/* NF */
extern Cell	*rstartloc;	/* RSTART */
extern Cell	*rlengthloc;	/* RLENGTH */

/* Cell.tval values: */
#define	NUM	01	/* number value is valid */
#define	STR	02	/* string value is valid */
#define DONTFREE 04	/* string space is not freeable */
#define	CON	010	/* this is a constant */
#define	ARR	020	/* this is an array */
#define	FCN	040	/* this is a function name */
#define FLD	0100	/* this is a field $1, $2, ... */
#define	REC	0200	/* this is $0 */

#define freeable(p)	(!((p)->tval & DONTFREE))

Awkfloat setfval(), getfval();
uchar	*setsval(), *getsval();
uchar	*tostring(), *tokname(), *qstring();

#include <malloc.h>

double	log(), sqrt(), exp(), atof();

/* function types */
#define	FLENGTH	1
#define	FSQRT	2
#define	FEXP	3
#define	FLOG	4
#define	FINT	5
#define	FSYSTEM	6
#define	FRAND	7
#define	FSRAND	8
#define	FSIN	9
#define	FCOS	10
#define	FATAN	11
#define	FTOUPPER 12
#define	FTOLOWER 13

/* Node:  parse tree is made of nodes, with Cell's at bottom */

typedef struct Node {
	int	ntype;
	struct	Node *nnext;
	int	lineno;
	int	nobj;
	struct Node *narg[1];	/* variable: actual size set by calling malloc */
} Node;

#define	NIL	((Node *) 0)

extern Node	*winner;
extern Node	*nullstat;
extern Node	*nullnode;

/* ctypes */
#define OCELL	1
#define OBOOL	2
#define OJUMP	3

/* Cell subtypes: csub */
#define	CFREE	7
#define CCOPY	6
#define CCON	5
#define CTEMP	4
#define CNAME	3 
#define CVAR	2
#define CFLD	1

/* bool subtypes */
#define BTRUE	11
#define BFALSE	12

/* jump subtypes */
#define JEXIT	21
#define JNEXT	22
#define	JBREAK	23
#define	JCONT	24
#define	JRET	25

/* node types */
#define NVALUE	1
#define NSTAT	2
#define NEXPR	3
#define	NFIELD	4

extern	Cell	*(*proctab[])();
extern	Cell	*nullproc();
extern	int	pairstack[], paircnt;
extern	Cell	*fieldadr();

extern	Node	*stat1(), *stat2(), *stat3(), *stat4(), *pa2stat();
extern	Node	*op1(), *op2(), *op3(), *op4();
extern	Node	*linkum(), *valtonode(), *rectonode(), *exptostat();
extern	Node	*makearr();

#define notlegal(n)	(n <= FIRSTTOKEN || n >= LASTTOKEN || proctab[n-FIRSTTOKEN] == nullproc)
#define isvalue(n)	((n)->ntype == NVALUE)
#define isexpr(n)	((n)->ntype == NEXPR)
#define isjump(n)	((n)->ctype == OJUMP)
#define isexit(n)	((n)->csub == JEXIT)
#define	isbreak(n)	((n)->csub == JBREAK)
#define	iscont(n)	((n)->csub == JCONT)
#define	isnext(n)	((n)->csub == JNEXT)
#define	isret(n)	((n)->csub == JRET)
#define isstr(n)	((n)->tval & STR)
#define isnum(n)	((n)->tval & NUM)
#define isarr(n)	((n)->tval & ARR)
#define isfunc(n)	((n)->tval & FCN)
#define istrue(n)	((n)->csub == BTRUE)
#define istemp(n)	((n)->csub == CTEMP)

#define NCHARS	(128+1)	/* could be 256+1 */
#define NSTATES	32

typedef struct rrow {
	int	ltype;
	int	lval;
	int	*lfollow;
} rrow;

typedef struct fa {
	uchar	*restr;
	int	anchor;
	int	use;
	uchar	gototab[NSTATES][NCHARS];
	int	*posns[NSTATES];
	uchar	out[NSTATES];
	int	initstat;
	int	curstat;
	int	accept;
	int	reset;
	struct	rrow re[1];
} fa;

extern	fa	*makedfa();
