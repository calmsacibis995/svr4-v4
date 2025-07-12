#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cvtomf:cvtomf.mk	1.1"

#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.

#    Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
#		      All Rights Reserved

# Enhanced Application Compatibility Support

# Changed ARCH to ARCH_TYPE to remove conflict with build variable of the
# same name
ARCH_TYPE = AR32WR
INC	= $(ROOT)/usr/include
INS	= install
NPROC	= ONEPROC
FLEX	= -DFLEXNAMES
FFLAG	=
SCOINC	= ../sco

NATIVE	=
DEBUG=
CFLAGS	= -O $(XENIX) $(DEBUG)
DEFLIST	= -D$(NPROC) -Di386 -DARCH=F_$(ARCH_TYPE) -D$(ARCH_TYPE) $(FLEX) $(NATIVE)
LIBM	= -lmalloc
LIBS	=
LIBES	=


OBJECTS	= coff.o cvtomf.o omf.o proc_sym.o proc_typ.o fltused.o

CFILES	= coff.c cvtomf.c omf.c proc_sym.c proc_typ.c fltused.c

HFILES	= coff.h cvtomf.h omf.h symbol.h leaf.h $(SCOINC)/filehdr.h \
	  scnhdr.h linenum.h aouthdr.h reloc.h $(SCOINC)/syms.h \
	  storclass.h sgs.h 

SHFILES = cvtomflib.sh


all: cvtomf cvtomflib fltused.o

cvtomf: $(OBJECTS)
	$(CC) $(FFLAG) $(LDFLAGS) -o cvtomf $(OBJECTS) $(LIBS) $(LIBES) $(LIBM)

coff.o: coff.c coff.h cvtomf.h 
	$(CC) -c $(CFLAGS) $(DEFLIST) -I. -I$(SCOINC) -I$(INC) coff.c


cvtomf.o: cvtomf.c cvtomf.h 
	$(CC) -c $(CFLAGS) $(DEFLIST) -I. -I$(SCOINC) -I$(INC) cvtomf.c


omf.o: omf.c omf.h cvtomf.h 
	$(CC) -c $(CFLAGS) $(DEFLIST) -I. -I$(SCOINC) -I$(INC) omf.c

proc_sym.o: proc_sym.c symbol.h cvtomf.h leaf.h
	$(CC) -c $(CFLAGS) $(DEFLIST) -I. -I$(SCOINC) -I$(INC) proc_sym.c

proc_typ.o: proc_typ.c symbol.h cvtomf.h leaf.h
	$(CC) -c $(CFLAGS) $(DEFLIST) -I. -I$(SCOINC) -I$(INC) proc_typ.c

cvtomflib: cvtomflib.sh
	cp cvtomflib.sh cvtomflib
	chmod 755 cvtomflib

install: all
	-mkdir $(ROOT)/usr/bin
	$(INS) -f $(ROOT)/usr/bin cvtomf
	$(INS) -f $(ROOT)/usr/bin cvtomflib
	-mkdir $(ROOT)/usr/lib
	$(INS) -f $(ROOT)/usr/lib fltused.o

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f cvtomf cvtomflib
# End Enhanced Application Compatibility Support
