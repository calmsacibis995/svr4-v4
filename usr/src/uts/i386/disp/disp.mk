#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-dsp:disp.mk	1.3.2.2"

STRIP = strip
INCRT = ..

OFILE = $(CONF)/pack.d/gendisp/Driver.o
TFILE = $(CONF)/pack.d/ts/Driver.o
RFILE = $(CONF)/pack.d/rt/Driver.o
VFILE = $(CONF)/pack.d/vc/Driver.o

DASHG =
DASHO = -O
PFLAGS = $(DASHG) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS) -I$(INCRT)
DEFLIST =
FRC =

FILES = \
	disp.o \
	priocntl.o \
	sysclass.o

all:	$(OFILE) $(TFILE) $(RFILE) $(VFILE)

clean:
	-rm -f $(FILES) rt.o ts.o vc.o gendisp.o

clobber:	clean
	-rm -f $(OFILE) $(RFILE) $(TFILE) $(VFILE)

$(OFILE): $(FILES)
	[ -d $(CONF)/pack.d/gendisp ] || mkdir $(CONF)/pack.d/gendisp; \
	$(LD) -r $(FILES) -o $(OFILE)

$(RFILE):	rt.o
	[ -d $(CONF)/pack.d/rt ] || mkdir $(CONF)/pack.d/rt; \
	$(LD) -r -o $@ rt.o

$(TFILE):	ts.o
	[ -d $(CONF)/pack.d/ts ] || mkdir $(CONF)/pack.d/ts; \
	$(LD) -r -o $@ ts.o

$(VFILE):	vc.o
	[ -d $(CONF)/pack.d/vc ] || mkdir $(CONF)/pack.d/vc; \
	$(LD) -r -o $@ vc.o

#
# Header dependencies
#

disp.o: disp.c \
	$(INCRT)/sys/types.h \
	$(INCRT)/sys/param.h \
	$(INCRT)/sys/sysmacros.h \
	$(INCRT)/sys/fs/s5dir.h \
	$(INCRT)/sys/signal.h \
	$(INCRT)/sys/seg.h \
	$(INCRT)/sys/tss.h \
	$(INCRT)/sys/user.h \
	$(INCRT)/sys/immu.h \
	$(INCRT)/sys/systm.h \
	$(INCRT)/sys/sysinfo.h \
	$(INCRT)/sys/var.h \
	$(INCRT)/sys/errno.h \
	$(INCRT)/sys/cmn_err.h \
	$(INCRT)/sys/cred.h \
	$(INCRT)/sys/proc.h \
	$(INCRT)/sys/procset.h \
	$(INCRT)/sys/debug.h \
	$(INCRT)/sys/inline.h \
	$(INCRT)/sys/priocntl.h \
	$(INCRT)/sys/disp.h \
	$(INCRT)/sys/class.h \
	$(INCRT)/sys/bitmap.h \
	$(INCRT)/sys/kmem.h \
	$(FRC)

priocntl.o: priocntl.c \
	$(INCRT)/sys/types.h \
	$(INCRT)/sys/param.h \
	$(INCRT)/sys/sysmacros.h \
	$(INCRT)/sys/fs/s5dir.h \
	$(INCRT)/sys/signal.h \
	$(INCRT)/sys/tss.h \
	$(INCRT)/sys/user.h \
	$(INCRT)/sys/immu.h \
	$(INCRT)/sys/systm.h \
	$(INCRT)/sys/sysinfo.h \
	$(INCRT)/sys/var.h \
	$(INCRT)/sys/errno.h \
	$(INCRT)/sys/cred.h \
	$(INCRT)/sys/proc.h \
	$(INCRT)/sys/procset.h \
	$(INCRT)/sys/debug.h \
	$(INCRT)/sys/inline.h \
	$(INCRT)/sys/priocntl.h \
	$(INCRT)/sys/disp.h \
	$(INCRT)/sys/class.h \
	$(INCRT)/sys/ts.h \
	$(INCRT)/sys/tspriocntl.h \
	$(FRC)

