/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cvtomf:storclass.h	1.1"

/*
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/* Enhanced Application Compatibility Support */
#ifndef _STORCLASS_H
#define _STORCLASS_H


/*
 *   STORAGE CLASSES
 */

#define  C_EFCN          -1    /* physical end of function */
#define  C_NULL          0
#define  C_AUTO          1     /* automatic variable */
#define  C_EXT           2     /* external symbol */
#define  C_STAT          3     /* static */
#define  C_REG           4     /* register variable */
#define  C_EXTDEF        5     /* external definition */
#define  C_LABEL         6     /* label */
#define  C_ULABEL        7     /* undefined label */
#define  C_MOS           8     /* member of structure */
#define  C_ARG           9     /* function argument */
#define  C_STRTAG        10    /* structure tag */
#define  C_MOU           11    /* member of union */
#define  C_UNTAG         12    /* union tag */
#define  C_TPDEF         13    /* type definition */
#define C_USTATIC	 14    /* undefined static */
#define  C_ENTAG         15    /* enumeration tag */
#define  C_MOE           16    /* member of enumeration */
#define  C_REGPARM	 17    /* register parameter */
#define  C_FIELD         18    /* bit field */
#define  C_BLOCK         100   /* ".bb" or ".eb" */
#define  C_FCN           101   /* ".bf" or ".ef" */
#define  C_EOS           102   /* end of structure */
#define  C_FILE          103   /* file name */

	/*
	 * The following storage class is a "dummy" used only by STS
	 * for line number entries reformatted as symbol table entries
	 */

#define  C_LINE		 104
#define  C_ALIAS	 105   /* duplicate tag */
#define  C_HIDDEN	 106   /* special storage class for external */
			       /* symbols in dmert public libraries  */
#define  C_SHADOW	 107   /* shadow symbol */

#endif	/* _STORCLASS_H */

/* End Enhanced Application Compatibility Support */
