/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/gram.c	1.6"
/*
* common/gram.c - common assembler parsing
*/
#include <stdio.h>
#include <string.h>
#ifndef __STDC__
#  include <memory.h>
#endif
#include "common/as.h"
#include "common/dirs.h"
#include "common/expr.h"
#include "common/gram.h"
#include "common/syms.h"

static FILE *instream;	/* current input stream */
static Uchar *inbuf;	/* current input buffer */
static size_t current;	/* position of next character in buffer */
static size_t maxcur;	/* 1+position of last character in buffer */

static int
#ifdef __STDC__
fillbuf(void) /* append to buffer (grow if must); return whether at EOF */
#else
fillbuf()
#endif
{
	static size_t inbufsz;	/* length of input buffer */
	register size_t sz, mc;

	/*
	* Append to buffer (extending it as necessary) until at least
	* one '\n' has just been read, or EOF reached.  This behavior
	* guarantees that the token information for each statement will
	* remain valid during the parsing of that statement and that
	* scanning for single-line tokens need not refill the buffer.
	* Furthermore, arrange for a '\0' after EOF so that normal
	* scanning will not have to check for end-of-buffer conditions.
	* Finally, arrange to read always in exact multiples of BUFSIZ.
	* The extra 128 allocated bytes are to allow at most that many
	* characters on the moved end of the buffer [see newline()]
	* without causing a one less BUFSIZ than otherwise possible read.
	*/
	mc = maxcur;
	sz = 0;
	do
	{
		if ((mc += sz) + BUFSIZ >= inbufsz)
		{
			inbufsz += 4 * BUFSIZ + 128;
			inbuf = (Uchar *)alloc((void *)inbuf, inbufsz);
		}
		sz = (inbufsz - mc) / BUFSIZ * BUFSIZ;
		if ((sz = fread((void *)&inbuf[mc],
			sizeof(Uchar), sz, instream)) == 0)
		{
			if (ferror(instream))
			{
				error("read error");
				fatal((char *)0);
			}
			inbuf[mc] = '\0';
			break;
		}
	} while (memchr((void *)&inbuf[mc], '\n', sz) == 0);
	if ((mc += sz) <= maxcur)
		return 1;	/* at EOF */
	maxcur = mc;
	return 0;
}

static Uchar *
#ifdef __STDC__
comment(void) /* skip to \n or EOF (return 0); just saw comment intro */
#else
comment()
#endif
{
	register Uchar *cur;

	if ((cur = (Uchar *)memchr((void *)&inbuf[current], '\n',
		maxcur - current)) == 0)
	{
		error("end-of-file in comment");
		current = maxcur;
		return 0;
	}
	current = cur - inbuf;
	return cur;
}

static Uchar chtab[1 << CHAR_BIT]; /* character classification table */
#define CH_LET	0x01	/* a-z, A-Z, ., _ */
#define CH_OCT	0x02	/* 0-7 */
#define CH_DIG	0x04	/* 0-9 */
#define CH_HEX	0x08	/* 0-9, a-f, A-F, */
#define CH_B_X	0x10	/* b, x, B, X */
#define CH_E_E	0x20	/* e, E */
#define CH_SGN	0x40	/* +, - */

static void
#ifdef __STDC__
setchtab(register int bit, register const Uchar *let) /* set for each ch */
#else
setchtab(bit, let)register int bit; register Uchar *let;
#endif
{
	while (*let != '\0')
		chtab[*let++] |= bit;
}

void
#ifdef __STDC__
initchtab(void)	/* initialize character classification table */
#else
initchtab()
#endif
{
	setchtab(CH_LET, (const Uchar *)"abcdefghijklmnopqrstuvwxyz.");
	setchtab(CH_LET, (const Uchar *)"ABCDEFGHIJKLMNOPQRSTUVWXYZ_$");
	setchtab(CH_OCT, (const Uchar *)"01234567");
	setchtab(CH_DIG, (const Uchar *)"0123456789");
	setchtab(CH_HEX, (const Uchar *)"0123456789abcdefABCDEF");
	setchtab(CH_B_X, (const Uchar *)"bxBX");
	setchtab(CH_E_E, (const Uchar *)"eE");
	setchtab(CH_SGN, (const Uchar *)"+-");
}

