/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)master:udp/space.c	1.3.2.2"

#define NUDP	512

unsigned char	udp_dev[(NUDP+7)/8];
int		nudp = NUDP;

/* udpcksum = 1 to enable checksum */
int		udpcksum = 1;
