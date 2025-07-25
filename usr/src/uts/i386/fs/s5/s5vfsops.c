/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-fs:s5/s5vfsops.c	1.3.1.5"

#include "sys/types.h"
#include "sys/buf.h"
#include "sys/cmn_err.h"
#include "sys/conf.h"
#include "sys/debug.h"
#include "sys/errno.h"
#include "sys/time.h"
#include "sys/fbuf.h"
#include "sys/file.h"
#include "sys/kmem.h"
#include "sys/mount.h"
#include "sys/open.h"
#include "sys/param.h"
#include "sys/statvfs.h"
#include "sys/swap.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/uio.h"
#include "sys/var.h"
#include "sys/vfs.h"
#include "sys/vnode.h"

#include "sys/immu.h"
#include "sys/signal.h"
#include "sys/user.h"

#include "sys/proc.h"
#include "sys/disp.h"

#include "vm/seg.h"

#include "sys/fs/s5param.h"
#include "sys/fs/s5macros.h"
#include "sys/fs/s5inode.h"
#include "sys/fs/s5ino.h"
#include "sys/fs/s5filsys.h"

#include "fs/fs_subr.h"

#define	SBSIZE	sizeof(struct filsys)

STATIC int s5vfs_init();

/*
 * UNIX file system VFS operations vector.
 */
STATIC int s5mount(), s5unmount(), s5root(), s5statvfs();
STATIC int s5sync(), s5vget(), s5mountroot();

struct vfsops s5vfsops = {
	s5mount,
	s5unmount,
	s5root,
	s5statvfs,
	s5sync,
	s5vget,
	s5mountroot,
	fs_nosys,	/* swapvp */
	fs_nosys,	/* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};

STATIC int
s5mount(vfsp, mvp, uap, cr)
	register struct vfs *vfsp;
	struct vnode *mvp;
	struct mounta *uap;
	struct cred *cr;
{
	register dev_t dev;
	register struct filsys *fp;
	register struct s5vfs *s5vfsp;
	struct vfs *dvfsp;
	struct inode *rip;
	struct vnode *rvp, *bvp;
	int rdonly = (uap->flags & MS_RDONLY);
	int remount = (uap->flags & MS_REMOUNT);
	enum vtype type;
	int error;
	struct fbuf *fbp;
	struct filsys *oldfp = NULL;
	extern int s5fstype;

	if (!suser(cr))
		return EPERM;
	if (mvp->v_type != VDIR)
		return ENOTDIR;
	if (remount == 0 && (mvp->v_count > 1 || (mvp->v_flag & VROOT)))
		return EBUSY;

	/*
	 * Resolve path name of special file being mounted.
	 */
	if (error = lookupname(uap->spec, UIO_USERSPACE, FOLLOW, NULLVPP, &bvp))
		return error;

	if (bvp->v_type != VBLK) {
		VN_RELE(bvp);
		return ENOTBLK;
	}

	dev = bvp->v_rdev;

	/*
	 * Ensure that this device isn't already mounted, unless this is
	 * a remount request.
	 */
	dvfsp = vfs_devsearch(dev);
	if (remount) {
		VN_RELE(bvp);
		/*
		 * Remount requires that the device already be mounted,
		 * and on the same mount point.
		 */
		if (dvfsp == NULL)
			return EINVAL;
		if (dvfsp != vfsp)
			return EBUSY;
		s5vfsp = S5VFS(vfsp);
		bvp = s5vfsp->vfs_devvp;
	} else {
		/*
		 * Ordinary mount.
		 */
		if (dvfsp != NULL) {
			VN_RELE(bvp);
			return EBUSY;
		}
		/*
		 * Allocate VFS private data.
		 */
		if ((s5vfsp = (struct s5vfs *)
		  kmem_alloc(sizeof(struct s5vfs), KM_SLEEP)) == NULL) {
			VN_RELE(bvp);
			return EBUSY;
		}
		vfsp->vfs_bcount = 0;
		vfsp->vfs_data = (caddr_t) s5vfsp;
		vfsp->vfs_fstype = s5fstype;
		/*
		 * Open the device.
		 */
		if (error = VOP_OPEN(&bvp, rdonly ? FREAD : FREAD|FWRITE, cr))
			goto out;
		vfsp->vfs_dev = dev;
		vfsp->vfs_fsid.val[0] = dev;
		vfsp->vfs_fsid.val[1] = s5fstype;
		s5vfsp->vfs_bufp = geteblk();
		s5vfsp->vfs_devvp = bvp;
	}

