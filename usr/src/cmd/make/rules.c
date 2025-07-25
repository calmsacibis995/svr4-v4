/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)make:rules.c	1.17"
/*	@(#)make:rules.c	1.14 of 7/22/88	*/

/* DEFAULT RULES FOR UNIX
**
**	These are the internal rules that "make" trucks around with it at
**	all times. One could completely delete this entire list and just
**	conventionally define a global "include" makefile which had these
**	rules in it. That would make the rules dynamically changeable
**	without recompiling make. This file may be modified to local
**	needs.
*/

char *builtin[] = {

	".SUFFIXES: .o .c .c~ .y .y~ .l .l~ .s .s~ .sh .sh~ .h .h~ .f .f~ \
.C .C~ .Y .Y~ .L .L~",

/* PRESET VARIABLES */
	"MAKE=make", 	"BUILD=build",
	"AR=ar",	"ARFLAGS=-rv",
	"AS=as", 	"ASFLAGS=",
	"CC=cc", 	"CFLAGS=-O",
	"F77=f77", 	"FFLAGS=-O",
	"GET=get", 	"GFLAGS=",
	"LD=ld", 	"LDFLAGS=",
	"LEX=lex", 	"LFLAGS=",
	"YACC=yacc", 	"YFLAGS=",
	"C++C=CC", 	"C++FLAGS=-O",

/* SINGLE SUFFIX RULES */
	".c:",
		"\t$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<",
	".c~:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $*.c",
		"\t-rm -f $*.c",
	".f:",
		"\t$(F77) $(FFLAGS) $(LDFLAGS) -o $@ $<",
	".f~:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(F77) $(FFLAGS) $(LDFLAGS) -o $@ $*.f",
		"\t-rm -f $*.f",
	".s:",
		"\t$(AS) $(ASFLAGS) -o $*.o $<",
		"\t$(CC) $(LDFLAGS) -o $@ $*.o",
		"\t-rm -f $*.o",
	".s~:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(AS) $(ASFLAGS) -o $*.o $*.s",
		"\t$(CC) $(LDFLAGS) -o $* $*.o",
		"\t-rm -f $*.[so]",
	".sh:",
		"\tcp $< $@; chmod +x $@",
	".sh~:",
		"\t$(GET) $(GFLAGS) $<",
		"\tcp $*.sh $*; chmod +x $@",
		"\t-rm -f $*.sh",
	".C:",
		"\t$(C++C) $(C++FLAGS) $(LDFLAGS) -o $@ $<",
	".C~:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(C++C) $(C++FLAGS) $(LDFLAGS) -o $@ $*.C",
		"\t-rm -f $*.C",

/* DOUBLE SUFFIX RULES */
	".c.a:",
		"\t$(CC) $(CFLAGS) -c $<",
		"\t$(AR) $(ARFLAGS) $@ $*.o",
		"\t-rm -f $*.o",
	".c.o:",
		"\t$(CC) $(CFLAGS) -c $<",
	".c~.a:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(CC) $(CFLAGS) -c $*.c",
		"\t$(AR) $(ARFLAGS) $@ $*.o",
		"\t-rm -f $*.[co]",
	".c~.c:",
		"\t$(GET) $(GFLAGS) $<",
	".c~.o:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(CC) $(CFLAGS) -c $*.c",
		"\t-rm -f $*.c",
	".f.a:",
		"\t$(F77) $(FFLAGS) -c $*.f",
		"\t$(AR) $(ARFLAGS) $@ $*.o",
		"\t-rm -f $*.o",
	".f.o:",
		"\t$(F77) $(FFLAGS) -c $*.f",
	".f~.a:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(F77) $(FFLAGS) -c $*.f",
		"\t$(AR) $(ARFLAGS) $@ $*.o",
		"\t-rm -f $*.[fo]",
	".f~.f:",
		"\t$(GET) $(GFLAGS) $<",
	".f~.o:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(F77) $(FFLAGS) -c $*.f",
		"\t-rm -f $*.f",
	".h~.h:",
		"\t$(GET) $(GFLAGS) $<",
	".l.c:",
		"\t$(LEX) $(LFLAGS) $<",
		"\tmv lex.yy.c $@",
	".l.o:",
		"\t$(LEX) $(LFLAGS) $<",
		"\t$(CC) $(CFLAGS) -c lex.yy.c",
		"\t-rm -f lex.yy.c; mv lex.yy.o $@",
	".l~.c:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(LEX) $(LFLAGS) $*.l",
		"\tmv lex.yy.c $@",
		"\t-rm -f $*.l",
	".l~.l:",
		"\t$(GET) $(GFLAGS) $<",
	".l~.o:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(LEX) $(LFLAGS) $*.l",
		"\t$(CC) $(CFLAGS) -c lex.yy.c",
		"\t-rm -f lex.yy.c $*.l",
		"\tmv lex.yy.o $@",
	".s.a:",
		"\t$(AS) $(ASFLAGS) -o $*.o $*.s",
		"\t$(AR) $(ARFLAGS) $@ $*.o",
	".s.o:",
		"\t$(AS) $(ASFLAGS) -o $@ $<",
	".s~.a:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(AS) $(ASFLAGS) -o $*.o $*.s",
		"\t$(AR) $(ARFLAGS) $@ $*.o",
		"\t-rm -f $*.[so]",
	".s~.o:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(AS) $(ASFLAGS) -o $*.o $*.s",
		"\t-rm -f $*.s",
	".s~.s:",
		"\t$(GET) $(GFLAGS) $<",
	".sh~.sh:",
		"\t$(GET) $(GFLAGS) $<",
	".y.c:",
		"\t$(YACC) $(YFLAGS) $<",
		"\tmv y.tab.c $@",
	".y.o:",
		"\t$(YACC) $(YFLAGS) $<",
		"\t$(CC) $(CFLAGS) -c y.tab.c",
		"\t-rm -f y.tab.c",
		"\tmv y.tab.o $@",
	".y~.c:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(YACC) $(YFLAGS) $*.y",
		"\tmv y.tab.c $*.c",
		"\t-rm -f $*.y",
	".y~.o:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(YACC) $(YFLAGS) $*.y",
		"\t$(CC) $(CFLAGS) -c y.tab.c",
		"\t-rm -f y.tab.c $*.y",
		"\tmv y.tab.o $*.o",
	".y~.y :",
		"\t$(GET) $(GFLAGS) $<",
	".C.a:",
		"\t$(C++C) $(C++FLAGS) -c $<",
		"\t$(AR) $(ARFLAGS) $@ $*.o",
		"\t-rm -f $*.o",
	".C.o:",
		"\t$(C++C) $(C++FLAGS) -c $<",
	".C~.a:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(C++C) $(C++FLAGS) -c $*.C",
		"\t$(AR) $(ARFLAGS) $@ $*.o",
		"\t-rm -f $*.[Co]",
	".C~.C:",
		"\t$(GET) $(GFLAGS) $<",
	".C~.o:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(C++C) $(C++FLAGS) -c $*.C",
		"\t-rm -f $*.C",
	".L.C:",
		"\t$(LEX) $(LFLAGS) $<",
		"\tmv lex.yy.c $@",
	".L.o:",
		"\t$(LEX) $(LFLAGS) $<",
		"\t$(C++C) $(C++FLAGS) -c lex.yy.c",
		"\t-rm -f lex.yy.c; mv lex.yy.o $@",
	".L~.C:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(LEX) $(LFLAGS) $*.L",
		"\tmv lex.yy.c $@",
		"\t-rm -f $*.L",
	".L~.L:",
		"\t$(GET) $(GFLAGS) $<",
	".L~.o:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(LEX) $(LFLAGS) $*.L",
		"\t$(C++C) $(C++FLAGS) -c lex.yy.c",
		"\t-rm -f lex.yy.c $*.L",
		"\tmv lex.yy.o $@",
	".Y.C:",
		"\t$(YACC) $(YFLAGS) $<",
		"\tmv y.tab.c $@",
	".Y.o:",
		"\t$(YACC) $(YFLAGS) $<",
		"\t$(C++C) $(C++FLAGS) -c y.tab.c",
		"\t-rm -f y.tab.c",
		"\tmv y.tab.o $@",
	".Y~.C:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(YACC) $(YFLAGS) $*.Y",
		"\tmv y.tab.c $*.C",
		"\t-rm -f $*.Y",
	".Y~.o:",
		"\t$(GET) $(GFLAGS) $<",
		"\t$(YACC) $(YFLAGS) $*.Y",
		"\t$(C++C) $(C++FLAGS) -c y.tab.c",
		"\t-rm -f y.tab.c $*.Y",
		"\tmv y.tab.o $*.o",
	".Y~.Y :",
		"\t$(GET) $(GFLAGS) $<",

	"markfile.o:	markfile",
		"\tA=@;echo \"static char _sccsid[] = \\042`grep $$A'(#)' markfile`\\042;\" > markfile.c",
		"\t$(CC) -c markfile.c",
		"\t-rm -f markfile.c",
	    0 };
