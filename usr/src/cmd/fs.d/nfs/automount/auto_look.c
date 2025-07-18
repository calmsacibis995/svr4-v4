/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)nfs.cmds:nfs/automount/auto_look.c	1.6.2.1"

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
#include <ctype.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/param.h>
#include <pwd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>
#include <rpc/xdr.h>
#include <rpc/clnt.h>

#ifdef YP
#include <rpcsvc/ypclnt.h>
#endif YP

#include "nfs_prot.h"
#define NFSCLIENT
typedef nfs_fh fhandle_t;
#include <rpcsvc/mount.h>
#include <nfs/mount.h>
#include "automount.h"

#define	MAXHOSTS 20
nfsstat do_mount();
void diag();
void getword();
void unquote();
void macro_expand();
extern int trace;
char *malloc();


nfsstat
lookup(dir, name, vpp, cred)
	struct autodir *dir;
	char *name;
	struct avnode **vpp;
	struct authunix_parms *cred;
{
	struct mapent *me;
	struct mapfs *mfs;
	struct link *link;
	struct filsys *fs;
	char *linkpath;
	nfsstat status;

	if (name[0] == '.' &&
	    (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
		*vpp = &dir->dir_vnode;
		return (NFS_OK);
	}


	if ((link = findlink(dir, name)) && link->link_fs) {
		link->link_death = time_now + max_link_time;
		link->link_fs->fs_rootfs->fs_death = time_now + max_link_time;
		*vpp =  &link->link_vnode;
		 return (NFS_OK);
	}

	me = getmapent(dir->dir_map, dir->dir_opts, name, cred);
	if (me == NULL) {
		if (*name == '=' && cred->aup_uid == 0) {
			if (isdigit(*(name+1))) {
				trace = atoi(name+1);
				(void) fprintf(stderr, "automount trace = %d\n",
					trace);
			} else {
				diag(name+1);
			}
		}
		return (NFSERR_NOENT);
	}

	if (trace > 1) {
		struct mapent *ms;
	
		fprintf(stderr, "%s/ %s (%s)\n",
			dir->dir_name, name, me->map_root);
		for (ms = me ; ms ; ms = ms->map_next) {
			fprintf(stderr, "   %s \t-%s\t",
				*ms->map_mntpnt ? ms->map_mntpnt : "/",
				ms->map_mntopts);
			for (mfs = ms->map_fs ; mfs ; mfs = mfs->mfs_next)
				fprintf(stderr, "%s:%s%s%s ",
					mfs->mfs_host,
					mfs->mfs_dir,
					*mfs->mfs_subdir ? ":" : "",
					mfs->mfs_subdir);
			fprintf(stderr, "\n");
		}
	}

	fs = NULL;
	status = do_mount(dir, me, &fs, &linkpath);
	if (status != NFS_OK) {
		free_mapent(me);
		return (status);
	}

	link = makelink(dir, name, fs, linkpath);
	free_mapent(me);
	if (link == NULL)
		return (NFSERR_NOSPC);
	*vpp = &link->link_vnode;
	return (NFS_OK);
}

void
diag(s)
	char *s;
{
	register int i;
	struct autodir *dir;
	register struct avnode *avnode;
	register struct link *link;
	register struct filsys *fs, *nextfs;
	extern struct q fh_q_hash[];
	extern int verbose;
	extern dev_t tmpdev;

	switch (*s) {
	case 'v':	/* toggle verbose */
		verbose = !verbose;
		syslog(LOG_ERR, "verbose %s", verbose? "on" : "off");
		break;

	case 'n':	/* print vnodes */
		for (i = 0; i < FH_HASH_SIZE; i++) {
			avnode = HEAD(struct avnode, fh_q_hash[i]);
			for (; avnode; avnode = NEXT(struct avnode, avnode)) {
				if (avnode->vn_type == VN_LINK) {
				link = (struct link *)avnode->vn_data;
				fprintf(stderr, "link: %s/ %s ",
					link->link_dir->dir_name,
					link->link_name);
				if (link->link_path)
					fprintf(stderr, "-> \"%s\" ",
						link->link_path);
				if (link->link_fs)
					fprintf(stderr, "@ %s:%s ",
						link->link_fs->fs_host,
						link->link_fs->fs_dir);
				fprintf(stderr, "\t[%d]\n",
					link->link_death == 0 ? 0 :
					  link->link_death - time_now);
				} else {
				dir = (struct autodir *)avnode->vn_data;
				fprintf(stderr, "dir : %s %s -%s\n",
					dir->dir_name, dir->dir_map,
					dir->dir_opts);
				}
			}
		}

	case 'f' :	/* print fs's */
		for (fs = HEAD(struct filsys, fs_q); fs; fs = nextfs) {
			nextfs = NEXT(struct filsys, fs);
			(void) fprintf(stderr, "%s %s:%s -%s ",
				fs->fs_mntpnt, fs->fs_host, fs->fs_dir,
				fs->fs_opts);
			if (fs->fs_mine) {
				(void) fprintf(stderr, "(%x)%x:%x ",
					tmpdev & 0xFFFF,
					fs->fs_mntpntdev & 0xFFFF,
					fs->fs_mountdev & 0xFFFF);
				(void) fprintf(stderr, "%d\n",
					fs->fs_death > time_now ?
					fs->fs_death - time_now : 0);
			} else
				(void) fprintf(stderr, "\n");
		} /* end for */
		break;
	}
}

struct mapent *
getmapent(mapname, mapopts, key, cred)
	char *mapname, *mapopts, *key;
	struct authunix_parms *cred;
{
	FILE *fp = NULL;
	char *ypline = NULL;
	struct mapent *me = NULL;
	int len;
	char *get_line();
	char word[64], wordq[64];
	char *lp, linebuf[1024], linebufq[1024], *lq = linebufq;
	struct mapent *getmapent_hosts();
	struct mapent *getmapent_passwd();
	struct mapent *do_mapent();


