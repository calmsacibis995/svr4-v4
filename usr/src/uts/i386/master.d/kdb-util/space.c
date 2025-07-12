/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)master:kdb-util/space.c	1.2.2.1"

#include "sys/kdebugger.h"
#include "sys/conf.h"
#include "config.h"

/*
 * To install additional debugger interfaces, add them to this table.
 * You must then also provide a stub for your init routine in case your
 * module is de-configured.
 */
extern void db_init();
void (*kdb_inittbl[])() = {
	db_init,
	(void (*)())0
};


#ifdef AT386
int asyputchar(), asygetchar();
static struct conssw asysw = {
	asyputchar,	0,	asygetchar
};
#endif

struct conssw *dbg_io[] = {
	&conssw,
#ifdef AT386
	&asysw,
#endif
};

struct conssw **cdbg_io = dbg_io;
int ndbg_io = sizeof(dbg_io) / sizeof(struct conssw *);

/*
 * As a security feature, the kdb_security flag (set by the KDBSECURITY
 * tuneable) is provided.  If it is non-zero, the debugger should ignore
 * attempts to enter from a console key sequence.
 */
int kdb_security = KDBSECURITY;
