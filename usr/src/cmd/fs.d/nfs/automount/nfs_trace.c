/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)nfs.cmds:nfs/automount/nfs_trace.c	1.2.2.1"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
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
 *     Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 *
 *  (c) 1986,1987,1988.1989  Sun Microsystems, Inc
 *  (c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *            All rights reserved.
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <rpc/types.h>
#include "nfs_prot.h"

p_void()
{
}

struct statent {
	nfsstat val;
	char *str;
} stattab[] = {
#define doit(XXX)	{ XXX, "XXX" },
	doit(NFS_OK)
	doit(NFSERR_PERM)
	doit(NFSERR_NOENT)
	doit(NFSERR_IO)
	doit(NFSERR_NXIO)
	doit(NFSERR_ACCES)
	doit(NFSERR_EXIST)
	doit(NFSERR_NODEV)
	doit(NFSERR_NOTDIR)
	doit(NFSERR_ISDIR)
	doit(NFSERR_FBIG)
	doit(NFSERR_NOSPC)
	doit(NFSERR_ROFS)
	doit(NFSERR_NAMETOOLONG)
	doit(NFSERR_NOTEMPTY)
	doit(NFSERR_DQUOT)
	doit(NFSERR_STALE)
	doit(NFSERR_WFLUSH)
#undef doit
};
#define STATTABSIZE (sizeof(stattab)/sizeof(struct statent))


p_nfsstat(status)
	nfsstat *status;
{
	int i;

	for (i = 0; i < STATTABSIZE; i++) {
		if (stattab[i].val == *status) {
			(void) fprintf(stderr, "stat=%s", stattab[i].str);
			return;
		}
	}
	(void) fprintf(stderr, "stat=%d", *status);
}

p_fhandle(fh)
	nfs_fh *fh;
{
	register int *ip = (int *)fh;

	(void) fprintf(stderr, "fh=[%d, %d, %d]", ip[0], ip[1], ip[2]);
}

p_diropargs(d)
	diropargs *d;
{
	p_fhandle(&d->dir);
	(void) fprintf(stderr, ", name=%s", d->name);
}

p_nfstime(t)
	nfstime *t;
{
	char *s;
	char *ctime();
	
	s = ctime((long *)&t->seconds);
	s[strlen(s) - 1] = 0;
	(void) fprintf(stderr, "%s", s);
}

p_fattr(f)
	fattr *f;
{
	(void) fprintf(stderr, "type=%u, mode=%o, nlink=%u, uid=%u, gid=%u,\
size=%u, blocksize=%u, rdev=%u, blocks=%u, fsid=%u, fileid=%u",
		f->type, f->mode, f->nlink, f->uid, f->gid, f->size, 
		f->blocksize, f->rdev, f->blocks, f->fsid, f->fileid);
	(void) fprintf(stderr, ", atime=");
	p_nfstime(&f->atime);
	(void) fprintf(stderr, ", mtime=");
	p_nfstime(&f->mtime);
	(void) fprintf(stderr, ", ctime=");
	p_nfstime(&f->ctime);
}

p_sattr(s)
	sattr *s;
{
	(void) fprintf(stderr, "mode=%o, uid=%u, gid=%u, size=%u", 
		s->mode, s->uid, s->gid, s->size);
	(void) fprintf(stderr, ", atime=");
	p_nfstime(&s->atime);
	(void) fprintf(stderr, ", mtime=");
	p_nfstime(&s->mtime);
}

p_diropres(d)
	diropres *d;
{
	p_nfsstat(&d->status);
	if (d->status == NFS_OK) {
		(void) fprintf(stderr, ", ");
		p_fhandle(&d->diropres_u.diropres.file);
		(void) fprintf(stderr, ", ");
		p_fattr(&d->diropres_u.diropres.attributes);
	}
}

p_sattrargs(sa)
	sattrargs *sa;
{
	p_fhandle(&sa->file);
	(void) fprintf(stderr, ", ");
	p_sattr(&sa->attributes);
}

p_attrstat(as)
	attrstat *as;
{
	p_nfsstat(&as->status);
	if (as->status == NFS_OK) {
		(void) fprintf(stderr, ", ");
		p_fattr(&as->attrstat_u.attributes);
	}
}

p_readlinkres(r)
	readlinkres *r;
{
	p_nfsstat(&r->status);
	if (r->status == NFS_OK) {
		(void) fprintf(stderr, ", data=%s", r->readlinkres_u.data);
	}
}

p_readargs(r)
	readargs *r;
{
	p_fhandle(&r->file);
	(void) fprintf(stderr, ", offset=%u, count=%u", r->offset, r->count);
}

p_readres(r)
	readres *r;
{
	p_nfsstat(&r->status);
	if (r->status == NFS_OK) {
		(void) fprintf(stderr, ", ");
		p_fattr(&r->readres_u.reply.attributes);
		(void) fprintf(stderr, ", len=%u, data=(data)",
			r->readres_u.reply.data.data_len);
	}
}

p_writeargs(w)
	writeargs *w;
{
	p_fhandle(&w->file);
	(void) fprintf(stderr, ", offset=%u, len=%u, data=(data)", 
		w->offset, w->data.data_len);
}

p_createargs(c)
	createargs *c;
{
	p_diropargs(&c->where);
	(void) fprintf(stderr, ", ");
	p_sattr(&c->attributes);
}

p_renameargs(r)
	renameargs *r;
{
	p_diropargs(&r->from); 
	(void) fprintf(stderr, ", ");
	p_diropargs(&r->to);
}

p_linkargs(args)
	linkargs *args;
{
	p_fhandle(&args->from);
	(void) fprintf(stderr, ", ");
	p_diropargs(&args->to);
}

p_symlinkargs(args)
	symlinkargs *args;
{
	p_diropargs(&args->from);
	(void) fprintf(stderr, ", to=%s, ", args->to);
	p_sattr(&args->attributes);
}

p_statfsres(res)
	statfsres *res;
{
	p_nfsstat(&res->status);
	if (res->status == NFS_OK) {
		(void) fprintf(stderr,
		", tsize=%d, bsize=%d, blocks=%d, bfree=%d, bavail=%d",
			res->statfsres_u.reply.tsize, 
			res->statfsres_u.reply.bsize, 
			res->statfsres_u.reply.blocks,
			res->statfsres_u.reply.bfree, 
			res->statfsres_u.reply.bavail);
	}
}

p_readdirargs(args)
	readdirargs *args;
{
	p_fhandle(&args->dir);
	(void) fprintf(stderr, ", cookie=%d, count=%d",
		*(int *)args->cookie, args->count);
}

p_entryp(p)
	entry *p;
{
	while (p != NULL) {
		(void) fprintf(stderr, "(fileid=%u, name=%s, cookie=%u), ", 
			p->fileid, p->name, *(int*)p->cookie);
		p = p->nextentry;
	}
}

p_readdirres(res)
	readdirres *res;
{
	p_nfsstat(&res->status);
	if (res->status == NFS_OK) {
		p_entryp(res->readdirres_u.reply.entries);
		(void) fprintf(stderr, ", eof=%d", res->readdirres_u.reply.eof);
	}
}
	
struct procinfo {
	char *name;
	int (*pargs)();
	int (*pres)();
} procs[] = {
    { "NULL",		p_void,		p_void		},
    { "GETATTR",	p_fhandle,	p_attrstat	},
    { "SETATTR",	p_sattrargs, 	p_attrstat	},
    { "ROOT",		p_void,		p_void		},
    { "LOOKUP",		p_diropargs,	p_diropres	},
    { "READLINK",	p_fhandle,	p_readlinkres	},
    { "READ",		p_readargs,	p_readres	},
    { "WRITECACHE",	p_void,		p_void		},
    { "WRITE",		p_writeargs,	p_attrstat	},
    { "CREATE",		p_createargs,	p_diropres	},
    { "REMOVE",		p_diropargs,	p_nfsstat	},
    { "RENAME",		p_renameargs,	p_nfsstat	},
    { "LINK",		p_linkargs,	p_nfsstat	},
    { "SYMLINK",	p_symlinkargs,	p_nfsstat	},
    { "MKDIR",		p_createargs,	p_nfsstat	},
    { "RMDIR",		p_diropargs,	p_nfsstat	},
    { "READDIR",	p_readdirargs,	p_readdirres	},
    { "STATFS",		p_fhandle,	p_statfsres	},
};


trace_call(procnum, args)
	int procnum;
	char *args;
{
	(void) fprintf(stderr, "%s call(", procs[procnum].name);
	(*procs[procnum].pargs)(args);
	(void) fprintf(stderr, ")\n");
}

trace_return(procnum, res)
	int procnum;
	char *res;
{
	(void) fprintf(stderr, "%s return(", procs[procnum].name);
	(*procs[procnum].pres)(res);
	(void) fprintf(stderr, ")\n");
}
