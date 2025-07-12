#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dlpi_ether:ie6/sys/Makefile	1.1"

ADDON =		 ie6
PKGDIR =	../../pkg/$(ADDON)
FILES =		../../sys/dlpi_ether.h ie6.h

all:	$(FILES)

install: all
	[ -d $(PKGDIR)/sys ] || mkdir $(PKGDIR)/sys
	@for h in $(FILES); \
	do \
	cp $$h $(PKGDIR)/sys ; \
	done

clean:
	@for h in $(FILES) ; \
	do \
	rm -rf $(PKGDIR)/sys/`basename $$h` ; \
	done

clobber: clean