	/*
	 * Invalidate all data associated with this device (pages and
	 * buffers).  This is probably overkill.
	 */
	(void) VOP_PUTPAGE(bvp, 0, 0, B_INVAL, cr);
	binval(dev);
	if (remount)
		(void) iflush(vfsp, 1);
	/*
	 * Read the superblock.  We do this in the remount case as well
	 * because it might have changed (if fsck was applied, for example).
	 *
	 * In the remount case we should really loop until the ilock and
	 * flock become free.  For the moment we trust that remount will only
	 * be done on a quiescent file system.
	 */
	fp = getfs(vfsp);
	if (error = fbread(bvp, (off_t) SUPERBOFF, SBSIZE, S_WRITE, &fbp))
		goto closeout;
	if (remount) {
		/*
		 * Save the contents of the in-core superblock so it
		 * can be restored if the mount fails.
		 */
		oldfp = (struct filsys *)kmem_alloc(SBSIZE, KM_SLEEP);
		bcopy((caddr_t)fp, (caddr_t)oldfp, SBSIZE);
	}
	bcopy(fbp->fb_addr, (caddr_t)fp, SBSIZE);

	if (fp->s_magic != FsMAGIC) {
		fbrelsei(fbp, S_OTHER);
		error = EINVAL;
		goto closeout;
	}

	fp->s_ilock = 0;
	fp->s_flock = 0;
	fp->s_ninode = 0;
	fp->s_inode[0] = 0;
	fp->s_fmod = 0;
	fp->s_ronly = (char)rdonly;

	if (rdonly) {
		vfsp->vfs_flag |= VFS_RDONLY;
		fbrelsei(fbp, S_OTHER);
	} else {
		if (fp->s_state + (long)fp->s_time == FsOKAY) {
			fp->s_state = FsACTIVE;
			bcopy((caddr_t)fp, fbp->fb_addr, SBSIZE);
			if (fbwritei(fbp) != 0) {
				error = EROFS;
				goto closeout;
			}
		} else {
			fbrelsei(fbp, S_OTHER);
			error = ENOSPC;
			goto closeout;
		}
	}


	/*
	 * Determine blocksize.
	 */
	vfsp->vfs_bsize = FsBSIZE(fp->s_bshift);
	if (vfsp->vfs_bsize < FsMINBSIZE || vfsp->vfs_bsize > MAXBSIZE) {
		error = EINVAL;
		goto closeout;
	}

	s5vfs_init(s5vfsp, vfsp->vfs_bsize);

	if (remount == 0) {
		if (error = iget(vfsp, S5ROOTINO, &rip))
			goto closeout;
		rvp = ITOV(rip);
		rvp->v_flag |= VROOT;
		s5vfsp->vfs_root = rvp;
		IUNLOCK(rip);
	}

	return 0;

closeout:
	/* 
	 * Clean up on error.
	 */
	if (oldfp) {
		bcopy((caddr_t)oldfp, (caddr_t)fp, SBSIZE);
		kmem_free((caddr_t)oldfp, SBSIZE);
	}
	if (remount)
		return error;
	(void) VOP_CLOSE(bvp, rdonly ? FREAD : FREAD|FWRITE, 1, (off_t) 0, cr);
	brelse(s5vfsp->vfs_bufp);

out:
	ASSERT(error);
	kmem_free((caddr_t) s5vfsp, sizeof(struct s5vfs));
	VN_RELE(bvp);
	return error;
}

STATIC int
s5unmount(vfsp, cr)
	struct vfs *vfsp;
	struct cred *cr;
{
	dev_t dev;
	register struct vnode *bvp, *rvp;
	register struct filsys *fp;
	register struct inode *rip;
	register struct s5vfs *s5vfsp = S5VFS(vfsp);
	struct fbuf *fbp;
	int error;

	ASSERT(vfsp->vfs_op == &s5vfsops);

	if (!suser(cr))
		return EPERM;
	dev = vfsp->vfs_dev;

	if (iflush(vfsp, 0) < 0)
		return EBUSY;