	if (strcmp(mapname, "-hosts") == 0)
		return (getmapent_hosts(mapname, mapopts, key));
	if (strcmp(mapname, "-passwd") == 0)
		return (getmapent_passwd(mapname, mapopts, key, cred));

	if ((fp = fopen(mapname, "r")) != NULL) {
		/*
		 * The map is in a file, and the file exists; scan it.
		 */
		for (;;) {
			lp = get_line(fp, linebuf, sizeof linebuf);
			if (lp == NULL) {
				(void) fclose(fp);
				return ((struct mapent *)0);
			}

			/* now have complete line */

			lq = linebufq;
			unquote(lp, lq);
			getword(word, wordq, &lp, &lq, ' ');
			if (strcmp(word, key) == 0)
				break;
			if (word[0] == '*' && word[1] == '\0')
				break;
			if (word[0] == '+') {
				me = getmapent(word+1, mapopts, key, cred);
				if (me != NULL) {
					(void) fclose(fp);
					return (me);
				}
			}
		}
		(void) fclose(fp);
	} else {
		/*
		 * The map is a YP map, or is claimed to be a file but
		 * the file does not exist; just lookup the YP entry.
		 */
#ifndef YP
			return ((struct mapent *)NULL);
#else
		if (try_yp(mapname, key, &ypline, &len))
			return ((struct mapent *)NULL);

		/* trim the YP entry - ignore # and beyond */
		if (lp = strchr(ypline, '#'))
			*lp = '\0';
		len = strlen(ypline);
		if (len <= 0)
			goto done;
		/* trim trailing white space */
		lp = &ypline[len - 1];
		while (lp > ypline && isspace(*lp))
			*lp-- = '\0';
		if (lp == ypline)
			goto done;
		(void) strcpy(linebuf, ypline);
		lp = linebuf;
		lq = linebufq;
		unquote(lp, lq);
#endif YP
	}

	/* now have correct line */

