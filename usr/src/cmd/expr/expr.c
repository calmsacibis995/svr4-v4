/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)expr:expr.c	1.22.1.1"
# define A_STRING 258
# define NOARG 259
# define OR 260
# define AND 261
# define EQ 262
# define LT 263
# define GT 264
# define GEQ 265
# define LEQ 266
# define NEQ 267
# define ADD 268
# define SUBT 269
# define MULT 270
# define DIV 271
# define REM 272
# define MCH 273
# define MATCH 274
/* Enhanced Application Compatibility */
# define SUBSTR 275
# define LENGTH 276
# define INDEX 277
/* End Enhanced Application Compatibility */

/* size of subexpression array */
#define MSIZE	512
#define error(c)	errxx()
#define EQL(x,y) !strcmp(x,y)

#define ERROR(c)	errxx()
#include  <stdlib.h>
#include  <regexpr.h>
#include  <locale.h>


long atol();
char *ltoa(), *strcpy(), *strncpy();
void exit();
char	**Av;
char *buf;
int	Ac;
int	Argi;
int noarg;
int paren;
/*	
 *	Array used to store subexpressions in regular expressions
 *	Only one subexpression allowed per regular expression currently 
 */
char Mstring[1][MSIZE];


char *operator[] = { 
	"|", "&", "+", "-", "*", "/", "%", ":",
	"=", "==", "<", "<=", ">", ">=", "!=",
	"match", 
	/* Enhanced Application Compatibility */
	"substr", "length", "index",
	/* End Enhanced Application Compatibility */
	"\0" };
int op[] = { 
	OR, AND, ADD,  SUBT, MULT, DIV, REM, MCH,
	EQ, EQ, LT, LEQ, GT, GEQ, NEQ,
	MATCH,
	/* Enhanced Application Compatibility */
	SUBSTR, LENGTH, INDEX
	/* End Enhanced Application Compatibility */
 	};
int pri[] = {
	1,2,3,3,3,3,3,3,4,4,5,5,5,6,7,
	/* Enhanced Application Compatibility */
	8,9,9
	/* Enhanced Application Compatibility */
	};

yylex() {
	register char *p;
	register i;

	if(Argi >= Ac) return NOARG;

	p = Av[Argi];

	if((*p == '(' || *p == ')') && p[1] == '\0' )
		return (int)*p;
	for(i = 0; *operator[i]; ++i)
		if(EQL(operator[i], p))
			return op[i];


	return A_STRING;
}

char *rel(oper, r1, r2) register char *r1, *r2; 
{
	long i;

	if(ematch(r1, "-\\{0,1\\}[0-9]*$") && ematch(r2, "-\\{0,1\\}[0-9]*$"))
		i = atol(r1) - atol(r2);
	else
		i = strcmp(r1, r2);
	switch(oper) {
	case EQ: 
		i = i==0; 
		break;
	case GT: 
		i = i>0; 
		break;
	case GEQ: 
		i = i>=0; 
		break;
	case LT: 
		i = i<0; 
		break;
	case LEQ: 
		i = i<=0; 
		break;
	case NEQ: 
		i = i!=0; 
		break;
	}
	return i? "1": "0";
}

char *arith(oper, r1, r2) char *r1, *r2; 
{
	long i1, i2;
	register char *rv;

	if(!(ematch(r1, "-\\{0,1\\}[0-9]*$") && ematch(r2, "-\\{0,1\\}[0-9]*$")))
		yyerror("non-numeric argument");
	i1 = atol(r1);
	i2 = atol(r2);

	switch(oper) {
	case ADD: 
		i1 = i1 + i2; 
		break;
	case SUBT: 
		i1 = i1 - i2; 
		break;
	case MULT: 
		i1 = i1 * i2; 
		break;
	case DIV: 
		if (i2 == 0)
			yyerror("division by zero");
		i1 = i1 / i2; 
		break;
	case REM: 
		if (i2 == 0)
			yyerror("division by zero");
		i1 = i1 % i2; 
		break;
	}
	rv = malloc(16);
	(void) strcpy(rv, ltoa(i1));
	return rv;
}
char *conj(oper, r1, r2) char *r1, *r2; 
{
	register char *rv;

	switch(oper) {

	case OR:
		if(EQL(r1, "0")
		    || EQL(r1, ""))
			if(EQL(r2, "0")
			    || EQL(r2, ""))
				rv = "0";
			else
				rv = r2;
		else
			rv = r1;
		break;
	case AND:
		if(EQL(r1, "0")
		    || EQL(r1, ""))
			rv = "0";
		else if(EQL(r2, "0")
		    || EQL(r2, ""))
			rv = "0";
		else
			rv = r1;
		break;
	}
	return rv;
}

/* Enhanced Application Compatibility */
char *
substr(v, s, w)
char *v, *s, *w;
{
	register si, wi;
	register char *res;

	si = atol(s);
	wi = atol(w);
	while(--si) if(*v) ++v;

	res = v;
	
	while(wi--) if(*v) ++v;
	*v ='\0';
	return res;	
}

char *
length(s)
register char *s;
{
	register i = 0;
	register char *rv;

	while(*s++) ++i;

	rv = malloc(8);
	strcpy(rv, ltoa((long)i));
	return rv;
}

char *
index(s, t)
char *s, *t;
{
	register i, j;
	register char *rv;

	for(i = 0; s[i]; ++i)
		for(j = 0; t[j]; ++j)
			if(s[i]==t[j]) {
				strcpy(rv=malloc(8), ltoa((long)++i));
				return rv;
			}
	return "0";
}

