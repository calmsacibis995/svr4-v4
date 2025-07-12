/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dlpi_ether:io/dlpi_ether.c	1.1"
/*	Copyright (c) 1989  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	


/*
 *  This is the common STREAMS interface for DLPI ethernet drivers.
 *  It defines all of the service routines and ioctl's.  Board specific
 *  functions will be called to handle of the dirty details of getting
 *  packet to and from the wire.
 */
#include DLPI_HW_INC
#include "sys/dlpi_ether.h"

	int	DLopen();
	void	DLclose();
STATIC	void	DLcmds(), DLinfo_req(), DLbind_req(), DLunbind_req(),
		DLunitdata_req(), DLloopback(), DLioctl(), DLrsrv(),
		DLwput(), DLerror_ack(), DLuderror_ind();
STATIC	mblk_t	*DLmk_ud_ind();

static	uchar_t *copy_local_addr();
static	int	is_us(), is_broadcast();

extern	int	DLxmit_packet(), DLpromisc_off(), DLpromisc_on(),
		DLset_eaddr(), DLadd_multicast(), DLdel_multicast(),
		DLdisable(), DLenable(), DLreset(), DLis_multicast();

extern	void		splx();
extern	int		strlen();
extern	ushort_t	ntohs();

typedef int (*PSFI)();	/* Pointer to STREAMS Function return Int */

STATIC	struct	module_info	DLrminfo = {
	DL_ID, DL_NAME, DL_MIN_PACKET, DL_MAX_PACKET, DL_HIWATER, DL_LOWATER,
};
STATIC	struct	module_info	DLwminfo = {
	DL_ID, DL_NAME, DL_MIN_PACKET, DL_MAX_PACKET, DL_HIWATER, DL_LOWATER,
};
STATIC	struct	qinit		DLrinit = {
	NULL, (PSFI)DLrsrv, DLopen, (PSFI)DLclose, NULL, &DLrminfo, NULL,
};
STATIC	struct	qinit		DLwinit = {
	(PSFI)DLwput, NULL, NULL, NULL, NULL, &DLwminfo, NULL,
};

struct	streamtab	DLinfo = { &DLrinit, &DLwinit, NULL, NULL, };

extern	int		DLboards;
extern	DL_bdconfig_t	DLconfig[];
extern	DL_sap_t	DLsaps[];
extern	int		DLstrlog;
extern	char		DLid_string[];
extern	struct	ifstats	*DLifstats;

int	DLdevflag = 0;		/* V4.0 style driver */

/******************************************************************************
 *  DLopen()
 */
/* ARGSUSED */
DLopen(q, dev, flag, sflag, credp)
queue_t		*q;
dev_t		*dev;
int		flag;
int		sflag;
struct	cred	*credp;
{
	int		i, old;
	DL_bdconfig_t	*bd;
	DL_sap_t	*sap;
	major_t	major = getmajor(*dev);
	minor_t	minor = getminor(*dev);

	DL_LOG(strlog(DL_ID, 0, 3, SL_TRACE,
		"DLopen - major %d minor %d queue %x",
					(int)major, (int)minor, (int)q));

	/*
	 *  Find the board structure for this major number.
	 */
	for (i = DLboards, bd = DLconfig; i; bd++, i--)
		if (bd->major == major)
			break;
	
	if (i == 0) {
		DL_LOG(strlog(DL_ID, 0, 1, SL_TRACE,
			"DLopen - invalid major number (%d)", (int)major));
		return (ENXIO);
	}

	/*
	 *  Check if we found a board there.
	 */
	if (!(bd->flags & BOARD_PRESENT)) {
		DL_LOG(strlog(DL_ID, 0, 1, SL_TRACE,
			"DLopen - board for major (%d) not installed",
								(int)major));
		return (ENXIO);
	}
		
	/*
	 *  If it's a clone device, assign a minor number.
	 */
	if (sflag == CLONEOPEN) {
		for (i = 0, sap = bd->sap_ptr; i < bd->max_saps; i++, sap++)
			if (sap->state == DL_UNATTACHED)
				break;

		if (i == bd->max_saps) {
			DL_LOG(strlog(DL_ID, 0, 1, SL_TRACE,
				"DLopen - no minors left"));
			return (ECHRNG);
		}
		else
			minor = (minor_t) i;
	} else if (minor > bd->max_saps) {
		DL_LOG(strlog(DL_ID, 0, 1, SL_TRACE,
			"DLopen - invalid minor number (%d)", (int)minor));
		return (ECHRNG);
	}

	/*
	 *  If this stream has not been opened before, set it up.
	 */
	if (!q->q_ptr) {
		old = splstr();

		sap->state   = DL_UNBOUND;
		sap->read_q  = q;
		sap->write_q = WR(q);

		q->q_ptr = (caddr_t) sap;
		WR(q)->q_ptr = q->q_ptr;

		/*
		 *  Need to keep track of priviledge for later reference in 
		 *  bind requests.
		 */
		if (drv_priv(credp) == 0)
			sap->flags |= PRIVILEDGED;

		splx(old);
	}

