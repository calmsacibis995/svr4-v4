#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:mapcmds/mapcmds.mk	1.1"
#
#	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987, 1988.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation and should be treated as Confidential.
#
# MODS
#
# S000 3 May 88	sco!daniel
# -	Add -Me to CFLAGS so this compiles in environments where -Me
# -	is not present in /etc/default/cc.
# S001 10 Feb 89 sco!lorie
# -	Changed install directory for screens files to /usr/lib/console
#	rather than /usr/lib/keyboard (per spec).
#

# R4.0 file not available include ../make.inc
INC = $(ROOT)/usr/include
LEX = lex
DIR = $(ROOT)/bin
INS = install
INSDIR = $(ROOT)/usr/bin
INSDIR2 = $(ROOT)/bin
INSDIR3 = $(ROOT)/usr/lib/console
CFLAGS	= -O -I$(INC)
LDFLAGS = -s $(SHLIBS)
DLIB	= ${ROOT}/usr/lib

EXES	= mapscrn setkey
SRCS	= mapscrn.c maplib.c setkey.c
OBJS	= mapscrn.o maplib.o setkey.o

FLATFILE1 = screens

all:	$(EXES) $(FLATFILE1)

$(INSDIR) $(INSDIR2) $(INSDIR3):
	-mkdir $@
	-$(CH) chmod 755 $@
	-$(CH) chown bin $@
	-$(CH) chgrp bin $@

install: $(INSDIR) $(INSDIR2) $(INSDIR3) all
	$(INS) -f $(INSDIR) -m 0711 -u bin -g bin mapscrn
	$(INS) -f $(INSDIR2) -m 0711 -u bin -g bin setkey
	-@ for i in $(FLATFILE1) ; do \
		rm -f $(INSDIR3)/$$i ;\
		if [ -f $$i ] ; then \
			cat $$i | sed -e '/ident/d' > $(INSDIR3)/$$i ;\
			chmod 633 $(INSDIR3)/$$i ;\
			chgrp bin $(INSDIR3)/$$i ;\
			chown bin $(INSDIR3)/$$i ;\
		fi ;\
	done  

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(EXES)

$(OBJS):	$(FRC)

FRC:


mapscrn: mapscrn.o maplib.o
	$(CC) $(CFLAGS) -o $@ mapscrn.o maplib.o $(LDFLAGS) 

setkey:	setkey.o
	$(CC) $(CFLAGS) -o $@ setkey.o $(LDFLAGS) 

