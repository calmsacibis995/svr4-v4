#!/sbin/sh
#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:xx/FACE3.2/admbin/PS/WHEEL/mkwheel.sh	1.1.1.1"
echo "fault=$2" > /usr/tmp/chgwheel.$VPID
[ "$2" = "mail" ] && echo "login=$LOGNAME" >> /usr/tmp/chgwheel.$VPID
echo "freq=$3" >> /usr/tmp/chgwheel.$VPID
echo "requests=$4" >> /usr/tmp/chgwheel.$VPID

/bin/diff /usr/tmp/wheel.$VPID /usr/tmp/chgwheel.$VPID 2>/dev/null 1>&2

if [ $? = 0 ]
	then echo 1
	rm -rf /usr/tmp/chgwheel.$VPID
	exit
fi


if [ "$3" = "once" ]
	then freq=0
else
	freq=$3
fi

if [ "$2" = "mail" ] ; then
/usr/lib/lpadmin -S$1 -A"mail $LOGNAME" -W$freq -Q$4  2>/dev/null 1>&2
else
/usr/lib/lpadmin -S$1 -A"$2" -W$freq -Q$4  2>/dev/null 1>&2
fi

rm -rf /usr/tmp/chgwheel.$VPID

echo 0