	*dev = makedevice(major, minor);
	return (0);
}

/******************************************************************************
 *  DLclose()
 */
void
DLclose(q)
queue_t	*q;
{
	DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
	int		old;

	DL_LOG(strlog(DL_ID, 1, 3, SL_TRACE, "DLclose queue %x", (int)q));

	/*
	 *  If this was a promiscuous SAP, have board put out of promiscuous
	 *  mode.  If there are still promiscuous SAPs running, the call may
	 *  have no effect.
	 */
	if (sap->flags & PROMISCUOUS)
		(void)DLpromisc_off(sap->bd);

	/*
	 *  Cleanup SAP structure
	 */
	old = splstr();

	sap->state    = DL_UNATTACHED;
	sap->sap_addr = 0;
	sap->read_q   = NULL;
	sap->write_q  = NULL;
	sap->flags    = 0;

	splx(old);

	return;
}

/******************************************************************************
 *  DLwput()
 */
STATIC void
DLwput(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	DL_LOG(strlog(DL_ID, 2, 3, SL_TRACE,
		"DLwput queue %x type %d", (int)q, mp->b_datap->db_type));

	switch(mp->b_datap->db_type) {
		/*
		 *  Process data link commands.
		 */
		case M_PROTO:
		case M_PCPROTO:
			DLcmds(q, mp);
			break;
		/*
		 *  Flush read and write queues.
		 */
		case M_FLUSH:
			if (*mp->b_rptr & FLUSHW)
				flushq(q, FLUSHDATA);
			if (*mp->b_rptr & FLUSHR) {
				flushq(RD(q), FLUSHDATA);
				*mp->b_rptr &= ~FLUSHW;
				qreply(q, mp);
			} else
				freemsg(mp);
			break;
		/*
		 *  Process ioctls.
		 */
		case M_IOCTL:
			DLioctl(q, mp);
			break;
		/* 
		 *  Anything else we dump.
		 */
		case M_DATA:
		default:
			DL_LOG(strlog(DL_ID, 2, 1, SL_TRACE,
				"DLwput - unsupported type (%d)",
							mp->b_datap->db_type));
			freemsg(mp);
			break;
	}

	return;
}

/******************************************************************************
 *  DLcmds()
 */
STATIC void
DLcmds(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	/* LINTED pointer assignment */
	union DL_primitives  *dl = (union DL_primitives *)mp->b_datap->db_base;

	switch (dl->dl_primitive) {
		case DL_INFO_REQ:
			DLinfo_req(q);
			break;
		
		case DL_BIND_REQ:
			DLbind_req(q, mp);
			break;
		
		case DL_UNBIND_REQ:
			DLunbind_req(q, mp);
			break;
		
		case DL_UNITDATA_REQ:
			DLunitdata_req(q, mp);
			return;			/* don't free the message */

		case DL_ATTACH_REQ:
		case DL_DETACH_REQ:
		case DL_SUBS_BIND_REQ:
		case DL_UDQOS_REQ:
		case DL_CONNECT_REQ:
		case DL_CONNECT_RES:
		case DL_TOKEN_REQ:
		case DL_DISCONNECT_REQ:
		case DL_RESET_REQ:
		case DL_RESET_RES:
			DLerror_ack(q, mp, DL_NOTSUPPORTED);
			break;
		
		default:
			DLerror_ack(q, mp, DL_BADPRIM);
			break;
			
	}

	/*
	 *  Free the request.
	 */
	freemsg(mp);
}

/******************************************************************************
 *  DLinfo_req()
 */
STATIC void
DLinfo_req(q)
queue_t	*q;
{
	dl_info_ack_t	*info_ack;
	mblk_t		*resp;
	DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
	
	DL_LOG(strlog(DL_ID, 3, 3, SL_TRACE,
		"DLinfo request for queue %x", (int)q));

	/*
	 *  If we can't get the memory, just ignore the request.
	 */
	if ((resp = allocb(DL_PRIMITIVES_SIZE + DL_TOTAL_ADDR_LEN, BPRI_MED)) == NULL)
		return;

	/* LINTED pointer assignment */
	info_ack			= (dl_info_ack_t *)resp->b_wptr;
	info_ack->dl_primitive		= DL_INFO_ACK;
	info_ack->dl_max_sdu		= sap->max_spdu;
	info_ack->dl_min_sdu		= sap->min_spdu;
	info_ack->dl_addr_length	= DL_TOTAL_ADDR_LEN;
	info_ack->dl_mac_type		= sap->mac_type;
	info_ack->dl_current_state	= sap->state;
	info_ack->dl_service_mode	= sap->service_mode;
	info_ack->dl_qos_length		= 0;
	info_ack->dl_qos_offset		= 0;
	info_ack->dl_qos_range_length	= 0;
	info_ack->dl_qos_range_offset	= 0;
	info_ack->dl_provider_style	= sap->provider_style;
	info_ack->dl_addr_offset	= DL_INFO_ACK_SIZE;
	info_ack->dl_growth		= 0;

	resp->b_wptr += DL_INFO_ACK_SIZE;
	resp->b_datap->db_type = M_PCPROTO;

	/*
	 *  DLPI spec says if stream is not bound, address will be 0.
	 */
	if (sap->state == DL_IDLE) {
		/* LINTED pointer assignment */
		resp->b_wptr = copy_local_addr(sap, (ushort_t*)resp->b_wptr);
		info_ack->dl_addr_offset = DL_INFO_ACK_SIZE;
	}
	else
		info_ack->dl_addr_offset = 0;

	qreply(q, resp);
}

