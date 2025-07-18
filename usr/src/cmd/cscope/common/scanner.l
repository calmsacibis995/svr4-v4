%{
/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
%}
%{
/*	Copyright (c) 1988 AT&T	*/
%}
%{
/*	  All Rights Reserved  	*/
%}

%{
/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
%}
%{
/*	UNIX System Laboratories, Inc.                     	*/
%}
%{
/*	The copyright notice above does not evidence any   	*/
%}
%{
/*	actual or intended publication of such source code.	*/
%}

%{
#ident	"@(#)cscope:common/scanner.l	1.3"
/*	cscope - interactive C symbol cross-reference
 *
 *	C symbol scanner
 */
#include "global.h"

/* the line counting has been moved from character reading for speed */
/* comments are discarded */
#undef	input
#define input()  ((yytchar=(yytchar=yysptr>yysbuf?*--yysptr:getc(yyin))=='/'?comment():yytchar)==EOF?0:yytchar)
#define noncommentinput()  ((yytchar=yysptr>yysbuf?*--yysptr:getc(yyin))==EOF?0:yytchar)
#undef	unput
#define unput(c) (*yysptr++=(c))

#define	IFLEVELINC	5	/* #if nesting level size increment */

/* keyword text for fast testing of keywords in the scanner */
extern	char	enumtext[];
extern	char	externtext[];
extern	char	structtext[];
extern	char	typedeftext[];
extern	char	uniontext[];

int	first;	/* buffer index for first char of symbol */
int	last;	/* buffer index for last char of symbol */
int	lineno;	/* symbol line number */

static	BOOL	arraydimension;		/* inside array dimension declaration */
static	BOOL	bplisting;		/* breakpoint listing */
static	int	braces;			/* unmatched left brace count */
static	BOOL	classdef;		/* c++ class definition */
static	BOOL	elseelif;		/* #else or #elif found */
static	BOOL	esudef;			/* enum/struct/union global definition */
static	BOOL	external;		/* external definition */
static	int	externalbraces;		/* external definition outer brace count */
static	BOOL	fcndef;			/* function definition */
static	BOOL	global;			/* file global scope (outside functions) */
static	int	iflevel;		/* #if nesting level */
static	BOOL	initializer;		/* data initializer */
static	int	initializerbraces;	/* data initializer outer brace count */
static	BOOL	lex;			/* lex file */
static	int	miflevel = IFLEVELINC;	/* maximum #if nesting level */
static	int	*maxifbraces;		/* maximum brace count within #if */
static	int	*preifbraces;		/* brace count before #if */
static	int	parens;			/* unmatched left parenthesis count */
static	BOOL	ppdefine;		/* preprocessor define statement */
static	BOOL	psuedoelif;		/* psuedo-#elif */
static	BOOL	oldtype;		/* next identifier is an old type */
static	BOOL	rules;			/* lex/yacc rules */
static	BOOL	sdl;			/* sdl file */
static	BOOL	structfield;		/* structure field declaration */
static	int	tagdef;			/* class/enum/struct/union tag definition */
static	BOOL	template;		/* function template */
static	int	templateparens;		/* function template outer parentheses count */
static	int	typedefbraces = -1;	/* initial typedef brace count */
static	int	token;			/* token found */

void	multicharconstant();
%}
identifier	[a-zA-Z_][a-zA-Z_0-9]*
number		\.?[0-9][.0-9a-fA-FlLuUxX]*
%start SDL
%a 4000
%o 7000
%%
%\{		{	/* lex/yacc C declarations/definitions */
			global = YES;
			goto more;
			/* NOTREACHED */
		}
%\}		{
			global = NO;
			goto more;
			/* NOTREACHED */
		}
^%%		{	/* lex/yacc rules delimiter */
			braces = 0;
			if (rules == NO) {
				rules = YES;
				
				/* simulate a yylex() or yyparse() definition */
				(void) strcat(yytext, " /* ");
				first = strlen(yytext);
				if (lex == YES) {
					(void) strcat(yytext, "yylex");
				}
				else {	/* yacc: yyparse implicitly calls yylex */
					char *s = " yylex()";
					char *cp = s + strlen(s);
					while (--cp >= s) {
						unput(*cp);
					}
					(void) strcat(yytext, "yyparse");
				}
				last = strlen(yytext);
				(void) strcat(yytext, " */");
				yyleng = strlen(yytext);
				yymore();
				return(FCNDEF);
			}
			else {
				rules = NO;
				global = YES;
				last = first;
				yymore();
				return(FCNEND);
				/* NOTREACHED */
			}
		}
