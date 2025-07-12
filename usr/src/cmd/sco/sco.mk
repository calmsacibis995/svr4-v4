#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sco:sco.mk	1.3.1.9"

#	Makefile for sco 

# Many of the things that were being built here have moved to component
# "eac".

ROOT =

LEX = lex

DIR = $(ROOT)/bin

INC = $(ROOT)/usr/include

INS = install

INSDIR = $(ROOT)/usr/bin

# Enhanced Application Compatibility Support
INSDIR2 = $(ROOT)/usr/eac

INSDIR3 = $(ROOT)/usr/eac/bin

INSDIR4 = $(ROOT)/usr/eac/lib

INSDIR5 = $(ROOT)/usr/lib/coff

INSDIR6 = $(ROOT)/usr/lib/xout
# End Enhanced Application Compatibility Support

CFLAGS = -O -I$(INC)

LDFLAGS = -s $(SHLIBS)

MAKEFILE = sco.mk

# Enhanced Application Compatibility Support
MAINS = scompat setcolor ar ar_coff ar_xout initsock

OBJECTS =  scompat.o setcolor.o ar.o argtype.o executil.o ar_coff.o ar_xout.o initsock

SOURCES =  scompat.c setcolor.c ar.c argtype.c executil.c ar_coff.c ar_xout.c initsock.c

SCRIPTS = cc tape

all:	$(MAINS) $(SCRIPTS)
# End Enhanced Application Compatibility Support

scompat:		scompat.o 
	$(CC) $(CFLAGS) -o scompat scompat.o $(LDFLAGS)

setcolor:		setcolor.o 
	$(CC) $(CFLAGS) -o setcolor setcolor.o $(LDFLAGS)

# Enhanced Application Compatibility Support
ar: ar.o argtype.o executil.o
	$(CC) $(CFLAGS) -o ar ar.o argtype.o executil.o $(LDFLAGS)

ar_coff: ar_coff.o
	$(CC) $(CFLAGS) -o ar_coff ar_coff.o $(LDFLAGS)

ar_xout: ar_xout.o
	$(CC) $(CFLAGS) -o ar_xout ar_xout.o $(LDFLAGS)

initsock: initsock.o
	$(CC) $(CFLAGS) -o initsock initsock.o $(LDFLAGS)

# End Enhanced Application Compatibility Support


scompat.o:	 $(INC)/sys/types.h $(INC)/sys/kd.h \
		 $(INC)/stdio.h $(INC)/fcntl.h $(INC)/sys/at_ansi.h \
		 $(INC)/errno.h $(INC)/string.h $(INC)/sys/param.h 

setcolor.o:	 $(INC)/sys/types.h $(INC)/sys/kd.h \
		 $(INC)/stdio.h $(INC)/fcntl.h $(INC)/sys/at_ansi.h \
		 $(INC)/errno.h $(INC)/string.h $(INC)/sys/param.h 

# Enhanced Application Compatibility Support
ar.o:		 $(INC)/stdio.h ./argtype.h

argtype.o:	 $(INC)/stdio.h $(INC)/fcntl.h $(INC)/sys/types.h \
		 $(INC)/sys/stat.h ./argtype.h ./filehdr.h \
		 $(INC)/sys/x.out.h

executil.o:	 $(INC)/stdio.h $(INC)/string.h ./argtype.h

ar_coff.o:	 $(INC)/stdio.h $(INC)/signal.h $(INC)/values.h \
		 $(INC)/sys/types.h $(INC)/sys/stat.h ./ar.h ./filehdr.h \
		 ./syms.h $(INC)/string.h ./paths.h

ar_xout.o:	 $(INC)/stdio.h $(INC)/signal.h $(INC)/values.h \
		 $(INC)/sys/types.h $(INC)/sys/stat.h ./ar.h ./filehdr.h \
		 ./syms.h $(INC)/string.h ./paths.h

initsock.o:	$(INC)/sys/types.h $(INC)/sys/sysmacros.h $(INC)/sys/param.h \
		$(INC)/sys/file.h $(INC)/sys/vnode.h $(INC)/sys/stropts.h \
		$(INC)/sys/stream.h $(INC)/sys/strsubr.h $(INC)/netinet/in.h \
		$(INC)/sys/tiuser.h $(INC)/sys/sockmod.h $(INC)/sys/osocket.h \
		$(INC)/sys/stat.h

# End Enhanced Application Compatibility Support

$(INSDIR) $(INSDIR2) $(INSDIR3) $(INSDIR4) $(INSDIR5) $(INSDIR6):
	-mkdir $@
	-$(CH) chmod 755 $@
	-$(CH) chown bin $@
	-$(CH) chgrp bin $@

# Enhanced Application Compatibility Support
tape: tape.sh
	cp tape.sh tape
	chmod 755 tape

cc: cc.sh
	cp cc.sh cc
	chmod 755 cc
# End Enhanced Application Compatibility Support

install:	$(INSDIR) $(INSDIR2) $(INSDIR3) $(INSDIR4) $(INSDIR5) \
		$(INSDIR6) all
	$(INS) -f $(INSDIR) -m 0711 -u bin -g bin setcolor
	$(INS) -f $(INSDIR) -m 0711 -u bin -g bin scompat
# Enhanced Application Compatibility Support
	$(INS) -f $(INSDIR4) -m 0711 -u bin -g bin ar
	$(INS) -f $(INSDIR5) -m 0711 -u bin -g bin ar_coff
	mv $(INSDIR5)/ar_coff $(INSDIR5)/ar
	$(INS) -f $(INSDIR6) -m 0711 -u bin -g bin ar_xout
	mv $(INSDIR6)/ar_xout $(INSDIR6)/ar
	$(INS) -f $(INSDIR3) -m 0755 -u bin -g bin cc
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin tape
	$(INS) -f $(INSDIR3) -m 0755 -u bin -g bin initsock
# End Enhanced Application Compatibility Support
 
clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(OBJECTS) $(MAINS) $(SCRIPTS)
