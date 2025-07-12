#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pcintf:pkg_lcs/Makefile	1.1"
# SCCSID(@(#)Makefile	4.8	LCC)	/* Modified: 14:26:04 4/19/90 */
#
#  Makefile for LMF Message File Library
#
DIR=.

all: lcsgen lcsdump liblcs.a \
	$(DIR)/10646.lcs $(DIR)/6937.lcs \
	$(DIR)/8859.lcs  $(DIR)/euc-jis.lcs \
	$(DIR)/pc437.lcs $(DIR)/pc850.lcs \
	$(DIR)/sjis.lcs

lcsgen: lcsgen.o stricmp.o
	$(CC) $(CFLAGS) -o lcsgen lcsgen.o stricmp.o

lcsdump: lcsdump.o liblcs.a
	$(CC) $(CFLAGS) -o lcsdump lcsdump.o liblcs.a

liblcs.a: liblcs.a(get_tab.o) liblcs.a(set_tab.o) liblcs.a(prim.o) \
	  liblcs.a(t_string.o) liblcs.a(t_block.o) liblcs.a(caseconv.o)

lcsgen.o: lcs.h lcs_int.h

lcsdump.o: lcs.h lcs_int.h

get_tab.o: lcs.h lcs_int.h

set_tab.o: lcs.h lcs_int.h

prim.o: lcs.h lcs_int.h

$(DIR)/10646.lcs: 10646.cs lcsgen
	./lcsgen 10646.cs $@
$(DIR)/6937.lcs: 6937.cs lcsgen
	./lcsgen 6937.cs $@
$(DIR)/8859.lcs: 8859.cs lcsgen
	./lcsgen 8859.cs $@
$(DIR)/euc-jis.lcs: euc-jis.cs lcsgen
	./lcsgen euc-jis.cs $@
$(DIR)/pc437.lcs: pc437.cs lcsgen
	./lcsgen pc437.cs $@
$(DIR)/pc850.lcs: pc850.cs lcsgen
	./lcsgen pc850.cs $@
$(DIR)/sjis.lcs: sjis.cs lcsgen
	./lcsgen sjis.cs $@

clean:
	rm -f *.o

clobber: clean
	rm -f lcsgen lcsdump liblcs.a
	rm -f $(DIR)/10646.lcs $(DIR)/6937.lcs
	rm -f $(DIR)/8859.lcs  $(DIR)/euc-jis.lcs
	rm -f $(DIR)/pc437.lcs $(DIR)/pc850.lcs
	rm -f $(DIR)/sjis.lcs

