#!
#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mbus:cmd/initpkg/inittab.sh	1.3.1.1"

echo "# /etc/inittab on AT&T 286/386 processors is built by Installable
# Drivers (ID) each time the kernel is rebuilt. /etc/inittab is replaced
# by /etc/conf/cf.d/init.base appended with the component files in
# the /etc/conf/init.d directory by the /etc/conf/bin/idmkinit command.
ap::sysinit:/sbin/autopush -f /etc/ap/chan.ap
ak::sysinit:/usr/bin/sleep 120 </dev/console >/dev/console 2>&1 &
ck::sysinit:/sbin/setclk </dev/console >/dev/sysmsg 2>&1
bchk::sysinit:/sbin/bcheckrc </dev/console >/dev/sysmsg 2>&1
brc::bootwait:/sbin/brc 1> /dev/sysmsg 2>&1
mt:23:bootwait:/sbin/brc </dev/console >/dev/sysmsg 2>&1
is:2:initdefault:
r0:0:wait:/sbin/rc0 off 1> /dev/sysmsg 2>&1 </dev/console
r1:1:wait:/sbin/rc1  1> /dev/sysmsg 2>&1 </dev/console
r2:23:wait:/sbin/rc2 1> /dev/sysmsg 2>&1 </dev/console
r3:3:wait:/sbin/rc3  1> /dev/sysmsg 2>&1 </dev/console
r5:5:wait:/sbin/rc0 reboot 1> /dev/sysmsg 2>&1 </dev/console
r6:6:wait:/sbin/rc6 reboot 1> /dev/sysmsg 2>&1 </dev/console
sd:0:wait:/sbin/uadmin 2 0 >/dev/sysmsg 2>&1 </dev/console
fw:5:wait:/sbin/uadmin 2 2 >/dev/sysmsg 2>&1 </dev/console
rb:6:wait:/sbin/uadmin 2 1 >/dev/sysmsg 2>&1 </dev/console
li:23:wait:/usr/bin/ln /dev/systty /dev/syscon >/dev/null 2>&1
sc:234:respawn:/usr/lib/saf/sac -t 300
co:12345:respawn:/usr/lib/saf/ttymon -g -h -p \"Console Login: \" -d /dev/console -l console
" >inittab