/* End Enhanced Application Compatibility */

char *match(s, p)
char *s, *p;
{
	register char *rv;

	(void) strcpy(rv=malloc(8), ltoa((long)ematch(s, p)));
	if(nbra) {
		rv = malloc((unsigned) strlen(Mstring[0]) + 1);
		(void) strcpy(rv, Mstring[0]);
	}
	return rv;
}

ematch(s, p)
char *s;
register char *p;
{
	static char *expbuf;
	char *nexpbuf;
	register num;
	extern char *braslist[], *braelist[], *loc2;

	nexpbuf = compile(p, (char *)0, (char *)0);
	if(nbra > 1)
		yyerror("Too many '\\('s");
	if(regerrno) {
		if (regerrno != 41 || expbuf == NULL)
			errxx();
	} else {
		if (expbuf)
			free(expbuf);
		expbuf = nexpbuf;
	}
	if(advance(s, expbuf)) {
		if(nbra == 1) {
			p = braslist[0];
			num = braelist[0] - p;
			if ((num > MSIZE - 1) || (num < 0)) yyerror("string too long");
			(void) strncpy(Mstring[0], p, num);
			Mstring[0][num] = '\0';
		}
		return(loc2-s);
	}
	return(0);
}

errxx()
{
	yyerror("RE error");
}

yyerror(s)
char *s;
{
	(void) write(2, "expr: ", 6);
	(void) write(2, s, (unsigned) strlen(s));
	(void) write(2, "\n", 1);
	exit(2);
}

char *ltoa(l)
long l;
{
	static str[20];
	register char *sp = (char *) &str[18];	/*u370*/
	register i;
	register neg = 0;

	if(l == 0x80000000L)
		return "-2147483648";
	if(l < 0)
		++neg, l = -l;
	str[19] = '\0';
	do {
		i = l % 10;
		*sp-- = '0' + i;
		l /= 10;
	} 
	while(l);
	if(neg)
		*sp-- = '-';
	return ++sp;
}
char *expres(prior,par)  int prior, par; 
{
	int ylex, temp, op1;
	char *r1, *ra, *rb, *rc;
	ylex = yylex();
	if (ylex >= NOARG && ylex < MATCH) {
		yyerror("syntax error");
	}
	if (ylex == A_STRING) {
		r1 = Av[Argi++];
		temp = Argi;
	}
	else {
		if (ylex == '(') {
			paren++;
			Argi++;
			r1 = expres(0,Argi);
			Argi--;
		}
	}
lop:
	ylex = yylex();
	if (ylex > NOARG && ylex < MATCH) {
		op1 = ylex;
		Argi++;
		if (pri[op1-OR] <= prior ) 
			return r1;
		else {
			switch(op1) {
			case OR:
			case AND:
				r1 = conj(op1,r1,expres(pri[op1-OR],0));
				break;
			case EQ:
			case LT:
			case GT:
			case LEQ:
			case GEQ:
			case NEQ:
				r1=rel(op1,r1,expres(pri[op1-OR],0));
				break;
			case ADD:
			case SUBT:
			case MULT:
			case DIV:
			case REM:
				r1=arith(op1,r1,expres(pri[op1-OR],0));
				break;
			case MCH:
				r1=match(r1,expres(pri[op1-OR],0));
				break;
			}
			if(noarg == 1) {
				return r1;
			}
			Argi--;
			goto lop;
		}
	}
	ylex = yylex();
	if(ylex == ')') {
		if(par == Argi) {
			yyerror("syntax error");
		}
		if(par != 0) {
			paren--;
			Argi++;
		}
		Argi++;
		return r1;
	}
	ylex = yylex();
	if(ylex > MCH && ylex <= INDEX) {
		if (Argi == temp) {
			return r1;
		}
		op1 = ylex;
		Argi++;
		switch(op1) {
	/* Enhanced Application Compatibility */
		case SUBSTR:
			rc = expres(pri[op1-OR],0);
		case MATCH:
		case INDEX:
			rb = expres(pri[op1-OR],0);
		case LENGTH:
			ra = expres(pri[op1-OR],0);
	/* End Enhanced Application Compatibility */
		}

		switch(op1) {
		case MATCH: 
			r1 = match(rb,ra); 
			break;
	/* Enhanced Application Compatibility */
		case INDEX:
			r1 = index(rb,ra); 
			break;
		case SUBSTR:
			r1 = substr(rc,rb,ra); 
			break;
		case LENGTH:
			r1 = length(ra); 
			break;
	/* End Enhanced Application Compatibility */
		}
		if(noarg == 1) {
			return r1;
		}
		Argi--;
		goto lop;
	}
	ylex = yylex();
	if (ylex == NOARG) {
		noarg = 1;
	}
	return r1;
}
main(argc, argv) char **argv; 
{
	Ac = argc;
	Argi = 1;
	noarg = 0;
	paren = 0;
	Av = argv;
	(void)setlocale(LC_ALL, "");
	buf = expres(0,1);
	if(Ac != Argi || paren != 0) {
		yyerror("syntax error");
	}
	(void) write(1, buf, (unsigned) strlen(buf));
	(void) write(1, "\n", 1);
	exit((!strcmp(buf, "0") || !buf[0])? 1: 0);
	/* NOTREACHED */
}
