#ident	"@(#)doctools:shells/shells.mk	1.1"
# makefile for shell files -- nothing here to change

MAKEFILE=shells.mk
ARGS=all
TARGETS=defs.h format functions init deltamacs

all :	$(TARGETS)

defs.h :	defs.h.src
	@if [ "$(STRIP)" = "Y" ]; then \
		sed '/^#/ !s/[ 	]#[ 	].*//' $@.src > $@; \
	else \
		cat $@.src > $@; \
	fi
	@echo "--- Created file: shells/$@ ---"
	
format :	format.src
	@echo "LIB=$(LIBDIR)" > $@
	@if [ "$(STRIP)" = "Y" ]; then \
		sed '/^#/ !s/[ 	]#[ 	].*//' $@.src >> $@; \
	else \
		cat $@.src >> $@; \
	fi
	@echo "--- Created file: shells/$@ ---"
	
functions :	functions.src
	@if [ "$(STRIP)" = "Y" ]; then \
		sed '/^#/ !s/[ 	]#[ 	].*//' $@.src > $@; \
	else \
		cat $@.src > $@; \
	fi
	@echo "--- Created file: shells/$@ ---"
	
init :	init.src
	@echo "BIN=$(BINDIR)" > $@
	@echo "SYS=$(SYSINCLUDEDIR)" >> $@
	@echo ": \$${SCCSSRC:=$(SYSSCCSDIR)}" >> $@
	@echo ": \$${ARCSRC:=$(OLDSYSSCCSDIR)}" >> $@
	@if [ "$(STRIP)" = "Y" ]; then \
		sed '/^#/ !s/[ 	]#[ 	].*//' $@.src >> $@; \
	else \
		cat $@.src >> $@; \
	fi
	@echo "--- Created file: shells/$@ ---"
	
deltamacs :	deltamacs.src
	@if [ "$(STRIP)" = "Y" ]; then \
		sed '/^#/ !s/[ 	]#[ 	].*//' $@.src > $@; \
	else \
		cat $@.src > $@; \
	fi

install :	$(TARGETS)
	@if [ ! -d "$(LIBDIR)" ]; then \
		$(MAKEDIR) $(LIBDIR); \
		chmod 755 $(LIBDIR); \
		chgrp $(GROUP) $(LIBDIR); \
		chown $(OWNER) $(LIBDIR); \
	fi
	@if [ ! -d "$(BINDIR)" ]; then \
		$(MAKEDIR) $(BINDIR); \
		chmod 755 $(BINDIR); \
		chgrp $(GROUP) $(BINDIR); \
		chown $(OWNER) $(BINDIR); \
	fi
	@if [ -f $(LIBDIR)/defs.h -a -w $(LIBDIR)/defs.h ]; then \
		echo "--- Can't move defs.h to $(LIBDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp defs.h $(LIBDIR); \
		chmod 444 $(LIBDIR)/defs.h; \
		chgrp $(GROUP) $(LIBDIR)/defs.h; \
		chown $(OWNER) $(LIBDIR)/defs.h; \
		echo "--- Installed defs.h in $(LIBDIR) ---"; \
	fi
	@if [ -f $(LIBDIR)/functions -a -w $(LIBDIR)/functions ]; then \
		echo "--- Can't move functions to $(LIBDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp functions $(LIBDIR); \
		chmod 444 $(LIBDIR)/functions; \
		chgrp $(GROUP) $(LIBDIR)/functions; \
		chown $(OWNER) $(LIBDIR)/functions; \
		echo "--- Installed functions in $(LIBDIR) ---"; \
	fi
	@if [ -f $(LIBDIR)/init -a -w $(LIBDIR)/init ]; then \
		echo "--- Can't move init to $(LIBDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp init $(LIBDIR); \
		chmod 444 $(LIBDIR)/init; \
		chgrp $(GROUP) $(LIBDIR)/init; \
		chown $(OWNER) $(LIBDIR)/init; \
		echo "--- Installed init in $(LIBDIR) ---"; \
	fi
	@if [ -f $(BINDIR)/format -a -w $(BINDIR)/format ]; then \
		echo "--- Can't move format to $(BINDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp format $(BINDIR); \
		chmod 555 $(BINDIR)/format; \
		chgrp $(GROUP) $(BINDIR)/format; \
		chown $(OWNER) $(BINDIR)/format; \
		echo "--- Installed format in $(BINDIR) ---"; \
	fi
	@if [ -f $(BINDIR)/deltamacs -a -w $(BINDIR)/format ]; then \
		echo "--- Can't move deltamacs to $(BINDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp deltamacs $(BINDIR); \
		chmod 555 $(BINDIR)/deltamacs; \
		chgrp $(GROUP) $(BINDIR)/deltamacs; \
		chown $(OWNER) $(BINDIR)/deltamacs; \
		echo "--- Installed deltamacs in $(BINDIR) ---"; \
	fi

clean :
	rm -f $(TARGETS)

# EOF
