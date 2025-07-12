#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)xinstall:xinstall.mk	1.1.1.4"

INSDIR0=$(ROOT)/sbin
INSDIR=$(ROOT)/usr
INSDIR1=$(ROOT)/usr/lib
INSDIR2=$(ROOT)/usr/lib/custom
INSDIR3=$(ROOT)/etc
INSDIR4=$(ROOT)/etc/perms

INS=install
FILES = xinstall fixperm fixshlib custom help inst

all: $(FILES)

$(INSDIR0) $(INSDIR)  $(INSDIR1)  $(INSDIR2)  $(INSDIR3)  $(INSDIR4):
	-mkdir $@
	-$(CH) chmod 755 $@
	-$(CH) chown bin $@
	-$(CH) chgrp bin $@

install:$(INSDIR0) $(INSDIR) $(INSDIR1) $(INSDIR2) $(INSDIR3) $(INSDIR4) all
	$(INS) -f $(ROOT)/etc/perms inst
	$(INS) -f $(ROOT)/sbin xinstall
	$(INS) -f $(ROOT)/sbin fixperm
	$(INS) -f $(ROOT)/sbin fixshlib
	$(INS) -f $(ROOT)/sbin custom
	$(INS) -f $(ROOT)/usr/lib/custom help

clean clobber:
	rm -f *.o  fixshlib fixperm custom xinstall help inst

fixperm: fixpermR4.o
	$(CC) -O -s -o fixperm fixpermR4.o $(LDLIBS)

fixshlib: fixshlib.o
	$(CC) -O -s -o fixshlib fixshlib.o $(LDLIBS)
xinstall: xinstall.sh
	cp xinstall.sh xinstall

custom: customR4.sh
	cp customR4.sh custom

help: helpR4.src
	cp helpR4.src help

inst:
	cp instR4.perm inst