rt.o: rt.c \
	$(INCRT)/sys/types.h \
	$(INCRT)/sys/param.h \
	$(INCRT)/sys/sysmacros.h \
	$(INCRT)/sys/immu.h \
	$(INCRT)/sys/cred.h \
	$(INCRT)/sys/proc.h \
	$(INCRT)/sys/tss.h \
	$(INCRT)/sys/signal.h \
	$(INCRT)/sys/fs/s5dir.h \
	$(INCRT)/sys/user.h \
	$(INCRT)/sys/evecb.h \
	$(INCRT)/sys/hrtcntl.h \
	$(INCRT)/sys/priocntl.h \
	$(INCRT)/sys/class.h \
	$(INCRT)/sys/disp.h \
	$(INCRT)/sys/procset.h \
	$(INCRT)/sys/cmn_err.h \
	$(INCRT)/sys/debug.h \
	$(INCRT)/sys/rt.h \
	$(INCRT)/sys/rtpriocntl.h \
	$(INCRT)/sys/kmem.h \
	$(INCRT)/sys/systm.h \
	$(INCRT)/sys/inline.h \
	$(INCRT)/sys/errno.h \
	$(FRC)

sysclass.o: sysclass.c \
	$(INCRT)/sys/types.h \
	$(INCRT)/sys/param.h \
	$(INCRT)/sys/sysmacros.h \
	$(INCRT)/sys/fs/s5dir.h \
	$(INCRT)/sys/signal.h \
	$(INCRT)/sys/tss.h \
	$(INCRT)/sys/user.h \
	$(INCRT)/sys/immu.h \
	$(INCRT)/sys/systm.h \
	$(INCRT)/sys/sysinfo.h \
	$(INCRT)/sys/var.h \
	$(INCRT)/sys/errno.h \
	$(INCRT)/sys/cmn_err.h \
	$(INCRT)/sys/proc.h \
	$(INCRT)/sys/debug.h \
	$(INCRT)/sys/inline.h \
	$(INCRT)/sys/disp.h \
	$(INCRT)/sys/class.h \
	$(FRC)

ts.o: ts.c \
	$(INCRT)/sys/types.h \
	$(INCRT)/sys/param.h \
	$(INCRT)/sys/immu.h \
	$(INCRT)/sys/cred.h \
	$(INCRT)/sys/proc.h \
	$(INCRT)/sys/tss.h \
	$(INCRT)/sys/signal.h \
	$(INCRT)/sys/fs/s5dir.h \
	$(INCRT)/sys/user.h \
	$(INCRT)/sys/priocntl.h \
	$(INCRT)/sys/class.h \
	$(INCRT)/sys/disp.h \
	$(INCRT)/sys/procset.h \
	$(INCRT)/sys/debug.h \
	$(INCRT)/sys/ts.h \
	$(INCRT)/sys/tspriocntl.h \
	$(INCRT)/sys/evecb.h \
	$(INCRT)/sys/hrtcntl.h \
	$(INCRT)/sys/kmem.h \
	$(INCRT)/sys/systm.h \
	$(INCRT)/sys/inline.h \
	$(INCRT)/sys/errno.h \
	$(FRC)

vc.o: vc.c \
	$(INCRT)/sys/types.h \
	$(INCRT)/sys/param.h \
	$(INCRT)/sys/immu.h \
	$(INCRT)/sys/cred.h \
	$(INCRT)/sys/proc.h \
	$(INCRT)/sys/tss.h \
	$(INCRT)/sys/signal.h \
	$(INCRT)/sys/fs/s5dir.h \
	$(INCRT)/sys/user.h \
	$(INCRT)/sys/priocntl.h \
	$(INCRT)/sys/class.h \
	$(INCRT)/sys/disp.h \
	$(INCRT)/sys/procset.h \
	$(INCRT)/sys/debug.h \
	$(INCRT)/sys/vc.h \
	$(INCRT)/sys/vcpriocntl.h \
	$(INCRT)/sys/evecb.h \
	$(INCRT)/sys/hrtcntl.h \
	$(INCRT)/sys/kmem.h \
	$(INCRT)/sys/systm.h \
	$(INCRT)/sys/inline.h \
	$(INCRT)/sys/errno.h \
	$(FRC)

