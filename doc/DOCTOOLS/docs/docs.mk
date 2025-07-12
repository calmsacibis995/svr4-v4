#ident	"@(#)doctools:docs/docs.mk	1.1"
# makefile for index files -- nothing here to change

MAKEFILE=index.mk
ARGS=all
TARGETS=uguide manpages
UGUIDEFILES=MasterToc\
	Cchap1\
	chap1\
	Cchap2\
	chap2\
	Cchap3\
	chap3\
	Cchap4\
	chap4\
	Cchap5\
	chap5\
	Cchap6\
	chap6\
	Cindex\
	index\
	Crefcard\
	refcard

MANPAGEFILES=index.1\
	format.1\
	deltamacs.1\
	checkmacs.1

all :	$(TARGETS)

uguide :	$(UGUIDEFILES)
	
manpages :	$(MANPAGEFILES)

install :	$(TARGETS)
	@if [ ! -d "$(MANDIR)" ]; then \
		$(MAKEDIR) $(MANDIR); \
		chmod 755 $(MANDIR); \
		chgrp $(GROUP) $(MANDIR); \
		chown $(OWNER) $(MANDIR); \
	fi
	@if [ ! -d "$(DOCDIR)" ]; then \
		$(MAKEDIR) $(DOCDIR); \
		chmod 755 $(DOCDIR); \
		chgrp $(GROUP) $(DOCDIR); \
		chown $(OWNER) $(DOCDIR); \
	fi
	@cp $(UGUIDEFILES) $(DOCDIR)
	@chmod 444 $(DOCDIR)/*
	@chgrp $(GROUP) $(DOCDIR)/*
	@chown $(OWNER) $(DOCDIR)/*
	@cp $(MANPAGEFILES) $(MANDIR)
	@chmod 444 $(MANDIR)/*
	@chgrp $(GROUP) $(MANDIR)/*
	@chown $(OWNER) $(MANDIR)/*
	@echo "--- Installation of documentation is complete ---"
	@echo "\n--- DOCTOOLS Installation is Complete ---\n"

clean :

# EOF