static size_t token_beg;	/* start of last yylex() token in inbuf[] */
static size_t token_len;	/* extent of last yylex() token in inbuf[] */

static int
#ifdef __STDC__
name(void)	/* look for identifier at current; return whether found */
#else
name()
#endif
{
	register Uchar *cur;
	register size_t loc;

	loc = current;
	cur = inbuf + loc;
	if (!(chtab[*cur] & CH_LET))
		return 0;
	token_beg = loc;
	while (chtab[*++cur] & (CH_LET | CH_DIG))
		;
	current = cur - inbuf;
	token_len = current - loc;
	return 1;
}

static int
#ifdef __STDC__
numtoken(void)	/* number, first digit already; return whether floating */
#else
numtoken()
#endif
{
	register Uchar *cur;
	register size_t loc;
	register int retval = 0;

	/*
	* Allow some garbage number-like tokens through:
	* + Octal numbers may contain 8 and 9 digits.
	* + Binary numbers may contain [2-9a-fA-F].
	* + Numbers assumed to be binary or hex here may be taken as octal
	*   or decimal since they do not necessarily start with 0[bBxX].
	* Any such characters will be complained about during conversion.
	* Floating numbers are carefully recognized.
	*/
	loc = current - 1;
	token_beg = loc;
	cur = inbuf + loc;
	/*
	* Skip initial digits, stop on first nondecimal character.
	*/
	while (chtab[*++cur] & CH_DIG)
		;
	if (chtab[*cur] & CH_B_X)	/* bxBX */
	{
		/*
		* Skip any sequence of valid hex digits.
		* This also covers binary constants.
		*/
		while (chtab[*++cur] & CH_HEX)
			;
	}
	else	/* handle possible floating number */
	{
		if (*cur == '.')	/* handle significand */
		{
			retval = 1;
			while (chtab[*++cur] & CH_DIG)
				;
		}
		if (chtab[*cur] & CH_E_E) /* handle exponent */
		{
			register Uchar *ptr = cur;

			if (chtab[*++cur] & CH_SGN)
				cur++;
			if (!(chtab[*cur] & CH_DIG))
				cur = ptr;	/* incomplete exponent */
			else
			{
				retval = 1;
				while (chtab[*++cur] & CH_DIG)
					;
			}
		}
	}
	current = cur - inbuf;
	token_len = current - loc;
	return retval;
}

static void
#ifdef __STDC__
strtoken(void)	/* recognize string token; " already seen */
#else
strtoken()
#endif
{
	static const char MSGstr[] =
		"string missing terminating \", at %s";
	register Uchar *cur;
	register Uchar *now;	/* where to store next byte value */
	register Uchar *end;	/* only used if \new-line occurs */

	/*
	* Convert string from original to actual list
	* of byte values in place during recognition.
	*/
	now = cur = inbuf + current;
	end = inbuf + maxcur;
	for (;;)
	{
		if (*cur == '\\')	/* based on C's escapes */
		{
			switch (*++cur)
			{
			case '\n':	/* ignore \new-line pair */
				/*
				* Verify that there are characters in the
				* buffer through to the next new-line or EOF.
				*/
				if (++cur >= end
					|| memchr((void *)cur, '\n',
					end - cur) == 0)
				{
					register size_t cloc = cur - inbuf;
					register size_t nloc = now - inbuf;

					if (fillbuf())	/* at EOF */
					{
						error(MSGstr, "end-of-file");
						goto out;
					}
					cur = inbuf + cloc;
					now = inbuf + nloc;
					end = inbuf + maxcur;
				}
				curlineno++;
				continue;
			case 'a':
				*now = '\7'; /* assume ascii */
				break;
			case 'b':
				*now = '\b';
				break;
			case 'f':
				*now = '\f';
				break;
			case 'n':
				*now = '\n';
				break;
			case 'r':
				*now = '\r';
				break;
			case 't':
				*now = '\t';
				break;
			case 'v':
				*now = '\v';
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				*now = *cur - '0';
				if (chtab[*++cur] & CH_OCT)
				{
					*now <<= 3;
					*now += *cur - '0';
					if (chtab[*++cur] & CH_OCT)
					{
						*now <<= 3;
						*now += *cur - '0';
						cur++;
					}
				}
				now++;
				continue;
			default:	/* ignore \ by default */
				*now = *cur;
				break;
			}
			now++;
			cur++;
		}
		else if (*cur == '"')
		{
			cur++;	/* skip " character */
			break;
		}
		else if (*cur == '\n')
		{
			error(MSGstr, "new-line");
			break;
		}
		else
			*now++ = *cur++;	/* normal character */
	}
out:;
	token_beg = current;
	token_len = (now - inbuf) - current;
	current = cur - inbuf;
}