/******************************************************************************
 *  DLbind_req()
 */
STATIC void
DLbind_req(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	/* LINTED pointer assignment */
	dl_bind_req_t	*bind_req  = (dl_bind_req_t *)mp->b_datap->db_base;
	DL_sap_t	*sap       = (DL_sap_t *)q->q_ptr;
	DL_sap_t	*tmp;
	dl_bind_ack_t	*bind_ack;
	mblk_t		*resp;
	int		 old, i;

	DL_LOG(strlog(DL_ID, 4, 3, SL_TRACE,
		"DLbind request to sap 0x%x on queue %x",
				((int)bind_req->dl_sap) & 0xffff, (int)q));

	/*
	 *  If the stream is already bound, return an error.
	 */
	if (sap->state != DL_UNBOUND) {
		DL_LOG(strlog(DL_ID, 4, 1, SL_TRACE,
			"DLbind not valid for state %d on queue %x",
							sap->state, (int)q));
		DLerror_ack(q, mp, DL_OUTSTATE);
		return;
	}

	/*
	 *  Only a prevledged user can bind to the promiscuous SAP or a SAP
	 *  that is already bound.
	 */
	if (!(sap->flags & PRIVILEDGED)) {
		if ((ushort_t)bind_req->dl_sap == PROMISCUOUS_SAP) {
			DLerror_ack(q, mp, DL_ACCESS);
			return;
		}

		tmp = sap->bd->sap_ptr;
		i   = sap->bd->max_saps;
		while (i--) {
			if ((tmp->state    == DL_IDLE) &&
			    (tmp->sap_addr == (ushort_t)bind_req->dl_sap)) {
				DLerror_ack(q, mp, DL_BOUND);
				return;
			}
			tmp++;
		}

	}

	/*
	 *  Allocate memory for the response.  If none, just drop the request.
	 */
	if ((resp = allocb(DL_PRIMITIVES_SIZE + DL_TOTAL_ADDR_LEN, BPRI_MED)) == NULL) {
		DL_LOG(strlog(DL_ID, 4, 1, SL_TRACE,
			"DLbind allocb failure on queue %x", (int)q));
		return;
	}

	/*
	 *  Assign the SAP and set state.
	 */
	old = splstr();

	sap->sap_addr = (ushort_t)bind_req->dl_sap;  /* Stored in host order */
	sap->state    = DL_IDLE;

	splx(old);

	/* LINTED pointer assignment */
	bind_ack		 = (dl_bind_ack_t *)resp->b_wptr;
	bind_ack->dl_primitive   = DL_BIND_ACK;
	bind_ack->dl_sap	 = sap->sap_addr;
	bind_ack->dl_addr_length = DL_TOTAL_ADDR_LEN;
	bind_ack->dl_addr_offset = DL_BIND_ACK_SIZE;
	bind_ack->dl_max_conind  = 0;
	bind_ack->dl_growth      = 0;

	resp->b_wptr          += DL_BIND_ACK_SIZE;
	resp->b_datap->db_type = M_PCPROTO;

	/* LINTED pointer assignment */
	resp->b_wptr = copy_local_addr(sap, (ushort_t*)resp->b_wptr);

	qreply(q, resp);
}

/******************************************************************************
 *  DLunbind_req()
 */