	me = do_mapent(lp, lq, mapname, mapopts, key);

done:
	if (ypline)
		free((char *)ypline);
	return (me);
}

struct mapent *
do_mapent(lp, lq, mapname, mapopts, key)
	char *lp, *lq, *mapname, *mapopts, *key;
{
	char w[1024], wq[1024];
	struct mapent *me, *mp, *ms;
	int err, implied;

	macro_expand(key, lp, lq);
	if (trace > 1)
		(void) fprintf(stderr, "\"%s %s\"\n", key, lp);

	getword(w, wq, &lp, &lq, ' ');
	implied = *w != '/';

	ms = NULL;
	while (*w == '/' || implied) {
		mp = me;
		me = (struct mapent *)malloc(sizeof *me);
		if (me == NULL)
			goto alloc_failed;
		memset((char *) me, 0, sizeof *me);
		if (ms == NULL)
			ms = me;
		else
			mp->map_next = me;
		
		if (strcmp(w, "/") == 0 || implied)
			me->map_mntpnt = strdup("");
		else
			me->map_mntpnt = strdup(w);
		if (me->map_mntpnt == NULL)
			goto alloc_failed;

		if (implied)
			implied = 0;
		else
			getword(w, wq, &lp, &lq, ' ');

		if (w[0] == '-') {	/* mount options */
			me->map_mntopts = strdup(w+1);
			getword(w, wq, &lp, &lq, ' ');
		} else
			me->map_mntopts = strdup(mapopts);
		if (me->map_mntopts == NULL)
			goto alloc_failed;
		if (w[0] == '\0') {
			syslog(LOG_ERR, "map %s, key %s: bad", mapname, key);
			goto bad_entry;
		}
		err = mfs_get(mapname, me, w, wq, &lp, &lq);
		if (err < 0)
			goto alloc_failed;
		if (err > 0)
			goto bad_entry;
		me->map_next = NULL;
	}

	if (*key == '/') {
		*w = '\0';	/* a hack for direct maps */
	} else {
		(void) strcpy(w, "/");
		(void) strcat(w, key);
	}
	ms->map_root = strdup(w);
	if (ms->map_root == NULL)
		goto alloc_failed;

	return (ms);

alloc_failed:
	syslog(LOG_ERR, "Memory allocation failed: %m");
bad_entry:
	free_mapent(ms);
	return ((struct mapent *) NULL);
}

#ifdef YP

try_yp(map, key, ypline, yplen)
	char *map, *key;
	char **ypline;
	int *yplen;
{
	int reason;

	reason = yp_match(mydomain, map, key, strlen(key), ypline, yplen);
	if (reason) {
		if (reason == YPERR_KEY) {
			/*
			 * Try the default entry "*"
			 */
			if (yp_match(mydomain, map, "*", 1, ypline, yplen))
				return (1);
		} else {
			syslog(LOG_ERR, "%s: %s", map, yperr_string(reason));
			return (1);
		}
	}
	return (0);
}
#endif YP

char *
get_line(fp, line, linesz)
	FILE *fp;
	char *line;
	int linesz;
{
	register char *p;
	register int len;

	p = line;

	for (;;) {
		if (fgets(p, linesz - (p-line), fp) == NULL)
			return (NULL);

		/* ignore # and beyond */
		if (p = strchr(line, '#'))
			*p = '\0';
		len = strlen(line);
		if (len <= 0) {
			p = line;
			continue;
		}
		/* trim trailing white space */
		p = &line[len - 1];
		while (p > line && isspace(*p))
			*p-- = '\0';
		if (p == line)
			continue;
		/* if continued, get next line */
		if (*p == '\\')
			continue;
		return (line);
	}
}

mfs_get(mapname, me, w, wq, lp, lq)
	struct mapent *me;
	char *mapname, *w, *wq, **lp, **lq;
{
	struct mapfs *mfs, **mfsp;
	char *wlp, *wlq;
	char *hl, hostlist[1024], *hlq, hostlistq[1024];
	char hostname[MAXHOSTNAMELEN+1];
	char dirname[MAXPATHLEN+1], subdir[MAXPATHLEN+1];
	char qbuff[MAXPATHLEN+1];

	mfsp = &me->map_fs;
	*mfsp = NULL;

	while (*w && *w != '/') {
		wlp = w ; wlq = wq;
		getword(hostlist, hostlistq, &wlp, &wlq, ':');
		if (!*hostlist)
			goto bad_entry;
		getword(dirname, qbuff, &wlp, &wlq, ':');
		if (*dirname != '/')
			goto bad_entry;
		*subdir = '/'; *qbuff = ' ';
		getword(subdir+1, qbuff+1, &wlp, &wlq, ':');

		hl = hostlist ; hlq = hostlistq;
		for (;;) {
			getword(hostname, qbuff, &hl, &hlq, ',');
			if (!*hostname)
				break;
			mfs = (struct mapfs *)malloc(sizeof *mfs);
			if (mfs == NULL)
				return (-1);
			memset(mfs, 0, sizeof *mfs);
			*mfsp = mfs;
			mfsp = &mfs->mfs_next;
	
			mfs->mfs_host = strdup(hostname);
			if (mfs->mfs_host == NULL)
				return (-1);
			mfs->mfs_dir = strdup(dirname);
			if (mfs->mfs_dir == NULL)
				return (-1);
			mfs->mfs_subdir = strdup( *(subdir+1) ? subdir : "");
			if (mfs->mfs_subdir == NULL)
				return (-1);
		}
		getword(w, wq, lp, lq, ' ');
	}
	return (0);

bad_entry:
	syslog(LOG_ERR, "bad entry in map %s \"%s\"", mapname, w);
	return (1);
}

free_mapent(me)
	struct mapent *me;
{
	struct mapfs *mfs;
	struct mapent *m;