static int
#ifdef __STDC__
newline(void) /* advance curlineno and move buffer contents if necessary */
#else
newline()
#endif
{
	register size_t len;
	register Uchar *p, *q;
	register size_t cur;

	curlineno++;
	/*
	* Check for new-line just before buffer end,
	* or EOF without immediately preceding new-line.
	*/
	if ((cur = current) >= maxcur)
	{
		current = 0;
		maxcur = 0;
		return fillbuf();
	}
	p = &inbuf[cur];
	len = maxcur - cur;
	if (memchr((void *)p, '\n', len) == 0) /* no new-lines in buffer */
	{
		/*
		* Copy down remaining characters.  This loop must
		* be done by hand since the source and target
		* regions in inbuf may overlap.
		*/
		q = inbuf;
		do
		{
			*q++ = *p++;
		} while (--len != 0);
		current = 0;
		maxcur = q - inbuf;
		(void)fillbuf();	/* append new characters */
	}
	return 0;
}

#ifdef __STDC__
static int	synerr(void);		/* "syntax error" and recover */
static Expr	*exprparse(void);	/* parse expression */
static Operand	*operparse(void);	/* parse operand */
static int	yylex(void);		/* return next token */
#else
static int	synerr();
static Expr	*exprparse();
static Operand	*operparse();
static int	yylex();
#endif

	/*
	* The initial token values are expression operators.
	* Any additional tokens for the implementation should
	* use TOKEN_AVAIL and up.
	*/
#define TOKEN_EOF	(ExpOp_TOTAL + 0)	/* end of file */
#define TOKEN_EOS	(ExpOp_TOTAL + 1)	/* end of statement */
#define TOKEN_COMMA	(ExpOp_TOTAL + 2)
#define TOKEN_COLON	(ExpOp_TOTAL + 3)
#define TOKEN_AVAIL	(ExpOp_TOTAL + 4)	/* first available value */

	/*
	* Bits in the implementation-provided token table type field.
	* If toktab[] (provided in parse.c) is initialized at compile-time
	* then new expression operators will require additions to the table.
	*/
#define TOKTY_BINARY	0x01	/* can be binary operator */
#define TOKTY_PREFIX	0x02	/* can be unary prefix operator */
#define TOKTY_POSTFIX	0x04	/* can be unary postfix operator */
#define TOKTY_IDPICOP	0x08	/* can be identifier-modifying PIC operator */
#define TOKTY_LEFT	0x10	/* is a left grouping token */
#define TOKTY_RIGHT	0x20	/* is a right grouping token */
#define TOKTY_BUFFER	0x40	/* token spelling is in buffer */

struct token_info	/* contains parsing info for each token */
{
	const char	*tok_name;	/* generic spelling */
	Uint		tok_type;	/* TOKTY_* bits */
};

static int curtok;	/* current (look ahead) token */

#include "parse.c" /* implementation-specific portion */

static int
#ifdef __STDC__
synerr(void)	/* issue "syntax error"; skip to TOKEN_EOF or TOKEN_EOS */
#else
synerr()
#endif
{
	register int tok = curtok;

	if (!(toktab[tok].tok_type & TOKTY_BUFFER))
		error("syntax error at %s", toktab[tok].tok_name);
	else
	{
		error(tok == ExpOp_LeafString
			? "syntax error at %s: \"%s\""
			: "syntax error at %s: %s",
			toktab[tok].tok_name,
			prtstr(inbuf + token_beg, token_len));
	}
	while (tok != TOKEN_EOS)
	{
		if (tok == TOKEN_EOF)
			return 1;
		tok = yylex();
	}
	return 0;
}