<SDL>STATE[ \t]+({identifier}|\*)	{ /* sdl state, treat as function def */
			braces = 1;
			fcndef = YES;
			token = FCNDEF;
			goto findident;
			/* NOTREACHED */
		}
<SDL>ENDSTATE[ \t]	{ /* end of an sdl state, treat as end of a function */
			goto endstate;
			/* NOTREACHED */
		}
\{		{	/* count unmatched left braces for fcn def detection */
			++braces;
			
			/* mark an untagged enum/struct/union so its beginning
			   can be found */
			if (tagdef) {
				if (braces == 1) {
					esudef = YES;
				}
				token = tagdef;
				tagdef = '\0';
				last = first;
				yymore();
				return(token);
			}
			goto more;
			/* NOTREACHED */
		}
\#[ \t]*endif/.*\n[ \t\n]*#[ \t]*if	{
			/* attempt to correct erroneous brace count caused by:
			 * 
			 * #if ...
			 * 	... {
			 * #endif
			 * #if ...
			 * 	... {
			 * #endif
			 */
			/* the current #if must not have an #else or #elif */
			if (elseelif == YES) {
				goto endif;
				/* NOTREACHED */
			}
			psuedoelif = YES;
			goto more;
			/* NOTREACHED */
		}
\#[ \t]*ifn?(def)?	{ /* #if, #ifdef or #ifndef */
			elseelif = NO;
			if (psuedoelif == YES) {
				psuedoelif = NO;
				goto elif;
				/* NOTREACHED */
			}
			/* make sure there is room for the current brace count */
			if (iflevel == miflevel) {
				miflevel += IFLEVELINC;
				maxifbraces = (int *) myrealloc(maxifbraces, miflevel * sizeof(int));
				preifbraces = (int *) myrealloc(preifbraces, miflevel * sizeof(int));
			}
			/* push the current brace count */
			preifbraces[iflevel] = braces;
			maxifbraces[iflevel++] = 0;
			goto more;
			/* NOTREACHED */
		}
\#[ \t]*el(se|if)	{ /* #elif or #else */
			elseelif = YES;
		elif:
			if (iflevel > 0) {
				
				/* save the maximum brace count for this #if */
				if (braces > maxifbraces[iflevel]) {
					maxifbraces[iflevel - 1] = braces;
				}
				/* restore the brace count to before the #if */
				braces = preifbraces[iflevel - 1];
			}
			goto more;
			/* NOTREACHED */
		}
\#[ \t]*endif	{	/* #endif */
		endif:
			if (iflevel > 0) {
				
				/* get the maximum brace count for this #if */
				if (braces < maxifbraces[--iflevel]) {
					braces = maxifbraces[iflevel];
				}
			}
			goto more;
			/* NOTREACHED */
		}
\}		{
			/* could be the last enum member initializer */
			if (braces == initializerbraces) {
				initializerbraces = -1;
				initializer = NO;
			}
			if (--braces <= 0) {
		endstate:
				braces = 0;
				classdef = NO;
			}
			if (braces == 0 || braces == 1 && classdef == YES) {

				/* if the end of an enum/struct/union definition */
				if (esudef == YES) {
					esudef = NO;
				}
				/* if the end of the function */
				else if (fcndef == YES) {
					fcndef = NO;
					last = first;
					yymore();
					return(FCNEND);
				}
			}
			goto more;
			/* NOTREACHED */
		}
\(		{	/* count unmatched left parentheses for function templates */
			++parens;
			goto more;
			/* NOTREACHED */
		}
\)		{
			if (--parens <= 0) {
				parens = 0;
			}
			/* if the end of a function template */
			if (parens == templateparens) {
				templateparens = -1;
				template = NO;
			}
			goto more;
			/* NOTREACHED */
		}