STATIC void
DLunbind_req(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
	mblk_t		*resp;
	dl_ok_ack_t	*ok_ack;
	int		 old;

	DL_LOG(strlog(DL_ID, 5, 3, SL_TRACE,
		"DLunbind request for sap 0x%x on queue %x",
							sap->sap_addr, (int)q));

	/*
	 *  See if we are in the proper state to honor this request.
	 */
	if (sap->state != DL_IDLE) {
		DL_LOG(strlog(DL_ID, 5, 1, SL_TRACE,
			"DLunbind not valid for state %d on queue %x",
							sap->state, (int)q));
		DLerror_ack(q, mp, DL_OUTSTATE);
		return;
	}
		
	/* 
	 *  Allocate memory for response.  If none, toss the request.
	 */
	if ((resp = allocb(DL_PRIMITIVES_SIZE, BPRI_MED)) == NULL) {
		DL_LOG(strlog(DL_ID, 5, 1, SL_TRACE,
			"DLunbind allocb failure on queue %x", (int)q));
		return;
	}

	/*
	 *  Mark the SAP out of service and flush both queues
	 */
	old = splstr();
	sap->state    = DL_UNBOUND;
	sap->sap_addr = 0;
	splx(old);

	flushq(q, FLUSHDATA);
	flushq(RD(q), FLUSHDATA);

	/*
	 *  Generate and send the response.
	 */
	/* LINTED pointer assignment */
	ok_ack                       = (dl_ok_ack_t *)resp->b_wptr;
	ok_ack->dl_primitive         = DL_OK_ACK;
	ok_ack->dl_correct_primitive = DL_UNBIND_REQ;

	resp->b_wptr          += DL_OK_ACK_SIZE;
	resp->b_datap->db_type = M_PCPROTO;
	qreply(q, resp);

	return;
}

/******************************************************************************
 *  DLunitdata_req()
 */
STATIC void
DLunitdata_req(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	/* LINTED pointer assignment */
	dl_unitdata_req_t	*data_req = (dl_unitdata_req_t *)mp->b_rptr;
	DL_sap_t		*sap      = (DL_sap_t *)q->q_ptr;
	int			 size     = msgdsize(mp->b_cont);
	DL_eaddr_t		*ether_addr;
	int			 local, multicast, old;


	DL_LOG(strlog(DL_ID, 6, 3, SL_TRACE,
		"DLunitdata request of %d bytes for sap 0x%x on queue %x",
					size, sap->sap_addr, (int)q));

	/*
	 *  If the board has gone down, reject the request.
	 */
	if((sap->bd->flags & (BOARD_PRESENT | BOARD_DISABLED))!=BOARD_PRESENT) {
		DL_LOG(strlog(DL_ID, 6, 1, SL_TRACE,
			"DLunitdata request on disabled board for queue %x",
								(int)q));
		DLerror_ack(q, mp, DL_NOTINIT);
		return;
	}

	/*
	 *  Check for proper state and frame size.
	 */
	if (sap->state != DL_IDLE) {
		DL_LOG(strlog(DL_ID, 6, 1, SL_TRACE,
			"DLunitdata not valid for state %d on queue %x",
							sap->state, (int)q));
		DLuderror_ind(q, mp, DL_OUTSTATE);
		return;
	}
		
	if ((size > sap->max_spdu) || (size < sap->min_spdu)) {
		DL_LOG(strlog(DL_ID, 6, 1, SL_TRACE,
			"DLunitdata frame size of %d is invalid for queue %x",
								size, (int)q));
		DLuderror_ind(q, mp, DL_BADDATA);
		return;
	}

	/*
	 *  Check for frame that we should send to ourself.
	 */
	/* LINTED pointer assignment */
	ether_addr = (DL_eaddr_t*)(mp->b_rptr + data_req->dl_dest_addr_offset);
	local = is_us(sap, ether_addr);
	multicast = is_broadcast(ether_addr) || DLis_multicast(sap->bd, ether_addr);

	if (multicast || local)
		DLloopback(q, mp, ether_addr, multicast);

	/*
	 *  Update SNMP stats
	 */
	if (multicast)
		sap->bd->mib.ifOutNUcastPkts++;		/* SNMP */
	else
		sap->bd->mib.ifOutUcastPkts++;		/* SNMP */

	/*
	 *  Send frame.  If frame is local (not broadcast/multicast), only
	 *  send it to the net if that's how we're configured.
	 */
	if (local && !(sap->flags & SEND_LOCAL_TO_NET)) {
		freemsg(mp);
		return;
	}

	/*
	 *  If controller is not busy, transmit the frame.  Otherwise put
	 *  it on our queue for the tx interrupt routine to handle.
	 */
	old = splstr();

	if (sap->bd->flags & TX_BUSY) {
		sap->bd->flags |= TX_QUEUED;
		sap->bd->mib.ifOutQlen++;		/* SNMP */
		putq(q, mp);

		DL_LOG(strlog(DL_ID, 6, 2, SL_TRACE,
			"DLunitdata hardware busy - xmit deferred for queue %x",
								(int)q));
	} else if (DLxmit_packet(q, mp) == -1)
		DLuderror_ind(q, mp, DL_UNDELIVERABLE);

	splx(old);
	return;
}

/******************************************************************************
 *  DLioctl()
 */
