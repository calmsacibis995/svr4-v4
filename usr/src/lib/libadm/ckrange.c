/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libadm:ckrange.c	1.3.3.2"

#include <stdio.h>
#include <string.h>

extern int	ckquit;

extern long	strtol();
extern void	puterror(),
		puthelp(),
		putprmpt();
extern int	getinput();

#define MSGSIZ	256
#define PROMPT10	"Enter an integer between %ld and %ld"
#define PROMPT16	"Enter a base %d integer between %lx and %lx"
#define PROMPT8		"Enter a base %d integer between %lo and %lo"
#define MESG10		"Please enter an integer between %ld and %ld."
#define MESG16		"Please enter a base %d integer between %lx and %lx."
#define MESG8		"Please enter a base %d integer between %lo and %lo."

static void
setmsg(msg, lower, upper, base)
char	*msg;
long	lower, upper;
int	base;
{
	if(base == 0) base = 10;
	if(base == 16) 
		(void) sprintf(msg, MESG16, base, lower, upper);
	else
		if(base == 8) 
			(void) sprintf(msg, MESG8, base, lower, upper);
		else
			(void) sprintf(msg, MESG10, lower, upper);
}

void
ckrange_err(lower, upper, base, error)
long	lower, upper;
int	base;
char	*error;
{
	char	defmesg[MSGSIZ];

	setmsg(defmesg, lower, upper, base);
	puterror(stdout, defmesg, error);
}

void
ckrange_hlp(lower, upper, base, help)
long	lower, upper;
int	base;
char	*help;
{
	char	defmesg[MSGSIZ];

	setmsg(defmesg, lower, upper, base);
	puthelp(stdout, defmesg, help);
}

int
ckrange_val(lower, upper, base, input)
long	lower, upper;
int	base;
char	*input;
{
	char	*ptr;
	long	value;

	value = strtol(input, &ptr, base);
	if((*ptr != '\0') || (value < lower) || (value > upper))
		return(1);
	return(0);
}

int
ckrange(rngval, lower, upper, base, defstr, error, help, prompt)
long	*rngval;
long	lower, upper;
short	base;
char	*defstr, *error, *help, *prompt;
{
	int	valid, n;
	long	value;
	char	*ptr;
	char	input[128];
	char	defmesg[MSGSIZ];
	char	defpmpt[64];
	char	buffer[32];
	char	*choices[2];

	if(lower >= upper)
		return(2);

	if(base == 0)
		base = 10;

	if(base == 16)
		(void) sprintf(buffer, "%lx-%lx", lower, upper);
	else 
		if(base == 8)
			(void) sprintf(buffer, "%lo-%lo", lower, upper);
		else 
			(void) sprintf(buffer, "%ld-%ld", lower, upper);

	if(!prompt) {
		if(base == 16)
			(void) sprintf(defpmpt, PROMPT16, base, lower, upper);
		else
			if(base == 8)
				(void) sprintf(defpmpt, PROMPT8, base, lower, upper);
			else
				(void) sprintf(defpmpt, PROMPT10, lower, upper);
		prompt = defpmpt;
	}

	setmsg(defmesg, lower, upper, base);
	choices[0] = buffer;
	choices[1] = NULL;

start:
	putprmpt(stderr, prompt, choices, defstr);
	if(getinput(input))
		return(1);

	n = strlen(input);
	if(n == 0) {
		if(defstr) {
			*rngval = strtol(defstr, NULL, base);
			return(0);
		}
		puterror(stderr, defmesg, error);
		goto start;
	}
	if(!strcmp(input, "?")) {
		puthelp(stderr, defmesg, help);
		goto start;
	}
	if(ckquit && !strcmp(input, "q"))
		return(3);

	value = strtol(input, &ptr, base);
	if(valid = (*ptr == '\0'))
		valid = ((value >= lower) && (value <= upper));

	if(!valid) {
		puterror(stderr, defmesg, error);
		goto start;
	}
	*rngval = value;
	return(0);
}
