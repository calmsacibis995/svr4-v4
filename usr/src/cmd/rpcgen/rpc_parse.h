/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rpcgen:rpc_parse.h	1.2.3.2"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

/*
 * rpc_parse.h, Definitions for the RPCL parser 
 */

enum defkind {
	DEF_CONST,
	DEF_STRUCT,
	DEF_UNION,
	DEF_ENUM,
	DEF_TYPEDEF,
	DEF_PROGRAM
};
typedef enum defkind defkind;

typedef char *const_def;

enum relation {
	REL_VECTOR,	/* fixed length array */
	REL_ARRAY,	/* variable length array */
	REL_POINTER,	/* pointer */
	REL_ALIAS,	/* simple */
};
typedef enum relation relation;

struct typedef_def {
	char *old_prefix;
	char *old_type;
	relation rel;
	char *array_max;
};
typedef struct typedef_def typedef_def;

struct enumval_list {
	char *name;
	char *assignment;
	struct enumval_list *next;
};
typedef struct enumval_list enumval_list;

struct enum_def {
	enumval_list *vals;
};
typedef struct enum_def enum_def;

struct declaration {
	char *prefix;
	char *type;
	char *name;
	relation rel;
	char *array_max;
};
typedef struct declaration declaration;

struct decl_list {
	declaration decl;
	struct decl_list *next;
};
typedef struct decl_list decl_list;

struct struct_def {
	decl_list *decls;
};
typedef struct struct_def struct_def;

struct case_list {
	char *case_name;
	declaration case_decl;
	struct case_list *next;
};
typedef struct case_list case_list;

struct union_def {
	declaration enum_decl;
	case_list *cases;
	declaration *default_decl;
};
typedef struct union_def union_def;

struct proc_list {
	char *proc_name;
	char *proc_num;
	char *arg_type;
	char *arg_prefix;
	char *res_type;
	char *res_prefix;
	struct proc_list *next;
};
typedef struct proc_list proc_list;

struct version_list {
	char *vers_name;
	char *vers_num;
	proc_list *procs;
	struct version_list *next;
};
typedef struct version_list version_list;

struct program_def {
	char *prog_num;
	version_list *versions;
};
typedef struct program_def program_def;

struct definition {
	char *def_name;
	defkind def_kind;
	union {
		const_def co;
		struct_def st;
		union_def un;
		enum_def en;
		typedef_def ty;
		program_def pr;
	} def;
};
typedef struct definition definition;

definition *get_definition();