	while (me) {
		while (me->map_fs) {
			mfs = me->map_fs;
			if (mfs->mfs_host)
				free(mfs->mfs_host);
			if (mfs->mfs_dir)
				free(mfs->mfs_dir);
			if (mfs->mfs_subdir)
				free(mfs->mfs_subdir);
			me->map_fs = mfs->mfs_next;
			free((char *)mfs);
		}
		if (me->map_root)
			free(me->map_root);
		if (me->map_mntpnt)
			free(me->map_mntpnt);
		if (me->map_mntopts)
			free(me->map_mntopts);
		m = me->map_next;
		free((char *)me);	/* from all this misery */
		me = m;
	}
}

/*
 * Gets the next token from the string "p" and copies
 * it into "w".  Both "wq" and "w" are quote vectors
 * for "w" and "p".  Delim is the character to be used
 * as a delimiter for the scan.  A space means "whitespace".
 */
void
getword(w, wq, p, pq, delim)
	char *w, *wq, **p, **pq, delim;
{
	while ((delim == ' ' ? isspace(**p) : **p == delim) && **pq == ' ')
		(*p)++, (*pq)++;

	while (**p &&
	     !((delim == ' ' ? isspace(**p) : **p == delim) && **pq == ' ')) {
		*w++  = *(*p)++;
		*wq++ = *(*pq)++;
	}
	*w  = '\0';
	*wq = '\0';
}

/*
 * Performs text expansions in the string "pline".
 * "plineq" is the quote vector for "pline".
 * An identifier prefixed by "$" is replaced by the
 * corresponding environment variable string.  A "&"
 * is replaced by the key string for the map entry.
 */
void
macro_expand(key, pline, plineq)
	char *key, *pline, *plineq;
{
	register char *p,  *q;
	register char *bp, *bq;
	register char *s;
	char buffp[1024], buffq[1024];
	char envbuf[64], *pe;
	int expand = 0;
	char *getenv();

	p = pline ; q = plineq;
	bp = buffp ; bq = buffq;
	while (*p) {
		if (*p == '&' && *q == ' ') {	/* insert key */
			for (s = key ; *s ; s++) {
				*bp++ = *s;
				*bq++ = ' ';
			}
			expand++;
			p++; q++;
			continue;
		}

		if (*p == '$' && *q == ' ') {	/* insert env var */
			p++; q++;
			pe = envbuf;
			if (*p == '{') {
				p++ ; q++;
				while (*p && *p != '}') {
					*pe++ = *p++;
					q++;
				}
				if (*p) {
					p++ ; q++;
				}
			} else {
				while (*p && isalnum(*p)) {
					*pe++ = *p++;
					q++;
				}
			}
			*pe = '\0';
			s = getenv(envbuf);
			if (s) {
				while (*s) {
					*bp++ = *s++;
					*bq++ = ' ';
				}
				expand++;
			}
			continue;
		}
		*bp++ = *p++;
		*bq++ = *q++;

	}
	if (!expand)
		return;
	*bp = '\0';
	*bq = '\0';
	(void) strcpy(pline , buffp);
	(void) strcpy(plineq, buffq);
}

/* Removes quotes from the string "str" and returns
 * the quoting information in "qbuf". e.g.
 * original str: 'the "quick brown" f\ox'
 * unquoted str: 'the quick brown fox'
 *         qbuf: '    ^^^^^^^^^^^  ^ '
 */
void
unquote(str, qbuf)
	char *str, *qbuf;
{
	register int escaped, inquote, quoted;
	register char *ip, *bp, *qp;
	char buf[2048];

	escaped = inquote = quoted = 0;

	for (ip = str, bp = buf, qp = qbuf ; *ip ; ip++) {
		if (!escaped) {
			if (*ip == '\\') {
				escaped = 1;
				quoted++;
				continue;
			} else
			if (*ip == '"') {
				inquote = !inquote;
				quoted++;
				continue;
			}
		}

		*bp++ = *ip;
		*qp++ = (inquote || escaped) ? '^' : ' ';
		escaped = 0;
	}
	*bp = '\0';
	*qp = '\0';
	if (quoted)
		(void) strcpy(str, buf);
}

struct mapent *
getmapent_passwd(mapname, mapopts, login, cred)
	char *mapname, *mapopts, *login;
	struct authunix_parms *cred;
{
	struct mapent *me;
	struct mapfs *mfs;
	struct passwd *pw;
	char buf[64];
	char *p;
	int c;

