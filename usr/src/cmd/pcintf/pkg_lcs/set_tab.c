/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:pkg_lcs/set_tab.c	1.1"
/* SCCSID(@(#)set_tab.c	4.2	LCC)	* Modified: 17:02:30 2/23/90 */
/*
 *  lcs_set_tables(out_tbl, in_tbl)
 *
 *  Set translation tables
 */

#define NO_LCS_EXTERNS

#include <fcntl.h>
#include "lcs.h"
#include "lcs_int.h"


lcs_set_tables(out_tbl, in_tbl)
lcs_tbl out_tbl;
lcs_tbl in_tbl;
{
	if (strcmp(out_tbl->th_magic, LCS_MAGIC) ||
	    strcmp(in_tbl->th_magic, LCS_MAGIC)) {
		lcs_errno = LCS_ERR_BADTABLE;
		return -1;
	}
	lcs_output_table = out_tbl;
	lcs_input_table = in_tbl;
	return 0;
}


/*
 *  lcs_set_options(mode, user_char, country)
 *
 *  Set translation options
 */

lcs_set_options(mode, user_char, country)
short mode;
char user_char;
short country;
{
	lcs_mode = mode;
	lcs_user_char = user_char;
	lcs_country = country;
	return 0;
}
