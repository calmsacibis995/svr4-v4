#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nas:i386/Makefile	1.2"
#
# i386 assembler makefile
#

CFLAGS	= -O
MACH	= i386

AWK	= nawk
# To enable debug code:   DEBUG=-DDEBUG
# To disable debug code:  DEBUG=
DEBUG	=
FORMAT	= -DFORMAT=ELF
OBJFLIB	= ../../libelf/$(MACH)/libelf.a
DEFLIST	= $(DEBUG) $(FORMAT)
# FP emulation is selected
FP_OBJS = $(FPEOBJS)
FP_INC	= $(FPEINC)

MACHOBJS= dirs386.$o inst386.$o main386.$o relo386.$o stmt386.$o

include ../common/mkdeps

$(MACHOBJS): $C/as.h
dirs386.$o: amode.h dirs386.h $C/dirs.h $C/expr.h $C/nums.h $C/sect.h $C/syms.h
gram.$o:    stmt386.h $C/nums.h $C/stmt.h
inst386.$o: chkgen.c chkgen.h inst386.h stmt386.h $C/stmt.h
main386.$o: stmt386.h $(MACHINC)/sgs.h $C/stmt.h
relo386.$o: relo386.h stmt386.h \
		$C/expr.h $C/nums.h $C/relo.h $C/sect.h $C/stmt.h $C/syms.h
stmt386.$o: amode.h chkgen.h dirs386.h relo386.h stmt386.h \
		$C/expr.h $C/nums.h $C/sect.h $C/stmt.h $C/syms.h

stmt386.h:  chkgen.h

chkgen.c chkgen.h:  chkgen.awk chkgen.in chkgen.0.h 
	$(AWK) -f chkgen.awk chkgen.in chkgen.h chkgen.c

clobber::
	rm -f chkgen.c chkgen.h