	if (login[0] == '~' && login[1] == 0) {
		pw = getpwuid(cred->aup_uid);
		if (pw)
			login = pw->pw_name;
	}
	else
		pw = getpwnam(login);
	if (pw == NULL)
		return ((struct mapent *) NULL);
	for (c = 0, p = pw->pw_dir ; *p ; p++)
		if (*p == '/')
			c++;
	if (c != 3)     /* expect "/dir/host/user" */
		return ((struct mapent *) NULL);

	me = (struct mapent *)malloc(sizeof *me);
	if (me == NULL)
		goto alloc_failed;
	memset((char *) me, 0, sizeof *me);
	me->map_mntopts = strdup(mapopts);
	if (me->map_mntopts == NULL)
		goto alloc_failed;
	mfs = (struct mapfs *)malloc(sizeof *mfs);
	if (mfs == NULL)
		goto alloc_failed;
	memset((char *) mfs, 0, sizeof *mfs);
	me->map_fs = mfs;
	(void) strcpy(buf, "/");
	(void) strcat(buf, login);
	mfs->mfs_subdir = strdup(buf);
	p = strrchr(pw->pw_dir, '/');
	*p = '\0';
	p = strrchr(pw->pw_dir, '/');
	mfs->mfs_host = strdup(p+1);
	if (mfs->mfs_host == NULL)
		goto alloc_failed;
	me->map_root = strdup(p);
	if (me->map_root == NULL)
		goto alloc_failed;
	me->map_mntpnt = strdup("");
	if (me->map_mntpnt == NULL)
		goto alloc_failed;
	mfs->mfs_dir = strdup(pw->pw_dir);
	if (mfs->mfs_dir == NULL)
		goto alloc_failed;
	(void) endpwent();
	return (me);

alloc_failed:
	syslog(LOG_ERR, "Memory allocation failed: %m");
	free_mapent(me);
	return ((struct mapent *) NULL);
}

struct mapent *
getmapent_hosts(mapname, mapopts, host)
	char *mapname, *mapopts, *host;
{
	struct mapent *me, *ms, *mp;
	struct mapfs *mfs;
	struct exports *ex = NULL;
	struct exports *exlist, *texlist, **texp, *exnext;
	struct groups *gr;
	enum clnt_stat clnt_stat, pingmount();
	char name[MAXPATHLEN];
	int elen;

	/* check for special case: host is me */

	if (strcmp(host, self) == 0) {
		ms = (struct mapent *)malloc(sizeof *ms);
		if (ms == NULL)
			goto alloc_failed;
		memset((char *) ms, 0, sizeof *ms);
		ms->map_root = strdup("");
		if (ms->map_root == NULL)
			goto alloc_failed;
		ms->map_mntpnt = strdup("");
		if (ms->map_mntpnt == NULL)
			goto alloc_failed;
		ms->map_mntopts = strdup("");
		if (ms->map_mntopts == NULL)
			goto alloc_failed;
		mfs = (struct mapfs *)malloc(sizeof *mfs);
		if (mfs == NULL)
			goto alloc_failed;
		memset((char *) mfs, 0, sizeof *mfs);
		mfs = (struct mapfs *)malloc(sizeof *mfs);
		if (mfs == NULL)
			goto alloc_failed;
		memset((char *) mfs, 0, sizeof *mfs);
		ms->map_fs = mfs;
		mfs->mfs_host = strdup(self);
		if (mfs->mfs_host == NULL)
			goto alloc_failed;
		mfs->mfs_dir  = strdup("/");
		if (mfs->mfs_dir == NULL)
			goto alloc_failed;
		mfs->mfs_subdir  = strdup("");
		if (mfs->mfs_subdir == NULL)
			goto alloc_failed;
		return (ms);
	}

	if (pingmount(host) != RPC_SUCCESS)
		return ((struct mapent *) NULL);

	/* get export list of host */
        if (clnt_stat = rpc_call(host, MOUNTPROG, MOUNTVERS, MOUNTPROC_EXPORT,
	    xdr_void, 0, xdr_exports, &ex, "datagram_v")) {
	if (trace >1)
		(void) fprintf(stderr,
			"getmapent_hosts: rpc_call %s: error: %s\n",
				host, clnt_sperrno(clnt_stat));
			syslog(LOG_ERR, "%s: exports: %s", host,
		    		clnt_sperrno(clnt_stat));
		return ((struct mapent *) NULL);
	}
	if (ex == NULL) {
		if (trace > 1)
			(void) fprintf(stderr,
				"getmapent_hosts: null export list\n");
		return ((struct mapent *) NULL);
	}

	/* now sort by length of names - to get mount order right */
	exlist = ex;
	texlist = NULL;
	for (ex = exlist; ex; ex = exnext) {
		exnext = ex->ex_next;
		ex->ex_next = 0;
		elen = strlen(ex->ex_name);

#ifdef YP
		/*  check netgroup list  */
		if (ex->ex_groups) {
			for ( gr = ex->ex_groups ; gr ; gr = gr->g_next) {
				if (strcmp(gr->g_name, self) == 0 ||
					in_netgroup(gr->g_name, self, mydomain))
					1)
					break;
			}
			if (gr == NULL) {
				freeex(ex);
				continue;
			}
		}
#endif

		for (texp = &texlist; *texp; texp = &((*texp)->ex_next))
			if (elen < strlen((*texp)->ex_name))
				break;
		ex->ex_next = *texp;
		*texp = ex;
	}
	exlist = texlist;

