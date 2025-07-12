/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)at:uts/i386/io/kdvm/kdvm_cgi.c	1.1"

/* Enhanced Application Compatibility Support */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/immu.h"
#include "sys/proc.h"	
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/user.h"
#include "sys/inline.h"
#include "sys/kmem.h"
#include "sys/cmn_err.h"
#include "sys/vt.h"
#include "sys/at_ansi.h"
#include "sys/uio.h"
#include "sys/kd.h"
#include "sys/xque.h"
#include "sys/stream.h"
#include "sys/termios.h"
#include "sys/strtty.h"
#include "sys/stropts.h"
#include "sys/ws/ws.h"
#include "sys/ws/chan.h"
#include "sys/vid.h"
#include "sys/vdc.h"
#include "sys/cred.h"
#include "vm/as.h"
#include "vm/seg.h"
#include "vm/seg_objs.h"
#include "sys/mman.h"
#include "sys/ddi.h"

#include "sys/kdvm_cgi.h"

extern wstation_t  Kdws;

cgi_mapclass(chp, arg, rvalp)
int     *rvalp;
{
	int i, rv = 0;
	extern struct cgi_class cgi_classlist[];
	struct cgi_class *vcp;
#define MAXCLN	64
	char name[MAXCLN];
	faddr_t cgi_umapinit();

	for(i=0; i<MAXCLN; i++)
	{
		if(0 == (name[i] = fubyte(arg++)))
			break;
		if(-1 == name[i])
		{
			return(EFAULT);
		}
	}
	if(MAXCLN==i)
	{			/* name is garbage */
		return(EINVAL);
	}

	for(vcp=cgi_classlist; vcp->name; vcp++)
		if(!strcmp(name, vcp->name))		/* S018 */
			break;

	if(!vcp->name)
	{			/* name is not found */
		return(ENXIO);
	}

	if((*rvalp = (int *) cgi_umapinit(vcp->base, vcp->size)) != NULL)
		return(cgi_ioprivl(1, vcp->ports));
	return(EIO);
}

faddr_t
cgi_umapinit(chp, base, size)
channel_t	*chp;
{
 	struct kd_memloc	memloc;
	struct proc	*procp;
	faddr_t vaddr;
	struct map_info *map_p = &Kdws.w_map;

        if (chp != ws_activechan(&Kdws))
		return(NULL);

	drv_getparm(UPROCP, &procp);
	if (map_p->m_procp && map_p->m_procp != procp ||
					 map_p->m_cnt == CH_MAPMX)
		return(NULL);

	map_addr(&vaddr, size, (off_t)0, 1);
	if(vaddr == NULL)
		return(vaddr);

	memloc.vaddr = vaddr;
	memloc.physaddr = base;
	memloc.length = size;
	memloc.ioflg = 0;

	ws_mapavail(chp, map_p);
	
	if (!kdvm_xenixmap(procp, chp, map_p, &memloc))
	{
		(void) as_unmap(procp->p_as, vaddr, size);
		return(NULL);
	}
	return(vaddr);
}

/*
 * Grant or revoke permission to do
 * direct OUTs from user space.
 *
 */

cgi_ioprivl(arg, ports)
struct portrange *ports;
{
	short maxport, curport, ioports[2];	/* keep it simple for now */
	extern int enableio(), disableio();
	
	ioports[1] = 0;		/* delimit the (very short) list */
	for ( ; ports->count; ports++) {

		maxport = ports->first + ports->count;
		if (maxport > MAXTSSIOADDR) {
			return(EIO);
		}

		for (curport = ports->first; curport < maxport; curport++) {
			ioports[0] = curport;
			arg ?  enableio(ioports) : disableio(ioports);
		}
	}
	return 0;
}


/* End Enhanced Application Compatibility Support */
