/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:pkg_lcs/t_string.c	1.1"
/* SCCSID(@(#)t_string.c	4.2	LCC)	* Modified: 17:05:18 2/23/90 */

/*
 *  lcs_translate_string(out, out_size, in)
 */

#include "lcs.h"
#include "lcs_int.h"


lcs_translate_string(out, out_size, in)
char *out;
int out_size;
char *in;
{
	lcs_char canon[2];
	int r;
	int ib;

	lcs_errno = lcs_input_bytes_processed =
	lcs_output_bytes_processed = lcs_exact_translations =
	lcs_multiple_translations = lcs_best_single_translations =
	lcs_user_default_translations = 0;

	if (out_size <= 0) {
		lcs_errno = LCS_ERR_NOSPACE;
		return -1;
	}
	*out = '\0';
	canon[1] = lcs_ascii('\0');

	while (in[lcs_input_bytes_processed] != '\0') {
		ib = lcs_input_bytes_processed;
		if ((r = lcs_convert_in(canon, 0,
					&in[lcs_input_bytes_processed], 0)) < 0)
			return r;
		if (r) {
			if (lcs_mode & LCS_MODE_UPPERCASE)
				canon[0] = lcs_toupper(canon[0]);
			if (lcs_mode & LCS_MODE_LOWERCASE)
				canon[0] = lcs_tolower(canon[0]);

			if ((r = lcs_convert_out(out, out_size, canon, 0)) < 0)
				return r;
			out += r;
			out_size -= r;
		}
		if (lcs_errno) {
			lcs_input_bytes_processed = ib;
			return -1;
		}
	}
	return 0;
}