	/* Now create a mapent from the export list */
	ms = NULL;
	me = NULL;
	for (ex = exlist; ex; ex = ex->ex_next) {
		mp = me;
		me = (struct mapent *)malloc(sizeof *me);
		if (me == NULL)
			goto alloc_failed;
		memset((char *) me, 0, sizeof *me);

		if (ms == NULL)
			ms = me;
		else
			mp->map_next = me;

		(void) strcpy(name, "/");
		(void) strcat(name, host);
		me->map_root = strdup(name);
		if (me->map_root == NULL)
			goto alloc_failed;
		if (strcmp(ex->ex_name, "/") == 0)
			me->map_mntpnt = strdup("");
		else
			me->map_mntpnt = strdup(ex->ex_name);
		if (me->map_mntpnt == NULL)
			goto alloc_failed;
		me->map_mntopts = strdup(mapopts);
		if (me->map_mntopts == NULL)
			goto alloc_failed;
		mfs = (struct mapfs *)malloc(sizeof *mfs);
		if (mfs == NULL)
			goto alloc_failed;
		memset((char *) mfs, 0, sizeof *mfs);
		me->map_fs = mfs;
		mfs->mfs_host = strdup(host);
		if (mfs->mfs_host == NULL)
			goto alloc_failed;
		mfs->mfs_dir  = strdup(ex->ex_name);
		if (mfs->mfs_dir == NULL)
			goto alloc_failed;
		mfs->mfs_subdir = strdup("");
		if (mfs->mfs_subdir == NULL)
			goto alloc_failed;
	}
	freeex(exlist);
	return (ms);

alloc_failed:
	syslog(LOG_ERR, "Memory allocation failed: %m");
	free_mapent(ms);
	freeex(exlist);
	return ((struct mapent *) NULL);
}

