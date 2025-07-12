#!/sbin/sh
#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sco:tape.sh	1.1.1.2"
#
# Enhanced Application Compatibility Support
# This shell script is intended to behave like SCO's tape command.
# The tape command is implemented as an interface to USL's tapecntl command.
# The following SCO's tape command options are not supported.
#		status	- get the status of the tape 
#		amount	- amount of current or last transfer 
#		wfm	- write filemark 
# Also, options and arguments related to the QIC-40 and QIC-80 minitapes
# are not supported.
#
# Name: tape
# Usage:
#	tape usage: tape [-<type>] <command> [device]
#	  type: c (non-SCSI) or s (SCSI)
#	  Cartridge tape commands:
#		reten	- retension the tape
#		erase	- erase and retension tape
#		reset	- reset controller and drive
#		rewind	- rewind tape controller
#		rfm	- skip to next file
#
# exit codes
#	1: Cartridge Tape Utilities (qt) add-on pkg. not installed.
#	2: Specified device does not exist
#	3: Illegal options/arguments
#	4: tape device not specified and there is no default one

TAPECNTL="/usr/lib/tape/tapecntl"
DFLT_DEVICE=""
TYPE=""
TYPE_OPTION=""
command=""
USAGE="\ntape usage: tape [-<type>] <command> [device]\n\
	  type: c (non-SCSI) or s (SCSI)\n\
	  Cartridge tape commands:\n\
	\treten\t- retension the tape\n\
	\terase\t- erase and retension tape\n\
	\treset\t- reset controller and drive\n\
	\trewind\t- rewind tape controller\n\
	\trfm\t- skip to next file\n"

# Check if a cartridge tape driver is present 

if   [ -d /etc/conf/pack.d/qt ] 
then
	if [ `grep qt /etc/conf/cf.d/sdevice|cut -f2` != "Y" ]
	then exit 1
	fi
	
	TYPE="non-scsi"
	DFLT_DEVICE="/dev/rmt/c0s0"
elif  [ -d /etc/conf/pack.d/scsi ]
then
	TYPE="scsi"
	DFLT_DEVICE="/dev/rmt/c0t3d0s0"
else
	echo "ERROR: To use the command $0, you need to first install"
	echo "\tCartridge Tape Utilities add-on package."
	exit 1
fi

# Check if the default tape file exists
if [ -f /etc/default/tape ]
then
	DFLT_DEVICE=`grep device /etc/default/tape | cut -f2 -d "="` 
fi
TAPEDEVICE=${DFLT_DEVICE}

if [ $# = 0 ]
then 	echo ${USAGE} 
	exit 3
fi

set -- `getopt c:s:a: $*` 

if [ $? -ne 0 ]
then	echo ${USAGE}
	exit 3
fi

for i in $*
do
	case $i in
	-c )	
		if [ "${TYPE}" = "scsi" ]
		then
			echo "for a SCSI tape: use -s"
			echo ${USAGE}
			exit 3
		elif [ "${TYPE_OPTION}" = "-s" ] || [ ${2} = "-c" ] || [ ${2} = "-s" ]
		then
			echo ${USAGE}
			exit 3
		else
			TYPE_OPTION="-c"
			command=${2}
			shift 2 2> /dev/null
		fi
		;;
	-s )	
		if [ "${TYPE}" = "non-scsi" ]
		then
			echo "for a non-SCSI tape use -c"
			echo ${USAGE}
			exit 3
		elif [ "${TYPE_OPTION}" = "-c" ] || [ ${2} = "-c" ] || [ ${2} = "-s" ]
		then
			echo ${USAGE}
			exit 3
		else
			TYPE_OPTION="-s"
			command=${2}
			shift 2 2> /dev/null
		fi
		;;
 	-- )
		shift 2> /dev/null
		break
		;;
	esac
done

if [ "${command}" = "" ]
then 	command=${1}
	shift 2> /dev/null
fi

case $# in 
	0)
		if [ "${TAPEDEVICE}" = "" ]
		then
			echo ${USAGE}
			exit 4
		fi
		;;
	1)
		TAPEDEVICE=${1}
		;;
	*)	echo ${USAGE}
		exit 3
esac

if [ ! -r ${TAPEDEVICE} ]
then 	echo "$0: can't open '${TAPEDEVICE}': no such device"
	echo ${USAGE}
	exit 2
fi

case ${command} in 
	reten)
		$TAPECNTL -t $TAPEDEVICE
		;;
	erase)
		$TAPECNTL -e $TAPEDEVICE
		;;
	reset)
		$TAPECNTL -r $TAPEDEVICE
		;;
	rewind)
		$TAPECNTL -w $TAPEDEVICE
		;;
	rfm)
		$TAPECNTL -p 1 ${TAPEDEVICE}n
		;;
	*)
		echo ${USAGE}
		exit 3
		;;
esac
# End Enhanced Application Compatibility Support