	/*
	 * Mark inode as stale.
	 */
	inull(vfsp);
	/*
	 * Flush root inode to disk.
	 */
	rvp = s5vfsp->vfs_root;
	ASSERT(rvp != NULL);
	rip = VTOI(rvp);
	ILOCK(rip);
	/*
	 * At this point there should be no active files on the
	 * file system, and the super block should not be locked.
	 * Break the connections.
	 */
	bvp = s5vfsp->vfs_devvp;
	fp = getfs(vfsp);
	if (!fp->s_ronly) {
		bflush(dev);
		fp->s_time = hrestime.tv_sec;
		if (vfsp->vfs_flag & VFS_BADBLOCK)
			fp->s_state = FsBAD;
		else
			fp->s_state = FsOKAY - (long)fp->s_time;
		if (error = fbread(bvp, (off_t) SUPERBOFF,
		  SBSIZE, S_WRITE, &fbp)) {
			IUNLOCK(rip);
			return error;
		}
		bcopy((caddr_t)fp, fbp->fb_addr, SBSIZE);
		(void) fbwritei(fbp);
	}
	(void) VOP_PUTPAGE(bvp, 0, 0, B_INVAL, cr);
	if (error = VOP_CLOSE(bvp,
	  (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE,
	  1, (off_t) 0, cr)) {
		IUNLOCK(rip);
		return error;
	}
	VN_RELE(bvp);
	binval(dev);
	brelse(s5vfsp->vfs_bufp);
	iput(rip);
	iunhash(rip);
	kmem_free((caddr_t)s5vfsp, sizeof(struct s5vfs));
	rvp->v_vfsp = 0;
	return 0;
}

STATIC int
s5root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	struct s5vfs *s5vfsp = S5VFS(vfsp);
	struct vnode *vp = s5vfsp->vfs_root;

	VN_HOLD(vp);
	*vpp = vp;
	return 0;
}

STATIC int
s5statvfs(vfsp, sp)
	struct vfs *vfsp;
	register struct statvfs *sp;
{
	register struct filsys *fp;
	register struct s5vfs *s5vfsp = S5VFS(vfsp);
	register i;
	char *cp;

	if ((fp = getfs(vfsp))->s_magic != FsMAGIC)
		return EINVAL;

	bzero((caddr_t)sp, sizeof(*sp));
	sp->f_bsize = sp->f_frsize = vfsp->vfs_bsize;
	sp->f_blocks = fp->s_fsize;
	sp->f_bfree = sp->f_bavail = fp->s_tfree;
	sp->f_files = (fp->s_isize - 2) * s5vfsp->vfs_inopb;
	sp->f_ffree = sp->f_favail = fp->s_tinode;
	sp->f_fsid = vfsp->vfs_dev;
	strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sp->f_namemax = DIRSIZ;
	cp = &sp->f_fstr[0];
	for (i=0; i < sizeof(fp->s_fname) && fp->s_fname[i] != '\0'; i++,cp++)
		*cp = fp->s_fname[i];
	*cp++ = '\0';
	for (i=0; i < sizeof(fp->s_fpack) && fp->s_fpack[i] != '\0'; i++,cp++)
		*cp = fp->s_fpack[i];
	*cp = '\0';

	return 0;
}

STATIC void s5update(), s5flushsb();
STATIC int s5updlock, s5updwant;

/* ARGSUSED */
STATIC int
s5sync(vfsp, flag, cr)
	struct vfs *vfsp;
	short flag;
	struct cred *cr;
{
	while (s5updlock) {
		s5updwant = 1;
		sleep((caddr_t)&s5updlock, PINOD);
	}
	s5updlock = 1;
	if (flag & SYNC_ATTR)
		s5flushi(SYNC_ATTR);
	else
		s5update();
	s5updlock = 0;
	if (s5updwant) {
		s5updwant = 0;
		wakeprocs((caddr_t)&s5updlock, PRMPT);
	}
	return 0;
}

/*
 * s5update() goes through the disk queues to initiate sandbagged I/O,
 * through the inodes to write modified nodes, and through the VFS list
 * table to initiate modified superblocks.
 */
STATIC void
s5update()
{
	register struct vfs *vfsp;
	extern struct vfsops s5vfsops;

	for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next)
		if (vfsp->vfs_op == &s5vfsops)
			s5flushsb(vfsp);
	s5flushi(0);
	bflush(NODEV);	/* XXX */
}