=		{	/* if a global definition initializer */
			if (global == YES && ppdefine == NO && yytext[0] != '#') {
				initializerbraces = braces;
				initializer = YES;
			}
			goto more;
			/* NOTREACHED */
		}
:		{	/* a if global structure field */
			if (global == YES && ppdefine == NO && yytext[0] != '#') {
				structfield = YES;
			}
			goto more;
			/* NOTREACHED */
		}
\,		{
			if (braces == initializerbraces) {
				initializerbraces = -1;
				initializer = NO;
			}
			structfield = NO;
			goto more;
			/* NOTREACHED */
		}
;		{	/* if the enum/struct/union was not a definition */
			if (braces == 0) {
				esudef = NO;
			}
			/* if the end of a typedef */
			if (braces == typedefbraces) {
				typedefbraces = -1;
			}
			/* if the end of a external definition */
			if (braces == externalbraces) {
				externalbraces = -1;
				external = NO;
			}
			structfield = NO;
			initializer = NO;
			goto more;
			/* NOTREACHED */
		}
\#[ \t]*define[ \t]+{identifier}	{
				
			/* preprocessor macro or constant definition */
			ppdefine = YES;
			token = DEFINE;
			if (compress == YES) {
				yytext[0] = '\0';	/* compress the keyword */
			}
		findident:
			first = yyleng - 1;
			while (yytext[first] != ' ' && yytext[first] != '\t') {
				--first;
			}
			++first;
			goto fcn;
			/* NOTREACHED */
		}
