/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cscope:common/compath.c	1.2"
/*
 *	compath(pathname)
 *
 *	This compresses pathnames.  All strings of multiple slashes are
 *	changed to a single slash.  All occurrences of "./" are removed.
 *	Whenever possible, strings of "/.." are removed together with
 *	the directory names that they follow.
 *
 *	WARNING: since pathname is altered by this function, it should
 *		 be located in a temporary buffer. This avoids the problem
 *		 of accidently changing strings obtained from makefiles
 *		 and stored in global structures.
 */

#include <string.h>

#if BSD
#define	strchr	index
#endif
#ifndef NULL
#define	NULL	0
#endif

char *
compath(pathname)			/*FDEF*/
char *pathname;
{
	register char	*nextchar;
	register char	*lastchar;
	char	*sofar;
	char	*pnend;

	int	pnlen;

		/*
		 *	do not change the path if it has no "/"
		 */

	if (strchr(pathname, '/') == NULL)
		return(pathname);

		/*
		 *	find all strings consisting of more than one '/'
		 */

	for (lastchar = pathname + 1; *lastchar != '\0'; lastchar++)
		if ((*lastchar == '/') && (*(lastchar - 1) == '/'))
		{

			/*
			 *	find the character after the last slash
			 */

			nextchar = lastchar;
			while (*++lastchar == '/')
			{
			}

			/*
			 *	eliminate the extra slashes by copying
			 *	everything after the slashes over the slashes
			 */

			sofar = nextchar;
			while ((*nextchar++ = *lastchar++) != '\0')
				;
			lastchar = sofar;
		}

		/*
		 *	find all strings of "./"
		 */

	for (lastchar = pathname + 1; *lastchar != '\0'; lastchar++)
		if ((*lastchar == '/') && (*(lastchar - 1) == '.') &&
		    ((lastchar - 1 == pathname) || (*(lastchar - 2) == '/')))
		{

			/*
			 *	copy everything after the "./" over the "./"
			 */

			nextchar = lastchar - 1;
			sofar = nextchar;
			while ((*nextchar++ = *++lastchar) != '\0')
				;
			lastchar = sofar;
		}

		/*
		 *	find each occurrence of "/.."
		 */

	for (lastchar = pathname + 1; *lastchar != '\0'; lastchar++)
		if ((lastchar != pathname) && (*lastchar == '/') &&
		    (*(lastchar + 1) == '.') && (*(lastchar + 2) == '.') &&
		    ((*(lastchar + 3) == '/') || (*(lastchar + 3) == '\0')))
		{

			/*
			 *	find the directory name preceding the "/.."
			 */

			nextchar = lastchar - 1;
			while ((nextchar != pathname) &&
			    (*(nextchar - 1) != '/'))
				--nextchar;

			/*
			 *	make sure the preceding directory's name
			 *	is not "." or ".."
			 */

			if ((*nextchar == '.') &&
			    (*(nextchar + 1) == '/') ||
			    ((*(nextchar + 1) == '.') && (*(nextchar + 2) == '/')))
				/* EMPTY */;
			else
			{

				/*
				 * 	prepare to eliminate either
				 *	"dir_name/../" or "dir_name/.."
				 */

				if (*(lastchar + 3) == '/')
					lastchar += 4;
				else
					lastchar += 3;

				/*
				 *	copy everything after the "/.." to
				 *	before the preceding directory name
				 */

				sofar = nextchar - 1;
				while ((*nextchar++ = *lastchar++) != '\0');
					
				lastchar = sofar;

				/*
				 *	if the character before what was taken
				 *	out is '/', set up to check if the
				 *	slash is part of "/.."
				 */

				if ((sofar + 1 != pathname) && (*sofar == '/'))
					--lastchar;
			}
		}

	/*
 	 *	if the string is more than a character long and ends
	 *	in '/', eliminate the '/'.
	 */

	pnlen = strlen(pathname);
	pnend = strchr(pathname, '\0') - 1;

	if ((pnlen > 1) && (*pnend == '/'))
	{
		*pnend-- = '\0';
		pnlen--;
	}

	/*
	 *	if the string has more than two characters and ends in
	 *	"/.", remove the "/.".
	 */

	if ((pnlen > 2) && (*(pnend - 1) == '/') && (*pnend == '.'))
		*--pnend = '\0';

	/*
	 *	if all characters were deleted, return ".";
	 *	otherwise return pathname
	 */

	if (*pathname == '\0')
		(void) strcpy(pathname, ".");

	return(pathname);
}