int
s5flushi(flag)
	short flag;
{
	register struct vnode *vp;
	register struct inode *ip, *iend;
	register int cheap = flag & SYNC_ATTR;

	for (ip = &inode[0], iend = &inode[ninode]; ip < iend; ip++) {
		vp = ITOV(ip);
		if ((ip->i_flag & (ILOCKED|IRWLOCKED)) == 0
		  && vp->v_count != 0
		  && vp->v_vfsp != 0
		  && (vp->v_pages != NULL
		    || ip->i_flag & (IACC|IUPD|ICHG|IMOD))) {
			ILOCK(ip);
			VN_HOLD(vp);
			/*
			 * If this is an inode sync for file system hardening
			 * or this is a full sync but file is a swap file,
			 * don't sync pages but make sure the inode is up
			 * to date. In other cases, push everything out.
			 */
			if (cheap || IS_SWAPVP(vp))
				iupdat(ip);
			else
				(void) syncip(ip, B_ASYNC);
			iput(ip);
		}
		PREEMPT();
	}

	return 0;
}

STATIC void
s5flushsb(vfsp)
	register struct vfs *vfsp;
{
	register struct filsys *fp;
	register struct vnode *vp;
	struct fbuf *fbp;

	fp = getfs(vfsp);
	if (fp->s_fmod == 0 || fp->s_ilock != 0 || fp->s_flock != 0
	  || fp->s_ronly != 0)
		return;
	fp->s_fmod = 0;
	fp->s_time = hrestime.tv_sec;
	vp = S5VFS(vfsp)->vfs_devvp;
	VN_HOLD(vp);
	if (fbread(vp, (off_t) SUPERBOFF, SBSIZE, S_WRITE, &fbp) == 0) {
		bcopy((caddr_t)fp, fbp->fb_addr, SBSIZE);
		(void) fbwritei(fbp);
	}
	VN_RELE(vp);
}

STATIC int
s5vget(vfsp, vpp, fidp)
	struct vfs *vfsp;
	struct vnode **vpp;
	struct fid *fidp;
{
	register struct ufid *ufid;
	struct inode *ip;

	ufid = (struct ufid *) fidp;
	if (iget(vfsp, ufid->ufid_ino, &ip)) {
		*vpp = NULL;
		return 0;
	}
	if (ip->i_gen != ufid->ufid_gen) {
		iput(ip);
		*vpp = NULL;
		return 0;
	}
	IUNLOCK(ip);
	*vpp = ITOV(ip);
	return 0;
}

/*
 * Mount root file system.
 * "why" is ROOT_INIT on initial call, ROOT_REMOUNT if called to
 * remount the root file system, and ROOT_UNMOUNT if called to
 * unmount the root (e.g., as part of a system shutdown).
 */
/* ARGSUSED */
STATIC int
s5mountroot(vfsp, why)
	struct vfs *vfsp;
	enum whymountroot why;
{
	register struct filsys *fp;
	struct s5vfs *s5vfsp;
	struct inode *mip;
	extern int s5fstype;
	struct vnode *vp;
	struct fbuf *fbp;
	int error;

	switch (why) {
	case ROOT_INIT:
		/*
		 * Initialize the root device.
		 */
		vp = makespecvp(rootdev, VBLK);
		if (error = VOP_OPEN(&vp, FREAD|FWRITE, u.u_cred)) {
			VN_RELE(vp);
			return error;
		}
		s5vfsp =
		  (struct s5vfs *) kmem_alloc(sizeof(struct s5vfs), KM_SLEEP);
		if (s5vfsp == NULL)
			cmn_err(CE_PANIC,
			  "s5mountroot: no memory for VFS private data");
		s5vfsp->vfs_devvp = vp;
		s5vfsp->vfs_bufp = geteblk();
		vfsp->vfs_data = (caddr_t) s5vfsp;
		fp = getfs(vfsp);
		break;

	case ROOT_REMOUNT:
		s5vfsp = S5VFS(vfsp);
		(void) iflush(vfsp, 1);
		(void) VOP_PUTPAGE(s5vfsp->vfs_devvp,
		  0, 0, B_INVAL, (struct cred *) NULL);
		binval(vfsp->vfs_dev);
		fp = getfs(vfsp);
		if (fp->s_state == FsACTIVE)
			return EINVAL;
		break;

	case ROOT_UNMOUNT:
		s5update();
		fp = getfs(vfsp);
		if (fp->s_state == FsACTIVE) {
			fp->s_time = hrestime.tv_sec;
			if (vfsp->vfs_flag & VFS_BADBLOCK)
				fp->s_state = FsBAD;
			else
				fp->s_state = FsOKAY - (long)fp->s_time;
			vp = S5VFS(vfsp)->vfs_devvp;
			if (fbread(vp, (off_t)SUPERBOFF, SBSIZE,
			  S_WRITE, &fbp) == 0) {
				bcopy((caddr_t)fp, fbp->fb_addr, SBSIZE);
				(void) fbwritei(fbp);
			}
			(void) VOP_CLOSE(vp, FREAD|FWRITE, 1,
			  (off_t)0, u.u_cred);
			VN_RELE(vp);
		}
		bdwait();
		return 0;

	default:
		return EINVAL;
	}
	
