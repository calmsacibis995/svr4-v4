/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/version.h	1.1.1.2"
/* SCCSID(@(#)version.h	4.5	LCC);	/* Modified: 17:57:57 2/25/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/******************************************************************************
*
*  This file is included by all of the main files that make up the unix
*  server portions of PCI.
*
*****************************************************************************/

#define VERS_MAJOR	3
#define VERS_MINOR	1
#define VERS_SUBMINOR	3

struct version { 
	short 	vers_major,
		vers_minor,
		vers_subminor;
	};



extern char server_version[];
extern char *bridge_version;

extern unsigned short bridge_ver_flags;

/* flags for bridge_ver_flags: */
#define	V_FAST_LSEEK	0x0001
#define	V_ERR_FILTER	0x0002

