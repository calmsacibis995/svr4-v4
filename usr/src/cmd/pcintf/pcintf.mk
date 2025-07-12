#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pcintf:pci.mk	1.1.1.2"
#	Copyright (c) 1989  Locus Computing Corporation
#	All Rights Reserved
#
#	@(#)pci.mk	1.6	4/15/91	14:40:21

ETC=$(ROOT)/etc
USR=$(ROOT)/usr
USRBIN=$(USR)/bin
#ADDON=$(USR)/src/add-on
PCI=$(USR)/pci
PCIBIN=$(PCI)/bin
PCILIB=$(PCI)/lib
DOSMSG=$(PCI)/dosmsg
SPOOL=$(USR)/spool
PCILOG=$(SPOOL)/pcilog

all:
	cd bridge; make intelsvr4eth; make intelsvr4232
	cd   util; make

install: all
	-rm -rf $(PCI) $(PCILOG)
	-rm -f $(USRBIN)/dos2unix $(USRBIN)/unix2dos $(ETC)/rc[0-2].d/[SK]95pci
	[ -d $(USR)		 ] || mkdir $(USR)
	[ -d $(USR)/src		 ] || mkdir $(USR)/src
	#[ -d $(ADDON)		 ] || mkdir $(ADDON)
	#[ -d $(ADDON)/pkg.pci	 ] || mkdir $(ADDON)/pkg.pci
	#[ -d $(ADDON)/pkg.pcieth ] || mkdir $(ADDON)/pkg.pcieth
	[ -d $(ETC)		 ] || mkdir $(ETC)
	[ -d $(ETC)/rc0.d	 ] || mkdir $(ETC)/rc0.d
	[ -d $(ETC)/rc1.d	 ] || mkdir $(ETC)/rc1.d
	[ -d $(ETC)/rc2.d	 ] || mkdir $(ETC)/rc2.d
	[ -d $(PCI)		 ] || mkdir $(PCI)
	[ -d $(PCIBIN)		 ] || mkdir $(PCIBIN)
	[ -d $(PCILIB)		 ] || mkdir $(PCILIB)
	[ -d $(DOSMSG)		 ] || mkdir $(DOSMSG)
	[ -d $(USRBIN)		 ] || mkdir $(USRBIN)
	[ -d $(SPOOL)		 ] || mkdir $(SPOOL)
	[ -d $(PCILOG)		 ] || mkdir $(PCILOG)
	chmod 775 $(PCILOG)
	#cp install/intelsvr4/copyright			$(ADDON)/pkg.pci
	#ln $(ADDON)/pkg.pci/copyright			$(ADDON)/pkg.pcieth
	#cp install/intelsvr4/pkg.pci/depend		$(ADDON)/pkg.pci
	#cp install/intelsvr4/pkg.pci/pkginfo		$(ADDON)/pkg.pci
	#cp install/intelsvr4/pkg.pci/prototype		$(ADDON)/pkg.pci
	#cp install/intelsvr4/pkg.pcieth/depend		$(ADDON)/pkg.pcieth
	#cp install/intelsvr4/pkg.pcieth/pkginfo		$(ADDON)/pkg.pcieth
	#cp install/intelsvr4/pkg.pcieth/postinstall	$(ADDON)/pkg.pcieth
	#cp install/intelsvr4/pkg.pcieth/prototype	$(ADDON)/pkg.pcieth
	#cp install/intelsvr4/pkg.pcieth/request		$(ADDON)/pkg.pcieth
	cp support/intelsvr4/errlogger			$(PCI)
	cp support/intelsvr4/pcistart			$(PCIBIN)
	cp support/intelsvr4/pcistop			$(PCIBIN)
	cp support/intelsvr4/pciprint			$(PCIBIN)
	cp support/intelsvr4/pciptys			$(PCI)
	cp support/intelsvr4/S95pci			$(ETC)/rc2.d
	ln $(ETC)/rc2.d/S95pci				$(ETC)/rc0.d/K95pci
	ln $(ETC)/rc2.d/S95pci				$(ETC)/rc1.d/K95pci
	cp bridge/intelsvr4eth.dir/loadpci		$(PCI)/loadpci
	cp bridge/intelsvr4eth.dir/consvr		$(PCI)/pciconsvr.eth
	cp bridge/intelsvr4eth.dir/dossvr		$(PCI)/pcidossvr.eth
	cp bridge/intelsvr4eth.dir/dosout		$(PCI)/pcidosout.eth
	cp bridge/intelsvr4eth.dir/mapsvr		$(PCI)/pcimapsvr.eth
	cp bridge/intelsvr4eth.dir/En.lmf		$(DOSMSG)
	cp pkg_lcs/pc437.lcs				$(PCILIB)
	cp pkg_lcs/pc850.lcs				$(PCILIB)
	cp pkg_lcs/8859.lcs				$(PCILIB)
	cp pkg_lmf/lmfmsg				$(PCIBIN)
	cp bridge/intelsvr4232.dir/dossvr		$(PCI)/pcidossvr.232
	cp bridge/intelsvr4232.dir/dosout		$(PCI)/pcidosout.232
	cp pkg_rlock/rlockshm				$(PCI)/sharectl
	cp util/charconv				$(USRBIN)
	ln $(USRBIN)/charconv				$(USRBIN)/dos2unix
	ln $(USRBIN)/charconv				$(USRBIN)/unix2dos

clean:
	cd util;	make clean
	cd pkg_lmf;	make clean
	cd pkg_lcs;	make clean
	cd pkg_rlock;	make clean
	cd pkg_lockset;	make clean
	-for i in bridge/*.dir ;	\
	do				\
		rm -f $$i/*.o ;		\
	done

clobber: clean
	cd util;	make clobber
	cd pkg_lmf;	make clobber
	cd pkg_lcs;	make clobber
	cd pkg_rlock;	make clobber
	cd pkg_lockset;	make clobber
	-rm -rf bridge/*.dir
