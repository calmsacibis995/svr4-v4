#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:eac.mk	1.2"

#	Makefile for eac 

ROOT =

LEX = lex

DIR = $(ROOT)/bin

INC = $(ROOT)/usr/include

INS = install

# Enhanced Application Compatibility Support


CFLAGS = -O -I$(INC)
LDFLAGS = -s $(SHLIBS)
MAKEFILE = eac.mk

# Enhanced Application Compatibility Support

MAINS = subdirs


all:	$(MAINS)


subdirs: 
	for i in  mapchan mapkey mapcmds rename vidi maplocale dosutil ;\
	do \
		( \
		echo "cd $$i && $(MAKE) -f $$i.mk " ;\
		cd $$i && $(MAKE) -f $$i.mk ;\
		) \
	done ;


install: all
	for i in  mapchan mapkey mapcmds rename vidi maplocale dosutil ;\
	do \
		( \
		echo "cd $$i && $(MAKE) -f $$i.mk install " ;\
		cd $$i && $(MAKE) -f $$i.mk  install;\
		) \
	done ;

clean: 
	for i in  mapchan mapkey mapcmds rename vidi maplocale dosutil ;\
	do \
		( \
		echo "cd $$i && $(MAKE) -f $$i.mk clean " ;\
		cd $$i && $(MAKE) -f $$i.mk  clean;\
		) \
	done ;

clobber: clean
	for i in  mapchan mapkey mapcmds rename vidi maplocale dosutil ;\
	do \
		( \
		echo "cd $$i && $(MAKE) -f $$i.mk clobber " ;\
		cd $$i && $(MAKE) -f $$i.mk  clobber;\
		) \
	done ;
# End Enhanced Application Compatibility Support