#ifdef YP

#define MAXGRPLIST 512
#define MIN(a,b) ((a) < (b) ? (a) : (b))
/*
 * Check cached netgroup info before calling innetgr().
 * Two lists are maintained here:
 * membership groups and non-membership groups.
 */
int
in_netgroup(group, hostname, domain)
	char *group, *hostname, *domain;
{
	static char   grplist[MAXGRPLIST+1];
	static char nogrplist[MAXGRPLIST+1];
	char key[256];
	char *ypline = NULL;
	int yplen;
	register char *gr, *p;
	static time_t last;
	static int cache_time = 300; /* 5 min */

	if (time_now > last + cache_time) {
		last = time_now;
		grplist[0]   = '\0';
		nogrplist[0] = '\0';
		(void) strcpy(key, hostname);
		(void) strcat(key, ".");
		(void) strcat(key, domain);
		if (yp_match(domain, "netgroup.byhost", key,
		    strlen(key), &ypline, &yplen) == 0) {
			(void) strncpy(grplist, ypline, MIN(yplen, MAXGRPLIST));
			grplist[MIN(yplen, MAXGRPLIST)] = '\0';
			free(ypline);
		}
	}

	for (gr = grplist ; *gr ; gr = p ) {
		for (p = gr ; *p && *p != ',' ; p++)
			;
		if (strncmp(group, gr, p - gr) == 0)
			return (1);
		if (*p == ',')
			p++;
	}
	for (gr = nogrplist ; *gr ; gr = p ) {
		for (p = gr ; *p && *p != ',' ; p++)
			;
		if (strncmp(group, gr, p - gr) == 0)
			return (0);
		if (*p == ',')
			p++;
	}

	if (innetgr(group, hostname, (char *)NULL, domain)) {
		if (strlen(grplist)+1+strlen(group) > MAXGRPLIST)
			return (1);
		if (*grplist)
			(void) strcat(grplist, ",");
		(void) strcat(grplist, group);
		return (1);
	} else {
		if (strlen(nogrplist)+1+strlen(group) > MAXGRPLIST)
			return (1);
		if (*nogrplist)
			(void) strcat(nogrplist, ",");
		(void) strcat(nogrplist, group);
		return (0);
	}
}
#endif YP