class[ \t]+{identifier}[ \t\na-zA-Z0-9_():]*\{	{	/* class definition */
			classdef = YES;
			tagdef =  'c';
			REJECT;
			/* NOTREACHED */
		}
(enum|struct|union)/([ \t\n]+{identifier})?[ \t\n]*\{	{ /* enum/struct/union definition */
			tagdef = *(yytext + first);
			goto ident;
			/* NOTREACHED */
		}
{identifier}/[ \t]*\([ \t\na-zA-Z0-9_*&[\]=,.]*\)[ \t\n()]*[:a-zA-Z_#{]	{
			
			/* warning: "if (...)" must not overflow yytext, so the content
			   of function argument definitions is restricted, in particular
			   parentheses are not allowed */

			/* if a function definition */
			/* note: "#define a (b) {" and "#if defined(a)\n#" are not */
			if (braces == 0 && ppdefine == NO && yytext[0] != '#' && rules == NO ||
			    braces == 1 && classdef == YES) {
				fcndef = YES;
				token = FCNDEF;
				goto fcn;
				/* NOTREACHED */
			}
			goto fcncal;
			/* NOTREACHED */
		}
{identifier}/[ \t]*\(	{ /* if a function call */
		fcncal:	if (fcndef == YES || ppdefine == YES || rules == YES) {
				token = FCNCALL;
				goto fcn;
				/* NOTREACHED */
			}
			if (template == NO) {
				templateparens = parens;
				template = YES;
			}
			goto ident;
			/* NOTREACHED */
		}
{identifier}/[* \t\n]+[a-zA-Z0-9_]	{	/* typedef name use */
			goto ident;
			/* NOTREACHED */
		}
{identifier}	{
			char	*s;

			if (global == YES && ppdefine == NO && yytext[0] != '#' &&
			    external == NO && initializer == NO && 
			    arraydimension == NO && structfield == NO &&
			    template == NO && fcndef == NO) {
				if (esudef == YES) {	/* if enum/struct/union */
					token = MEMBERDEF;
				}
				else {
					token = GLOBALDEF;
				}
			}
			else {
		ident:		token = IDENT;
			}
		fcn:
			/* if a long line */
			if (yyleng > STMTMAX) {
				int	c;
				
				/* skip to the end of the line */
				warning("line too long", yylineno);
				while ((c = input()) != LEXEOF) {
					if (c == '\n') {
						unput(c);
						break;
					}
				}
			}
			/* truncate a long symbol */
			if (yyleng - first > PATLEN) {
				warning("symbol too long", yylineno);
				yyleng = first + PATLEN;
				yytext[yyleng] = '\0';
			}
			/* if a keyword */
			yymore();
			if ((s = lookup(yytext + first)) != NULL) {
				first = yyleng;
				
				/* if the start of a typedef */
				if (s == typedeftext) {
					typedefbraces = braces;
					oldtype = YES;
				}
				/* if an enum/struct/union */
				/* (needed for "typedef struct tag name;" so
				   tag isn't marked as the typedef name) */
				else if (s == enumtext || s == structtext || s == uniontext) {
				}
				/* if an external definition */
				else if (s == externtext) {
					externalbraces = braces;
					external = YES;
				}
				/* keyword doesn't start a function template */
				else if (templateparens == parens && template == YES) {
					templateparens = -1;
					template = NO;
				}
				else {	/* next identifier after typedef was a keyword */
					oldtype = NO;
				}
			}
			else {	/* identifier */
				last = yyleng;
				
				/* if a class/enum/struct/union tag definition */
				if (tagdef && strnotequal(yytext + first, "class")) {
					token = tagdef;
					tagdef = '\0';
					if (braces == 0) {
						esudef = YES;
					}
				}
				/* if a typedef name */
				else if (braces == typedefbraces && oldtype == NO &&
				    arraydimension == NO) {
					token = TYPEDEF;
				}
				else {
					oldtype = NO;
				}
				return(token);
				/* NOTREACHED */
			}
		}
\[		{	/* array dimension (don't worry or about subscripts) */
			arraydimension = YES;
			goto more;
			/* NOTREACHED */
		}
\]		{
			arraydimension = NO;
			goto more;
			/* NOTREACHED */
		}
\\\n		{	/* preprocessor statement is continued on next line */
			goto eol;
			/* NOTREACHED */
		}
\n		{	/* end of the line */
			if (ppdefine == YES) {	/* end of a #define */
				ppdefine = NO;
				yyless(yyleng - 1);	/* rescan \n */
				last = first;
				yymore();
				return(DEFINEEND);
			}
			/* skip the first 8 columns of a breakpoint listing line */
			/* and skip the file path in the page header */
			if (bplisting == YES) {
				int	c, i;

				switch (input()) {	/* tab and EOF just fall through */
				case ' ':	/* breakpoint number line */
				case '[':
					for (i = 1; i < 8 && input() != LEXEOF; ++i)
						;
					break;
				case '.':	/* header line */
				case '/':
					/* skip to the end of the line */
					while ((c = input()) != LEXEOF) {
						if (c == '\n') {
							unput(c);
							break;
						}
					}
					break;
				case '\n':	/* empty line */
					unput('\n');
					break;
				}
			}
		eol:
			++yylineno;
			first = 0;
			last = 0;
			if (symbols > 0) {
				return(NEWLINE);
			}
			lineno = yylineno;
		}
\'		{	/* character constant */
			if (sdl == NO) {
				multicharconstant('\'');
			}
			goto more;
			/* NOTREACHED */
		}
\"		{	/* string constant */
			multicharconstant('"');
			goto more;
			/* NOTREACHED */
		}
^[ \t\f]+	{	/* don't save leading white space */
		}
\#[ \t]*include[ \t]*["<][^"> \t\n]+	{ /* #include file */
			char	*s;
			
			s = strpbrk(yytext, "\"<");
			incfile(s + 1, *s);
			first = s - yytext;
			last = yyleng;
			if (compress == YES) {
				yytext[0] = '\1';	/* compress the keyword */
			}
			yymore();
			return(INCLUDE);
			/* NOTREACHED */
		}
\#[ \t]*{identifier}	|	/* preprocessor keyword */
{number}	|	/* number */
.		{	/* punctuation and operators */
		more:	first = yyleng;
			yymore();
		}
%%

