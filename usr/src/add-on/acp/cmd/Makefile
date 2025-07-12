#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft 
#	Corporation and should be treated as Confidential.

#ident	"@(#)xx:cmd/Makefile	1.2.1.3"

COMMANDS = fsck mount
FRC =

all:
	for cmd in $(COMMANDS) ; \
	do \
		(cd $$cmd; $(MAKE) -f $$cmd.mk "FRC=$(FRC)" all); \
	done

install:
	for cmd in $(COMMANDS) ; \
	do \
		(cd $$cmd; $(MAKE) -f $$cmd.mk "FRC=$(FRC)" install); \
	done ;\
	[ -d $(ROOT)/usr/lib/event ] || mkdir $(ROOT)/usr/lib/event
	cp devices ttys $(ROOT)/usr/lib/event

clean:
	for cmd in $(COMMANDS) ; \
	do \
		(cd $$cmd; $(MAKE) -f $$cmd.mk clean); \
	done

clobber:	clean
