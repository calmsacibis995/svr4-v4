#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dlpi_ether:ie6/ID/Makefile	1.1"

STRIPFILES =	Master System Node
OTHERFILES =	Space.c
ADDON =		ie6
PKGDIR =	../../pkg/$(ADDON)
FILES =	$(STRIPFILES) $(OTHERFILES)

all:	$(FILES)

install:	all
	[ -d $(PKGDIR) ] || mkdir $(PKGDIR)
	@for i in $(STRIPFILES) ; \
	do \
		grep -v "^\*" $$i | grep -v "^$$" > $(PKGDIR)/$$i ; \
	done
	cp $(OTHERFILES) $(PKGDIR)

clean:
	@for i in $(STRIPFILES) $(OTHERFILES); \
	do \
		rm -f $(PKGDIR)/$$i ; \
	done

clobber: clean