void
initscanner(srcfile)
char	*srcfile;
{
	char	*s;
	
	if (maxifbraces == NULL) {
		maxifbraces = (int *) mymalloc(miflevel * sizeof(int));
		preifbraces = (int *) mymalloc(miflevel * sizeof(int));
	}
	first = 0;		/* buffer index for first char of symbol */
	last = 0;		/* buffer index for last char of symbol */
	lineno = 1;		/* symbol line number */
	yylineno = 1;		/* input line number */
	arraydimension = NO;	/* inside array dimension declaration */
	bplisting = NO;		/* breakpoint listing */
	braces = 0;		/* unmatched left brace count */
	classdef = NO;		/* c++ class definition */
	elseelif = NO;		/* #else or #elif found */
	esudef = NO;		/* enum/struct/union global definition */
	external = NO;		/* external definition */
	externalbraces = -1;	/* external definition outer brace count */
	fcndef = NO;		/* function definition */
	global = YES;		/* file global scope (outside functions) */
	iflevel = 0;		/* #if nesting level */
	initializer = NO;	/* data initializer */
	initializerbraces = -1;	/* data initializer outer brace count */
	lex = NO;		/* lex file */
	parens = 0;		/* unmatched left parenthesis count */
	ppdefine = NO;		/* preprocessor define statement */
	psuedoelif = NO;	/* psuedo-#elif */
	oldtype = NO;		/* next identifier is an old type */
	rules = NO;		/* lex/yacc rules */
	sdl = NO;		/* sdl file */
	structfield = NO;	/* structure field declaration */
	tagdef = '\0';		/* class/enum/struct/union tag definition */
	template = NO;		/* function template */
	templateparens = -1;	/* function template outer parentheses count */
	typedefbraces = -1;	/* initial typedef braces count */
	BEGIN 0;

	/* if this is not a C file */
	if ((s = strrchr(srcfile, '.')) != NULL) {
		switch (*++s) {	/* this switch saves time on C files */
		case 'b':
			if (strcmp(s, "bp") == 0) {	/* breakpoint listing */
				bplisting = YES;
			}
			break;
		case 'l':
			if (strcmp(s, "l") == 0) {	/* lex */
				lex = YES;
				global = NO;
			}
			break;
		case 's':
			if (strcmp(s, "sd") == 0) {	/* sdl */
				sdl = YES;
				BEGIN SDL;
			}
			break;
		case 'y':
			if (strcmp(s, "y") == 0) {	/* yacc */
				global = NO;
			}
			break;
		}
	}
}

comment()
{
	register int	c, lastc;

	do {
		if ((c = getc(yyin)) == '*') {	/* C comment */
			lastc = '\0';
			while ((c = getc(yyin)) != EOF &&
			    (c != '/' || lastc != '*')) { /* fewer '/'s */
				if (c == '\n') {
					++yylineno;
				}
				lastc = c;
			}
			/* return a blank for Reiser cpp token concatenation */
			if ((c = getc(yyin)) == '_' || isalnum(c)) {
				(void) ungetc(c, yyin);
				c = ' ';
				break;
			}
		}
		else if (c == '/') {		/* C++ comment */
			while ((c = getc(yyin)) != EOF && c != '\n') {
				;
			}
			break;
		}
		else {	/* not a comment */
			(void) ungetc(c, yyin);
			c = '/';
			break;
			/* NOTREACHED */
		}
		
	/* there may be an immediately following comment */
	} while (c == '/');
	return(c);
}

void
multicharconstant(terminator)
register char	terminator;
{
	register char	c;

	/* scan until the terminator is found */
	while ((c = yytext[yyleng++] = noncommentinput()) != terminator) {
		switch (c) {
		case '\\':	/* escape character */
			if ((yytext[yyleng++] = noncommentinput()) == '\n') {
				++yylineno;
			}
			break;
		case '\t':	/* tab character */
		
			/* if not a lex program, continue */
			if (lex == NO) {
				break;
			}
			/* fall through */
			
		case '\n':	/* illegal character */
			
			/* assume the terminator is missing, so put
			   this character back */
			unput(c);
			yytext[--yyleng] = '\0';
			/* fall through */
			
		case LEXEOF:	/* end of file */
			return;
			
		default:
			/* change a control character to a blank */
			if (!isprint(c)) {
				yytext[yyleng - 1] = ' ';
			}
		}
		/* if this token will overflow the line buffer */
		/* note: '\\' may cause yyleng to be > STMTMAX */
		if (yyleng >= STMTMAX) {
			
			/* truncate the token */
			while ((c = noncommentinput()) != LEXEOF) {
				if (c == terminator) {
					unput(c);
					break;
				}
				else if (c == '\n') {
					++yylineno;
				}
			}
		}
	}
	yytext[yyleng] = '\0';
}