STATIC void
DLioctl(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	/* LINTED pointer assignment */
	struct iocblk	*ioctl_req = (struct iocblk *)mp->b_rptr;
	DL_sap_t	*sap       = (DL_sap_t *)q->q_ptr;
	DL_bdconfig_t	*bd	   = sap->bd;
	int		old, failed, size1, size2;

	DL_LOG(strlog(DL_ID, 8, 3, SL_TRACE,
		"DLioctl request for command %d on queue %x",
					ioctl_req->ioc_cmd, (int)q));
	/*
	 *  Assume good stuff for now.
	 */
	ioctl_req->ioc_error = 0;
	mp->b_datap->db_type = M_IOCACK;

	/*
	 *  Screen for priviledged requests.
	 */
	switch (ioctl_req->ioc_cmd) {

	case DLIOCSMIB:		/* Set MIB */
	case DLIOCSENADDR:	/* Set ethernet address */
	case DLIOCSLPCFLG:	/* Set local packet copy flag */
	case DLIOCSPROMISC:	/* Toggle promiscuous state */
	case DLIOCADDMULTI:	/* Add multicast address */
	case DLIOCDELMULTI:	/* Delete multicast address */
	case DLIOCDISABLE:	/* Disable controller */
	case DLIOCENABLE:	/* Enable controller */
	case DLIOCRESET:	/* Reset controller */
		/*
		 *  Must be privledged user to do these.
		 */
		if (drv_priv(ioctl_req->ioc_cr)) {
			DL_LOG(strlog(DL_ID, 8, 1, SL_TRACE,
			    "DLioctl invalid priviledge for queue %x", (int)q));

			ioctl_req->ioc_error = EPERM;
			ioctl_req->ioc_count = 0;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			return;
		}
	}

	/*
	 *  Make sure IOCTL's that require buffers have them.
	 */
	switch (ioctl_req->ioc_cmd) {

	case DLIOCGMIB:		/* Get MIB */
	case DLIOCSMIB:		/* Set MIB */
	case DLIOCGENADDR:	/* Get ethernet address */
	case DLIOCSENADDR:	/* Set ethernet address */
	case DLIOCADDMULTI:	/* Add multicast address */
	case DLIOCDELMULTI:	/* Delete multicast address */
		/*
		 * Must have a non-null b_cont pointer.
		 */
		if (!mp->b_cont) {
			DL_LOG(strlog(DL_ID, 8, 1, SL_TRACE,
			    "DLioctl no data supplied by user for queue %x",
								(int)q));
			ioctl_req->ioc_error = EINVAL;
			ioctl_req->ioc_count = 0;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			return;
		}
	}

	/*
	 *  Process request.
	 */
	switch (ioctl_req->ioc_cmd) {

	case DLIOCGMIB:		/* Get MIB */
		/*
		 *  We'll send as much as they asked for.
		 */
		size1 = min(sizeof(DL_mib_t), ioctl_req->ioc_count);
		size2 = min(strlen(DLid_string) + 1, ioctl_req->ioc_count - size1);

		/*
		 *  Set some MIB items before copy
		 */
		bd->mib.ifDescrLen = size2;
		/* LINTED pointer assignment */
		(void)copy_local_addr(sap, (ushort_t*)&bd->mib.ifPhyAddress);
		mp->b_cont->b_wptr = mp->b_cont->b_rptr;

		old = splstr();
		bcopy((caddr_t)&bd->mib, (caddr_t)mp->b_cont->b_wptr, size1);
		splx(old);

		mp->b_cont->b_wptr += size1;

		if (size2) {
			strncpy((char*)mp->b_cont->b_wptr, DLid_string, size2);
			*(char*)(mp->b_cont->b_wptr + size2 - 1) = '\0';
			mp->b_cont->b_wptr += size2;
		}

		ioctl_req->ioc_count = size1 + size2;

		break;

	case DLIOCSMIB:		/* Set MIB */
		/*
		 *  We currently don't let them set the "ifDecr".
		 */
		old = splstr();
		bcopy((caddr_t)mp->b_cont->b_rptr,(caddr_t)&bd->mib,
				min(sizeof(DL_mib_t), ioctl_req->ioc_count));
		ioctl_req->ioc_count = 0;

		if (!(bd->flags & BOARD_PRESENT))
			bd->mib.ifOperStatus = DL_DOWN;

		splx(old);
		break;

	case DLIOCGENADDR:	/* Get ethernet address */
		/*
		 *  We'll send as much as they asked for.
		 */
		size1 = min(sizeof(DL_eaddr_t), ioctl_req->ioc_count);
		bcopy((caddr_t)&bd->eaddr, (caddr_t)mp->b_cont->b_rptr, size1);
		ioctl_req->ioc_count = size1;

		break;

	case DLIOCSENADDR:	/* Set ethernet address */
#ifdef ALLOW_SET_EADDR
		if ((ioctl_req->ioc_count < sizeof(DL_eaddr_t)) ||
			/* LINTED pointer assignment */
		    DLset_eaddr(bd, (DL_eaddr_t*)mp->b_cont->b_rptr)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		} else {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}
#else
		ioctl_req->ioc_error = EINVAL;
		mp->b_datap->db_type = M_IOCNAK;
#endif

		ioctl_req->ioc_count = 0;
		break;

	case DLIOCGLPCFLG:	/* Get local packet copy flag */
		ioctl_req->ioc_rval  = (bd->flags & SEND_LOCAL_TO_NET);
		ioctl_req->ioc_count = 0;
		break;

	case DLIOCSLPCFLG:	/* Set local packet copy flag */
		bd->flags |= SEND_LOCAL_TO_NET;
		ioctl_req->ioc_count = 0;
		break;

	case DLIOCGPROMISC:	/* Get promiscuous state */
		ioctl_req->ioc_rval  = (sap->flags & PROMISCUOUS);
		ioctl_req->ioc_count = 0;
		break;

	case DLIOCSPROMISC:	/* Toggle promiscuous state */
		if (sap->flags & PROMISCUOUS)
			failed = DLpromisc_off(bd);
		else
			failed = DLpromisc_on(bd);

		if (failed) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		} else
			sap->flags ^= PROMISCUOUS;

		ioctl_req->ioc_count = 0;
		break;

	case DLIOCADDMULTI:	/* Add multicast address */
		if ((ioctl_req->ioc_count < sizeof(DL_eaddr_t)) ||
			/* LINTED pointer assignment */
		    DLadd_multicast(bd, (DL_eaddr_t*)mp->b_cont->b_rptr)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}

		ioctl_req->ioc_count = 0;
		break;

	case DLIOCDELMULTI:	/* Delete multicast address */
		if ((ioctl_req->ioc_count < sizeof(DL_eaddr_t)) ||
			/* LINTED pointer assignment */
		    DLdel_multicast(bd, (DL_eaddr_t*)mp->b_cont->b_rptr)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}

		ioctl_req->ioc_count = 0;
		break;
	
	case DLIOCDISABLE:	/* Disable controller */
		if (DLdisable(bd)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}
		else
			bd->flags |= BOARD_DISABLED;
		break;

	case DLIOCENABLE:	/* Enable controller */
		if (DLenable(bd)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}
		else
			bd->flags &= ~BOARD_DISABLED;
		break;

	case DLIOCRESET:	/* Reset controller */
		if (DLreset(bd)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}
		break;
	case SIOCGIFFLAGS:	/* IP get ifnet flags */
		break;		/* just ignore or */

	default:
		DL_LOG(strlog(DL_ID, 8, 1, SL_TRACE,
			"DLioctl unknwon request %d from queue %x",
						ioctl_req->ioc_cmd, (int)q));
		ioctl_req->ioc_error = EINVAL;
		ioctl_req->ioc_count = 0;
		mp->b_datap->db_type = M_IOCNAK;
	}

	qreply(q, mp);
}

