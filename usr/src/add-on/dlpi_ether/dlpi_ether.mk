#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dlpi_ether:dlpi_ether.mk	1.1"

ADDON =		dlpi_ether
FRC=

all \
package \
clean:
	@for comp in */*.mk */Makefile ; \
	do \
		echo $$comp ; \
		(cd `dirname $$comp`; $(MAKE) -f `basename $$comp` "FRC=$(FRC)" $@); \
	done
	@echo "\n\t$(ADDON) build completed"

install:
	[ -d pkg ] || mkdir pkg
	@for comp in */*.mk */Makefile ; \
	do \
		echo $$comp ; \
		(cd `dirname $$comp`; $(MAKE) -f `basename $$comp` "FRC=$(FRC)" $@); \
	done
	@echo "\n\t$(ADDON) build completed"

clobber:
	rm -rf pkg
	@for comp in */*.mk */Makefile ; \
	do \
		echo $$comp ; \
		(cd `dirname $$comp`; $(MAKE) -f `basename $$comp` "FRC=$(FRC)" $@); \
	done
	@echo "\n\t$(ADDON) build completed"

FRC:
