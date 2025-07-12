#ident	"@(#)doctools:index/index.mk	1.1"
# makefile for index files -- nothing here to change

MAKEFILE=index.mk
ARGS=all
TARGETS=index
INDEXDIR=$(LIBDIR)/INDEX
INDEXFILES=add.dash\
	addfields\
	doclean\
	formati\
	get.books\
	get.nums\
	get.ranges\
	index.head\
	make.key\
	range.prep\
	range.sort\
	subitems

all :	$(TARGETS)

index :	index.src $(INDEXFILES)
	@echo "IBIN=$(INDEXDIR)" > $@
	@cat index.src >> $@
	@echo "--- Created file: index/$@ ---"
	
install :	$(TARGETS)
	@if [ ! -d "$(BINDIR)" ]; then \
		$(MAKEDIR) $(BINDIR); \
		chmod 755 $(BINDIR); \
		chgrp $(GROUP) $(BINDIR); \
		chown $(OWNER) $(BINDIR); \
	fi
	@if [ -f $(BINDIR)/index -a -w $(BINDIR)/index ]; then \
		echo "--- Can't move index to $(BINDIR) ---"; \
		echo "--- Read-only version exists there ---"; \
	else \
		cp index $(BINDIR); \
		chmod 555 $(BINDIR)/index; \
		chgrp $(GROUP) $(BINDIR)/index; \
		chown $(OWNER) $(BINDIR)/index; \
		echo "--- Installed index in $(BINDIR) ---"; \
	fi
	@if [ ! -d "$(INDEXDIR)" ]; then \
		$(MAKEDIR) $(INDEXDIR); \
		chmod 755 $(INDEXDIR); \
		chgrp $(GROUP) $(INDEXDIR); \
		chown $(OWNER) $(INDEXDIR); \
	fi
	@cp $(INDEXFILES) $(INDEXDIR)
	@chmod 555 $(INDEXDIR)/*
	@chgrp $(GROUP) $(INDEXDIR)/*
	@chown $(OWNER) $(INDEXDIR)/*
	@echo "--- Installation of indexing tools is complete ---"

clean :
	rm -f $(TARGETS)

# EOF