/******************************************************************************
 *  DLloopback()
 */
STATIC void
DLloopback(q, mp, dst_addr, multicast)
queue_t		*q;
mblk_t		*mp;
DL_eaddr_t	*dst_addr;
int	multicast;
{
	DL_sap_t		*sap = (DL_sap_t *)q->q_ptr;
	mblk_t			*loop_mp1, *loop_mp2;

	DL_LOG(strlog(DL_ID, 8, 3, SL_TRACE, "DLloopback queue %x", (int)q));

	/*
	 *  See if there is room on the read queue.
	 */
	if (!canput(OTHERQ(q))) {
		DL_LOG(strlog(DL_ID, 8, 2, SL_TRACE,
			"DLloopback read queue %x full - dropping frame",
								(int)q));
		sap->bd->mib.ifOutDiscards++;	/* SNMP */
		sap->bd->mib.ifInDiscards++;	/* SNMP */
		return;
	}

	/*
	 *  Allocate the resources and duplicate the message.
	 */
	loop_mp1 = DLmk_ud_ind(dst_addr, &sap->bd->eaddr, sap->sap_addr);
	loop_mp2 = copymsg(mp->b_cont);

	if (!loop_mp1 || !loop_mp2) {
		DL_LOG(strlog(DL_ID, 8, 1, SL_TRACE,
			"DLlooback no resources for queue %x %x %x",
					(int)q, (int)loop_mp1, (int)loop_mp2));
		if (loop_mp1)
			freemsg(loop_mp1);
		if (loop_mp2)
			freemsg(loop_mp2);

		sap->bd->mib.ifOutDiscards++;	/* SNMP */
		sap->bd->mib.ifInDiscards++;	/* SNMP */

		return;
	}

	/*
	 *  Link and send.
	 */
	linkb(loop_mp1, loop_mp2);
	putq(OTHERQ(q), loop_mp1);

	/*
	 *  Update SNMP stats
	 */
	if (multicast)
		sap->bd->mib.ifInNUcastPkts++;	/* SNMP */
	else
		sap->bd->mib.ifInUcastPkts++;	/* SNMP */
	
	return;
}

