#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)xl:ID/Makefile	1.3"

INSTALL=\
	Install \
	Remove	

ID = \
	Space.c

IDENT = \
	Files \
	Name \
	Size \
	System \
	Node \
	Master


all:	$(INSTALL) $(ID) $(IDENT) $(FRC)

install package:	all	$(FRC)
	-ls $(INSTALL) $(ID) | cpio -pduv ../pkg
	for i in $(IDENT); \
	do \
		grep -v "^#" $$i.sh | grep -v "^$$" > ../pkg/$$i ;\
	done

clean:	$(FRC)
	
clobber: clean
	rm -f $(IDENT)

FRC:
