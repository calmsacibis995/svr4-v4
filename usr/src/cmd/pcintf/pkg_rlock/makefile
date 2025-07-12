#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pcintf:pkg_rlock/makefile	1.1.1.2"
#! /bin/make -f
# @(#)makefile	1.11	4/15/91 14:30:35
#
# Locus Bridge Products Group - RLOCK Package Makefile
#

PROGS=		rlockshm
LIB_NAME=	librlock.a
LINT_LIB_NAME=	rlock

CFPLUS=		$(CF) -I. -I../pkg_lockset
CFLAGS=		$(DFLAGS) $(CFPLUS) -O
LINTFLAGS=	$(CFPLUS) -Dlint -u -x
LDFLAGS=	$(LF)
LIBFLAGS=	$(LIBF) ./librlock.a ../pkg_lockset/liblset.a
LINTLIBS=	../pkg_lockset/llib-llset.ln

LIB_OBJ=	\
		$(LIB_NAME)(multi_init.o) \
		$(LIB_NAME)(multi_lock.o) \
		$(LIB_NAME)(multi_open.o) \
		$(LIB_NAME)(rlock_estr.o) \
		$(LIB_NAME)(rlock_init.o) \
		$(LIB_NAME)(rlock_prim.o) \
		$(LIB_NAME)(set_cfg.o) \
		$(LIB_NAME)(shm_lock.o) \
		$(LIB_NAME)(shm_open.o) \
		$(LIB_NAME)(state.o)

RLOCKSHM_OBJ=	\
		rlockshm.o

LIB_LINTS=	\
		multi_init.ln \
		multi_lock.ln \
		multi_open.ln \
		rlock_estr.ln \
		rlock_init.ln \
		rlock_prim.ln \
		set_cfg.ln \
		shm_lock.ln \
		shm_open.ln \
		state.ln

RLOCKSHM_LINTS=	\
		rlockshm.ln

# don't let the library be deleted when make gets killed

.PRECIOUS:	$(LIB_NAME)
.SUFFIXES:	.ln

# default target: library

all:		$(LIB_NAME) $(PROGS)
	@echo "\nLocus Bridge Products Group - RLOCK Package is done.\n"

$(LIB_NAME):	$(LIB_OBJ)
	@echo "$(LIB_NAME) updated with:"; echo "\t$?"

# RLOCK shared memory manipulation

rlockshm:	$(RLOCKSHM_OBJ) $(LIB_NAME)
	-$(CC) $(LDFLAGS) -o $@ $(RLOCKSHM_OBJ) $(LIBFLAGS)
	strip $@ 
	-/usr/bin/mcs -d $@ 

# clean out generated files.

clean:
	-rm -f *.o *.ln

clobber:	clean
	-rm -f $(LIB_NAME) $(PROGS)

# lint stuff

.c.ln:
	lint $(LINTFLAGS) -c $<

lint:		$(LIB_LINTS) \
		$(RLOCKSHM_LINTS)
	lint $(LINTFLAGS) -o $(LINT_LIB_NAME) $(LIB_LINTS) $(LINTLIBS)
	@echo "\nLocus Bridge Products Group - RLOCK Package is done.\n"
