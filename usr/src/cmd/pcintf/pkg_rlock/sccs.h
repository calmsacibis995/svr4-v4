/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:pkg_rlock/sccs.h	1.1.1.2"
/*------------------------------------------------------------------------
 *
 *	sccs.h - sccs definitions 
 *
 *	comments:
 *		one definition, SCCSID(), is for source (.c) files.  the
 *		other, SCCSID_H(), is for header (.h) files.  in either
 *		case, id is a standard, quoted character string (the quotes
 *		are required).  for SCCSID_H(), file is the name of the
 *		header file, with the '.' replaced with '_'.  this is
 *		required in order to allow more than one header file to be
 *		included in a source file.
 *
 *		in addition, the last line is the SCCS id for this header,
 *		and must not be on the start of the line.  this allows the
 *		automatic mechanisms used for collection file version numbers
 *		to work correctly.
 *
 *-------------------------------------------------------------------------
 */

#if !defined(lint)
#	define SCCSID(id)		static char Sccsid[] = id
#else
#	define SCCSID(id)
#endif

#if defined(X_SCCS_H) && !defined(lint)
#	define SCCSID_H(id, file)	static char file[] = id;
#else
#	define SCCSID_H(id, file)
#endif

/* DO NOT MODIFY THIS LINE */	SCCSID_H("@(#)sccs.h	1.4", sccs_h)
