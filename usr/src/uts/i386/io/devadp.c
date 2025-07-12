/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-io:devadp.c	1.1"
/*
 * Indirect driver for controlling tty.
 */
#include "sys/types.h"
#include "sys/param.h"
#include "sys/immu.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/proc.h"
#include "sys/user.h"
#include "sys/conf.h"
#include "sys/tty.h"
#include "sys/stream.h"
#include "sys/strsubr.h"
#include "sys/cred.h"
#include "sys/cred.h"
#include "sys/conf.h"
#include "sys/ddi.h"
#include "sys/kd.h"
#include "sys/termio.h"
#include "sys/termios.h"
#include "sys/strtty.h"
#include "sys/xque.h"
#include "sys/ws/ws.h"
#include "sys/xque.h"
#include "sys/ws/ws.h"


#define	MONO_MINOR	1
#define	CGA_MINOR	2
#define	EGA_MINOR	4
#define	VGA_MINOR	5

static struct {
	int	model;
	int	minor;
	int	type;
} videos[] = {
	KD_MONO, MONO_MINOR, KD_MONO,
	KD_HERCULES, MONO_MINOR, KD_MONO,
	KD_CGA, CGA_MINOR, KD_CGA,
	KD_EGA, EGA_MINOR, KD_EGA,
	KD_VGA, VGA_MINOR, KD_VGA,
	KD_VDC400, VGA_MINOR, KD_VGA,
	KD_VDC750, VGA_MINOR, KD_VGA,
	KD_VDC600, VGA_MINOR, KD_VGA,
#ifdef EVGA
	KD_EVGA, VGA_MINOR, KD_VGA,
#endif
	0, 0, 0
};

int adpdevflag = 0;

extern struct vnode *common_specvp();
extern wstation_t	Kdws;


/* ARGSUSED */
int
adpopen(devp, flag, otyp, cr)
	dev_t *devp;
	int flag;
	int otyp;
	struct cred *cr;
{
	int	dev, error = ENAVAIL;
	vidstate_t	*vp = &Kdws.w_vstate;
	register i;

	
	for(i=0;videos[i].model; i++) 
		if(videos[i].minor == getminor(*devp) &&
		   	(videos[i].model == vp->v_type ||
		   	videos[i].type  == vp->v_type )) {
				error = 0;
				break;
			}
	if(error) {
		u.u_error = error;
		return(-1);
	}
		
	if(get_vt_dev(&dev, cr, 0) < 0)
		return(-1);

	return ws_open (dev, flag, otyp, cr);
}

/* ARGSUSED */
int
adpclose(dev, flag, otyp, cr)
	dev_t dev;
	int flag;
	int otyp;
	struct cred *cr;
{
	return 0;
}

/* ARGSUSED */
int
adpread(dev, uiop, cr)
	dev_t dev;
	struct uio *uiop;
	struct cred *cr;
{
	if(get_vt_dev(&dev, cr, 1) < 0)
		return(-1);

	return ws_read (dev, uiop, cr);
}

/* ARGSUSED */
int
adpwrite(dev, uiop, cr)
	dev_t dev;
	struct uio *uiop;
	struct cred *cr;
{
	if(get_vt_dev(&dev, cr, 1) < 0)
		return(-1);

	return ws_write (dev, uiop, cr);
}


/* ARGSUSED */
int
adpioctl(dev, cmd, arg, mode, cr, rvalp)
	dev_t dev;
	int cmd;
	int arg;
	int mode;
	struct cred *cr;
	int *rvalp;
{
	if(get_vt_dev(&dev, cr, 1) < 0)
		return(-1);

	return ws_ioctl (dev, cmd, arg, mode, cr, rvalp);
}

int
get_vt_dev(devp, cr, flag)
dev_t	*devp;
struct cred *cr;
int	flag;

{

	int ws_getvtdev();

	if(ws_getvtdev(devp) < 0)
		return(-1);
	if(flag) {
		int	rval;
		if(ws_ioctl (*devp, CONSADP, 0, 0, cr, &rval) != -1)
			*devp = makedevice(getmajor(*devp), rval);		
	}
	return(0);
}