/******************************************************************************
 *  DLrsrv()
 */
STATIC void
DLrsrv(q)
queue_t	*q;
{
	DL_sap_t	*sap;
	DL_sap_t	*next_sap;
	DL_bdconfig_t	*bd;
	mblk_t		*mp_data, *mp_ind, *mp_dup;
	DL_ether_hdr_t	*hdr;
	int		i;
	ushort_t	sap_id;

	/*
	 *  Process all messages waiting.
	 */
	while (mp_data = getq(q)) {
		/*
		 *  Set some convient pointers.
		 */
		/* LINTED pointer assignment */
		hdr = (DL_ether_hdr_t*)mp_data->b_rptr;
		sap = (DL_sap_t *)q->q_ptr;
		bd  = sap->bd;

		/*
		 *  Convert the SAP to host order
		 */
		sap_id = ntohs(hdr->len_type);

		/*
		 *  Create an indication message for this frame.
		 */
		if ((mp_ind = DLmk_ud_ind(&hdr->dst, &hdr->src, sap_id)) == NULL) {
			freemsg(mp_data);

			DL_LOG(strlog(DL_ID, 9, 1, SL_TRACE,
				"DLrsrv: - No receive buffer resources"));
			bd->mib.ifInDiscards++;		/* SNMP */
			continue;
		}

		/*
		 *  Strip the ethernet header now that the indication message
		 *  has been made.
		 */
		mp_data->b_rptr += sizeof(DL_ether_hdr_t);
		mp_ind->b_datap->db_type = M_PROTO;
		linkb(mp_ind, mp_data);

		/*
		 *  Go through the SAP list and see if anyone else is
		 *  interested in this frame.
		 */
		next_sap = sap->bd->sap_ptr;
		for(i = bd->max_saps; i; i--, next_sap++) {
			/*
			 *  Skip SAP indication this came in on.
			 */
			if (next_sap == sap)
				continue;

			if ( (next_sap->state    == DL_IDLE)          &&
			    ((next_sap->sap_addr == sap_id) ||
			     (next_sap->sap_addr == PROMISCUOUS_SAP)) &&
			      canput(next_sap->read_q)		      &&
			     (mp_dup = dupmsg(mp_ind)))

				putnext(next_sap->read_q, mp_dup);
		}

		/*
		 *  Don't forget the one who brung us.
		 */
		if (canput(sap->read_q)) {
			putnext(sap->read_q, mp_ind);

			DL_LOG(strlog(DL_ID, 9, 3, SL_TRACE, "DLrsrv queue %x",
							(int)sap->read_q));
			/*
			 *  Update SNMP stats.
			 */
			if (IS_MULTICAST(hdr->dst))		/* SNMP */
				bd->mib.ifInNUcastPkts++;
			else
				bd->mib.ifInUcastPkts++;	/* SNMP */
		} else {
			freemsg(mp_ind);
			bd->mib.ifInDiscards++;			/* SNMP */
		}
	}

	return;
}

/******************************************************************************
 *  DLmk_ud_ind()
 */
STATIC	mblk_t*
DLmk_ud_ind(dst, src, sap_id)
DL_eaddr_t	*dst;
DL_eaddr_t	*src;
ushort_t	 sap_id;	/* in host order */
{
	dl_unitdata_ind_t	*data_ind;
	mblk_t			*mp;
	register ushort_t	*word_p;
	register int		i;

	/*
	 *  Allocate the indication buffer with room for ethernet source,
	 *  destination, and SAP addresses.
	 */
	if ((mp = allocb(DL_UNITDATA_IND_SIZE + (sizeof(DL_eaddr_t)  * 2) +
						(sizeof(ushort_t)    * 2),
							BPRI_MED)) == NULL)
		return (NULL);

	/*
	 *  Fill out the indication.
	 */
	/* LINTED pointer assignment */
	data_ind                      =(dl_unitdata_ind_t *)mp->b_wptr;
	data_ind->dl_primitive        = DL_UNITDATA_IND;
	data_ind->dl_dest_addr_length = DL_TOTAL_ADDR_LEN;
	data_ind->dl_dest_addr_offset = DL_UNITDATA_IND_SIZE;
	data_ind->dl_src_addr_length  = DL_TOTAL_ADDR_LEN;
	data_ind->dl_src_addr_offset  = DL_UNITDATA_IND_SIZE + DL_TOTAL_ADDR_LEN;

	/* LINTED pointer assignment */
	word_p = (ushort_t*)(mp->b_wptr + DL_UNITDATA_IND_SIZE);

	/*
	 *  Copy destination address.
	 */
	for (i = 0; i < (sizeof(DL_eaddr_t) / 2); i++)
		*word_p++ = dst->words[ i ];
	*word_p++ = sap_id;

	/*
	 *  Copy source address.
	 */
	for (i = 0; i < (sizeof(DL_eaddr_t) / 2); i++)
		*word_p++ = src->words[ i ];
	*word_p++ = sap_id;

	/*
	 *  Set new write pointer.
	 */
	mp->b_wptr = (uchar_t*)word_p;

	return (mp);
}