int
#ifdef __STDC__
yyparse(void)	/* (always) return 0 for success */
#else
yyparse()
#endif
{
	register Oplist *olp;
	register Operand *op;
	register int tok;
	register size_t idbeg, idlen;

	while ((tok = yylex()) != TOKEN_EOF)
	{
		if (tok == TOKEN_EOS)
			continue;
		if (tok != ExpOp_LeafName)
		{
			curtok = tok;
			if (synerr())
				break;	/* at TOKEN_EOF */
			continue;
		}
		idbeg = token_beg;
		idlen = token_len;
		if ((tok = yylex()) == TOKEN_COLON)
		{
			label(inbuf + idbeg, idlen);
			continue;
		}
		if (tok == TOKEN_EOS)
		{
			stmt(inbuf + idbeg, idlen, (Oplist *)0);
			continue;
		}
#ifdef TOKEN_ASSIGN
		if (tok == TOKEN_ASSIGN) /* id = operand => .set id, operand */
		{
			olp = oplist((Oplist *)0,
				operand(idexpr(inbuf + idbeg, idlen)));
			curtok = yylex();
			if ((op = operparse()) != 0 && curtok == TOKEN_EOS)
			{
				directive((const Uchar *)".set", (size_t)4,
					oplist(olp, op));
			}
			else if (synerr())
				break;		/* at EOF */
			continue;
		}
#endif
		/*
		* Gather all operands into a list.
		*/
		olp = 0;
		curtok = tok;
		do
		{
			if ((op = operparse()) == 0)
				break;
			olp = oplist(olp, op);
		} while (curtok == TOKEN_COMMA
			&& (curtok = yylex()) != TOKEN_EOF);
		/*
		* We either stopped gathering due to a bad token
		* or because we reached the end of the statement.
		*/
		if (op != 0 && curtok == TOKEN_EOS)
			stmt(inbuf + idbeg, idlen, olp);
		else
		{
			if (synerr())
				break;	/* at TOKEN_EOF */
		}
	}
	return 0;
}

static Expr *
#ifdef __STDC__
primaryparse(void) /* primary -> name | name @PIC | num | flt | str | (exp) */
#else
primaryparse()
#endif
{
	register Expr *left;

	switch (curtok)
	{
	case ExpOp_LeafName:
		left = idexpr(inbuf + token_beg, token_len);
		if (!(toktab[curtok = yylex()].tok_type & TOKTY_IDPICOP))
			return left;
		left = unaryexpr(curtok, left);
		break;
	case ExpOp_LeafNumber:
		left = numexpr(inbuf + token_beg, token_len);
		break;
	case ExpOp_LeafFloat:
		left = fltexpr(inbuf + token_beg, token_len);
		break;
	case ExpOp_LeafString:
		left = strexpr(inbuf + token_beg, token_len);
		break;
	default:
		if (!(toktab[curtok].tok_type & TOKTY_LEFT))
			return 0;
		curtok = yylex();
		if ((left = exprparse()) == 0)
			return 0;
		if (!(toktab[curtok].tok_type & TOKTY_RIGHT))
			return 0;
		break;
	}
	curtok = yylex();
	return left;
}

static Expr *
#ifdef __STDC__
postfixparse(void)	/* postfix -> primary post'; post' -> | op post' */
#else
postfixparse()
#endif
{
	register Expr *left;
	register int op;

	if ((left = primaryparse()) == 0)
		return 0;
	while (toktab[op = curtok].tok_type & TOKTY_POSTFIX)
	{
		curtok = yylex();
		left = unaryexpr(op, left);
	}
	return left;
}

static Expr *
#ifdef __STDC__
prefixparse(void)	/* prefix -> op prefix | postfix */
#else
prefixparse()
#endif
{
	register Expr *right;
	register int op;

	if (toktab[op = curtok].tok_type & TOKTY_PREFIX)
	{
		curtok = yylex();
		if ((right = prefixparse()) == 0)
			return 0;
		return unaryexpr(op, right);
	}
	return postfixparse();
}

static Expr *
#ifdef __STDC__
exprparse(void)	/* expr -> prefix expr'; expr' -> | op prefix expr' */
#else
exprparse()
#endif
{
	register Expr *left, *right;
	register int op;

	if ((left = prefixparse()) == 0)
		return 0;
	while (toktab[op = curtok].tok_type & TOKTY_BINARY)
	{
		curtok = yylex();
		if ((right = prefixparse()) == 0)
			return 0;
		left = binaryexpr(op, left, right);
	}
	return left;
}
