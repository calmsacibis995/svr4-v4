/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:i386/packet.h	1.1"
/*
* i386/packet.h - i386 tuning for assembler numeric package
*
* Depends on:
*	"common/as.h"
*
* Only included from "common/nums.c"
*/

typedef Ulong	Packet;		/* single unit of number */

#define TOTNUMBITS	64	/* need 64 bits of integral value */

#define BYTE_ORDER	LITTLE_ENDIAN

#define	FLOATING_EXPRS		/* enable floating expressions */