/******************************************************************************
 *  DLerror_ack()
 */
STATIC	void
DLerror_ack(q, mp, error)
queue_t	*q;
mblk_t	*mp;
int	error;
{
	/* LINTED pointer assignment */
	union	DL_primitives	*prim = (union DL_primitives*)mp->b_rptr;
	dl_error_ack_t		*err_ack;
	mblk_t			*resp;

	/*
	 *  Allocate the response resource.  If we can't, just return.
	 */
	if ((resp = allocb(sizeof(union DL_primitives), BPRI_MED)) == NULL) {
		DL_LOG(strlog(DL_ID, 10, 1, SL_TRACE,
		    "DLerror_ack - no resources for error response on queue %x",
								(int)q));
		return;
	}

	/*
	 *  Fill it in.
	 */
	/* LINTED pointer assignment */
	err_ack                     = (dl_error_ack_t*)resp->b_wptr;
	err_ack->dl_primitive       = DL_ERROR_ACK;
	err_ack->dl_error_primitive = prim->dl_primitive;
	err_ack->dl_errno           = (ulong)error;
	err_ack->dl_unix_errno      = 0;

	resp->b_wptr += DL_ERROR_ACK_SIZE;
	resp->b_datap->db_type = M_PCPROTO;

	/*
	 *  Send it
	 */
	qreply(q, resp);
	return;
}

/******************************************************************************
 *  DLuderror_ind()
 */
STATIC	void
DLuderror_ind(q, mp, error)
queue_t	*q;
mblk_t	*mp;
int	error;
{
	/* LINTED pointer assignment */
	dl_uderror_ind_t   *uderr_ind = (dl_uderror_ind_t*)mp->b_rptr;

	/*
	 *  The unit data request is guaranteed to accomodate a unit data
	 *  error indication so we will just convert the data request.
	 */
	uderr_ind->dl_primitive = DL_UDERROR_IND;
	uderr_ind->dl_reserved  = 0;
	uderr_ind->dl_errno     = (ulong)error;

	qreply(q, mp);
	return;
}

/******************************************************************************
 *  is_us()
 */
static
is_us(sap, dst)
DL_sap_t	*sap;
DL_eaddr_t	*dst;
{
	DL_eaddr_t	*src = &sap->bd->eaddr;
	int	i;

	for (i = 0; i < (sizeof(DL_eaddr_t) / 2); i++)
		if (dst->words[ i ] != src->words[ i ])
			return (0);
	return (1);
}

/******************************************************************************
 *  is_broadcast()
 */
static
is_broadcast(dst)
DL_eaddr_t		*dst;
{
	int	i;

	for (i = 0; i < (sizeof(DL_eaddr_t) / 2); i++)
		if (dst->words[ i ] != 0xffff)
			return (0);
	return (1);
}

/******************************************************************************
 *  ntoa()
 */
static char
ntoa(val)
uchar_t	val;
{
	if (val < 10)
		return (val + 0x30);
	else
		return (val + 0x57);
}

/******************************************************************************
 *  DLprint_eaddr()
 */
void
DLprint_eaddr(addr)
uchar_t	addr[];
{
	int	i;
	char	a_eaddr[ DL_MAC_ADDR_LEN * 3 ];

	for (i = 0; i < DL_MAC_ADDR_LEN; i++) {
		a_eaddr[ i * 3     ] = ntoa((uchar_t)(addr[ i ] >> 4 & 0x0f));
		a_eaddr[ i * 3 + 1 ] = ntoa((uchar_t)(addr[ i ]      & 0x0f));
		a_eaddr[ i * 3 + 2 ] = ':';
	}

	a_eaddr[ sizeof(a_eaddr) - 1 ] = '\0';
	cmn_err(CE_CONT, "--- Ethernet Address: %s\n", a_eaddr);
}

/******************************************************************************
 *  copy_local_addr()
 */
static uchar_t*
copy_local_addr(sap, dst)
DL_sap_t	*sap;
ushort_t	*dst;
{
	ushort_t	*src = sap->bd->eaddr.words;
	int		i;

	for (i = 0; i < (sizeof (DL_eaddr_t)) / 2; i++)
		*dst++ = *src++;
	*dst++ = sap->sap_addr;

	return((uchar_t*)dst);
}
