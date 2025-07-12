#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:dosutil/dosformat/dosformat.mk	1.5"

LEX = lex
DIR = $(ROOT)/bin
INC = $(ROOT)/usr/include
INS = install
INSDIR = $(ROOT)/usr/bin
INSDIR2 = $(ROOT)/etc/default

AR =  ar

CFLAGS = -O -I$(INC)
LDFLAGS = -s $(SHLIBS)

MAKEFILE = dosformat.mk

LIB=libdos.a
DOSOBJECTS=$(CFILES:.c=.o)
CMDS=dosformat  

all:	$(LIB) $(CMDS)


$(INSDIR) $(INSDIR2):
	-mkdir $@
	-$(CH) chmod 755 $@
	-$(CH) chown bin $@
	-$(CH) chgrp bin $@

CFILES=MS-DOS.c \
	add_device.c \
	alloc_clust.c \
	basename.c \
	chain_clust.c \
	close_device.c \
	critical.c \
	del_label.c \
	dos_fil_size.c \
	dos_mod_date.c \
	dos_mod_time.c \
	fix_slash.c \
	free_space.c \
	get_assign.c \
	is_dir_empty.c \
	loc_free_dir.c \
	locate.c \
	lookup_dev.c \
	lookup_drv.c \
	make_label.c \
	mkdir.c \
	my_fgets.c \
	next_cluster.c \
	next_sector.c \
	open_device.c \
	parse_name.c \
	read_sector.c \
	rm_file.c \
	scan_dos_dir.c \
	strupr.c \
	write_fat.c \
	write_sector.c


dosformat:	$(LIB) dosformat.c
	$(CC) $(CFLAGS) -o dosformat dosformat.c $(LIB) $(LDFLAGS)

libdos.a:	$(DOSOBJECTS)
	$(AR) rv $(LIB) $?

$(DOSOBJECTS):	MS-DOS.h

install:	$(INSDIR) $(INSDIR2) all
	$(INS) -f $(INSDIR) -m 0711 -u bin -g dos dosformat
 
clean:
	rm -f *.o  $(DOSOBJECTS)

clobber: clean
	rm -f $(DOSOBJECTS) 
	rm -f $(CMDS) $(LIB)


