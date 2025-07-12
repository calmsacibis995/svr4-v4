#ident	"@(#)doctools:macros/macros.mk	1.1"
# makefile for macros files -- nothing here to change

MAKEFILE=macros.mk
ARGS=all
TARGETS=gen gmt macros.h rmt

all :	$(TARGETS)

gen :	gen.src
	@if [ "$(STRIP)" = "Y" ]; then \
		sed  '/^\.\\"/d;s/[ 	]\\".*//' $@.src > $@; \
	else \
		cat $@.src > $@; \
	fi
	@echo .so $(LIBDIR)/macros.h >> $@
	@echo "--- Created file: macros/$@ ---"
	
gmt :	gmt.src
	@if [ "$(STRIP)" = "Y" ]; then \
		sed  '/^\.\\"/d;s/[ 	]\\".*//' $@.src > $@; \
	else \
		cat $@.src > $@; \
	fi
	@echo ".so $(LIBDIR)/gen" >> $@
	@echo "--- Created file: macros/$@ ---"
	
macros.h :	macros.h.src
	@if [ "$(STRIP)" = "Y" ]; then \
		sed  '/^\.\\"/d;s/[ 	]\\".*//' $@.src > $@; \
	else \
		cat $@.src > $@; \
	fi
	@echo "--- Created file: macros/$@ ---"
	
rmt :	rmt.src
	@if [ "$(STRIP)" = "Y" ]; then \
		sed  '/^\.\\"/d;s/[ 	]\\".*//' $@.src > $@; \
	else \
		cat $@.src > $@; \
	fi
	@echo ".so $(LIBDIR)/gen" >> $@
	@echo "--- Created file: macros/$@ ---"
	
install :	$(TARGETS)
	@if [ ! -d "$(LIBDIR)" ]; then \
		$(MAKEDIR) $(LIBDIR); \
		chmod 755 $(LIBDIR); \
		chgrp $(GROUP) $(LIBDIR); \
		chown $(OWNER) $(LIBDIR); \
	fi
	@if [ -f $(LIBDIR)/gen -a -w $(LIBDIR)/gen ]; then \
		echo "--- Can't move gen to $(LIBDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp gen $(LIBDIR); \
		chmod 444 $(LIBDIR)/gen; \
		chgrp $(GROUP) $(LIBDIR)/gen; \
		chown $(OWNER) $(LIBDIR)/gen; \
		echo "--- Installed gen in $(LIBDIR) ---"; \
	fi
	@if [ -f $(LIBDIR)/gmt -a -w $(LIBDIR)/gmt ]; then \
		echo "--- Can't move gmt to $(LIBDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp gmt $(LIBDIR); \
		chmod 444 $(LIBDIR)/gmt; \
		chgrp $(GROUP) $(LIBDIR)/gmt; \
		chown $(OWNER) $(LIBDIR)/gmt; \
		echo "--- Installed gmt in $(LIBDIR) ---"; \
	fi
	@if [ -f $(LIBDIR)/macros.h -a -w $(LIBDIR)/macros.h ]; then \
		echo "--- Can't move macros.h to $(LIBDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp macros.h $(LIBDIR); \
		chmod 444 $(LIBDIR)/macros.h; \
		chgrp $(GROUP) $(LIBDIR)/macros.h; \
		chown $(OWNER) $(LIBDIR)/macros.h; \
		echo "--- Installed macros.h in $(LIBDIR) ---"; \
	fi
	@if [ -f $(LIBDIR)/rmt -a -w $(LIBDIR)/rmt ]; then \
		echo "--- Can't move rmt to $(LIBDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp rmt $(LIBDIR); \
		chmod 444 $(LIBDIR)/rmt; \
		chgrp $(GROUP) $(LIBDIR)/rmt; \
		chown $(OWNER) $(LIBDIR)/rmt; \
		echo "--- Installed rmt in $(LIBDIR) ---"; \
	fi

clean :
	rm -f $(TARGETS)

# EOF
