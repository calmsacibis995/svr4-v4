#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:maplocale/maplocale.mk	1.6"
#
INC = $(ROOT)/usr/include

DIR = $(ROOT)/bin
CFLAGS	= -O -I$(INC)
LDFLAGS = -s $(SHLIBS)
INS = install
UUDECODE = uudecode

INSDIR = $(ROOT)/usr/bin
INSDIR1 = $(ROOT)/etc
INSDIR2 = $(ROOT)/etc/default
INSDIR3 = $(ROOT)/usr/lib
INSDIR4 = $(ROOT)/usr/lib/lang
INSDIR5 = $(ROOT)/usr/lib/lang/english
INSDIR6 = $(ROOT)/usr/lib/lang/english/us
INSDIR7 = $(ROOT)/usr/lib/lang/english/us/88591

DEFAULT = default
DATAFILES =  collate ctype currency messages numeric time

all:	maplocale $(DATAFILES)

maplocale:	maplocale.o

$(INSDIR) $(INSDIR1) $(INSDIR2) $(INSDIR3) $(INSDIR4) $(INSDIR5) $(INSDIR6) $(INSDIR7) :
	-mkdir $@
	-$(CH) chmod 755 $@
	-$(CH) chown bin $@
	-$(CH) chgrp bin $@

install: $(INSDIR) $(INSDIR1) $(INSDIR2) $(INSDIR3) $(INSDIR4) \
	 $(INSDIR5) $(INSDIR6) $(INSDIR7) all
	$(INS) -f $(INSDIR) -m 0711 -u bin -g bin maplocale

	cp default $(INSDIR2)/lang
	- for i in $(DATAFILES) ; do \
		rm -f $(INSDIR7)/$$i ;\
		if [ -f $$i ] ; then \
			cp $$i  $(INSDIR7)/$$i ;\
			chmod 644 $(INSDIR7)/$$i ;\
			chgrp bin $(INSDIR7)/$$i ;\
			chown bin $(INSDIR7)/$$i ;\
		fi ;\
	done  
clean: 
	rm -f maplocale.o

clobber:	clean
	rm -f maplocale

maplocale:	maplocale.o
	$(CC) $(CFLAGS) -o maplocale maplocale.o $(LDFLAGS) 


collate:	collate.src
	cat collate.src | grep -v "^#ident	" | $(UUDECODE)

ctype:	ctype.src
	cat ctype.src | grep -v "^#ident	" | $(UUDECODE)

currency:	currency.src
	cat currency.src | grep -v "^#ident	" | $(UUDECODE)

numeric:	numeric.src
	cat numeric.src | grep -v "^#ident	" | $(UUDECODE)

messages:	messages.src
	cat messages.src | grep -v "^#ident	" | $(UUDECODE)

time:	time.src
	cat time.src | grep -v "^#ident	" | $(UUDECODE)

