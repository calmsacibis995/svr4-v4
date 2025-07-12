#ident	"@(#)doctools:cfiles/cfiles.mk	1.2"
#
#       Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#       Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#         All Rights Reserved
#
#       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#       UNIX System Laboratories, Inc.
#       The copyright notice above does not evidence any
#       actual or intended publication of such source code.
#
#       makefile for C programs
#
#     Modified by: M. Shapiro
#                 UNIX System Laboratories
#                 attunix!mxs   201-522-5181
#                   
#
# makefile for C files

MAKEFILE=cfiles.mk
ARGS=all
TARGETS=prep checkmacs vsresolve
CFLAGS=-O

all :	$(TARGETS)

prep :	prep.c
	@$(CC) $(CFLAGS) -o $@ $@.c
	@echo "--- Created file: cfiles/$@ ---"
	
checkmacs :	checkmacs.c
	@$(CC) $(CFLAGS) -o $@ $@.c
	@echo "--- Created file: cfiles/$@ ---"
	
vsresolve :	vsresolve.c
	@$(CC) $(CFLAGS) -o $@ $@.c
	@echo "--- Created file: cfiles/$@ ---"
	
install :	$(TARGETS)
	@if [ ! -d "$(BINDIR)" ]; then \
		$(MAKEDIR) $(BINDIR); \
		chmod 755 $(BINDIR); \
		chgrp $(GROUP) $(BINDIR); \
		chown $(OWNER) $(BINDIR); \
	fi
	@if [ ! -d "$(LIBDIR)" ]; then \
		$(MAKEDIR) $(LIBDIR); \
		chmod 755 $(LIBDIR); \
		chgrp $(GROUP) $(LIBDIR); \
		chown $(OWNER) $(LIBDIR); \
	fi
	@if [ -f $(LIBDIR)/prep -a -w $(LIBDIR)/prep ]; then \
		echo "--- Can't move prep to $(LIBDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp prep $(LIBDIR); \
		strip $(LIBDIR)/prep; \
		chmod 555 $(LIBDIR)/prep; \
		chgrp $(GROUP) $(LIBDIR)/prep; \
		chown $(OWNER) $(LIBDIR)/prep; \
		echo "--- Installed prep in $(LIBDIR) ---"; \
	fi
	@if [ -f $(BINDIR)/checkmacs -a -w $(BINDIR)/checkmacs ]; then \
		echo "--- Can't move checkmacs to $(BINDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp checkmacs $(BINDIR); \
		strip $(BINDIR)/checkmacs; \
		chmod 555 $(BINDIR)/checkmacs; \
		chgrp $(GROUP) $(BINDIR)/checkmacs; \
		chown $(OWNER) $(BINDIR)/checkmacs; \
		echo "--- Installed checkmacs in $(BINDIR) ---"; \
	fi
	@if [ -f $(LIBDIR)/vsresolve -a -w $(BINDIR)/vsresolve ]; then \
		echo "--- Can't move vsresolve to $(LIBDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp vsresolve $(LIBDIR); \
		strip $(LIBDIR)/vsresolve; \
		chmod 555 $(LIBDIR)/vsresolve; \
		chgrp $(GROUP) $(LIBDIR)/vsresolve; \
		chown $(OWNER) $(LIBDIR)/vsresolve; \
		echo "--- Installed vsresolve in $(LIBDIR) ---"; \
	fi

clean :
	rm -f $(TARGETS)
