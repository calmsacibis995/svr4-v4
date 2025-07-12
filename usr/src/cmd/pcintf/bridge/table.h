/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/table.h	1.1"
#ifdef H_SCCSIDS
#include <sccs.h>
SCCSID(@(#)table.h	4.2	LCC);	/* Modified: 19:13:26 9/8/89 */
#endif 

/* table.h - common info required for character translation tables */
#ifndef H_TABLE
#define H_TABLE

#include <lcs.h>

#ifndef DEFAULT_CHAR
#define DEFAULT_CHAR '*'
#endif

#ifndef NO_EXTERNS
extern short country;
extern char  default_char;

extern char dos_table_name[];
extern char unix_table_name[];

extern tbl_ptr unix_table, dos_table;		/* pointers to the UNIX
						 * and DOS translation
						 * tables
						 */
extern int  cur_table_flag;
#endif /* NO_EXTERNS */

#endif /* H_TABLE */