	vfsp->vfs_dev = rootdev;
	vfsp->vfs_fstype = s5fstype;
	vfsp->vfs_fsid.val[0] = rootdev;
	vfsp->vfs_fsid.val[1] = s5fstype;

	vp = s5vfsp->vfs_devvp;

	if (error = fbread(vp, (off_t)SUPERBOFF, SBSIZE, S_WRITE, &fbp)) {
		VN_RELE(vp);
		return error;
	}
	bcopy(fbp->fb_addr, (caddr_t)fp, SBSIZE);

	fp->s_fmod = 0;
	fp->s_ilock = 0;
	fp->s_flock = 0;
	fp->s_ninode = 0;
	fp->s_inode[0] = 0;
	fp->s_ronly = 0;
	if (fp->s_magic != FsMAGIC) {
		fbrelsei(fbp, S_OTHER);
		VN_RELE(vp);
		return EINVAL;
	}

	/*
	 * Determine blocksize.
	 */
	vfsp->vfs_bsize = FsBSIZE(fp->s_bshift);
	if (vfsp->vfs_bsize < FsMINBSIZE || vfsp->vfs_bsize > MAXBSIZE) {
		fbrelsei(fbp, S_OTHER);
		VN_RELE(vp);
		return EINVAL;
	}

	if ((fp->s_state + (long)fp->s_time) == FsOKAY)
		fp->s_state = FsACTIVE;
	else
		fp->s_state = FsBAD;

	bcopy((caddr_t)fp, fbp->fb_addr, SBSIZE);
	if (error = fbwritei(fbp)) {
		fp->s_state = FsBAD;
		error = 0;
	}
	s5vfs_init(s5vfsp, vfsp->vfs_bsize);

	if (why == ROOT_INIT) {
		clkset(fp->s_time);
		/* 
		 * Initialize root inode.
		 */
		if ((error = iget(vfsp, S5ROOTINO, &mip)) == 0) {
			ITOV(mip)->v_flag |= VROOT;
			IUNLOCK(mip);
			s5vfsp->vfs_root = ITOV(mip);
			if ((error = vfs_lock(vfsp)) == 0) {
				vfs_add(NULLVP, vfsp, 0);
				vfs_unlock(vfsp);
			}
		}
		if (error) {
			(void) VOP_CLOSE(vp, FREAD|FWRITE, 1,
			  (off_t)0, u.u_cred);
			VN_RELE(vp);
		}
	}
	return error;
}

STATIC int
s5vfs_init(s5vfsp, bsize)
	struct s5vfs *s5vfsp;
	int bsize;
{
	int i;

	for (i = bsize, s5vfsp->vfs_bshift = 0; i > 1; i >>= 1)
		s5vfsp->vfs_bshift++;
	s5vfsp->vfs_nindir = bsize / sizeof(daddr_t);
	s5vfsp->vfs_inopb = bsize / sizeof(struct dinode);
	s5vfsp->vfs_bmask = bsize - 1;
	s5vfsp->vfs_nmask = s5vfsp->vfs_nindir - 1;
	for (i = bsize/512, s5vfsp->vfs_ltop = 0; i > 1; i >>= 1)
		s5vfsp->vfs_ltop++;
	for (i = s5vfsp->vfs_nindir, s5vfsp->vfs_nshift = 0; i > 1; i >>= 1)
		s5vfsp->vfs_nshift++;
	for (i = s5vfsp->vfs_inopb, s5vfsp->vfs_inoshift = 0; i > 1; i >>= 1)
		s5vfsp->vfs_inoshift++;
	return 0;
}
