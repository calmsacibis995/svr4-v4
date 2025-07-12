#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pcintf:pkg_lockset/makefile	1.1.1.2"
#! /bin/make -f
# @(#)makefile	1.4	4/15/91 14:30:48
#
# Locus Bridge Products Group - LOCKSET Package Makefile
#

LIB_NAME=	liblset.a
LINT_LIB_NAME=	lset
CFPLUS=		$(CF) -I.
CFLAGS=		$(CFPLUS) -O
LINTFLAGS=	$(CFPLUS) -Dlint -u -x

LIB_OBJ=	\
		$(LIB_NAME)(lockset.o) \
		$(LIB_NAME)(lset_estr.o) \
		$(LIB_NAME)(lsetclr.o) \
		$(LIB_NAME)(lsetcount.o) \
		$(LIB_NAME)(lsetnew.o) \
		$(LIB_NAME)(lsetrmv.o) \
		$(LIB_NAME)(lsetset.o) \
		$(LIB_NAME)(lsetuse.o) \
		$(LIB_NAME)(lsetunuse.o)

LIB_LINTS=	\
		lockset.ln \
		lset_estr.ln \
		lsetclr.ln \
		lsetcount.ln \
		lsetnew.ln \
		lsetrmv.ln \
		lsetset.ln \
		lsetuse.ln \
		lsetunuse.ln

# don't let the library be deleted when make gets killed

.PRECIOUS:	$(LIB_NAME)
.SUFFIXES:	.ln

# default target: library

all:		$(LIB_NAME)
	@echo "\nLocus Bridge Products Group - LOCKSET Package is done.\n"

$(LIB_NAME):	$(LIB_OBJ)
	@echo "$(LIB_NAME) updated with:"; echo "\t$?"

# clean out generated files.

clean:
	-rm -f *.o *.ln

clobber:	clean
	-rm -f $(LIB_NAME)

# lint stuff

.c.ln:
	lint $(LINTFLAGS) -c $<

lint:		$(LIB_LINTS)
	lint $(LINTFLAGS) -o $(LINT_LIB_NAME) $(LIB_LINTS)
	@echo "\nLocus Bridge Products Group - LOCKSET Package is linted.\n"
