/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_connect.c	1.1.1.2"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)p_connect.c	4.11	LCC);	/* Modified: 16:03:49 1/4/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/* History:
   [1/26/88 PD] Peter Dobson - corrected log message to print out correct
	values.
   [2/26/88 PD] added code to make RS232 em recieve loop exit if DOSOUT program
	has quit.
   [3/9/90 SCAHNG] made DOSSVR eat the ACK from PC BRIDGE, if emulation session
		has gone. 
*/

#define	TI_CHECK_INTERVAL	5	/* log a check every 5 packets */
#define	TI_DUP_WINDOW		128	/* size of the window to forget about dups */

/*			 PC-INTERFACE  Configuration Manager		*/
#include	"pci_types.h"
#include	<errno.h>
#include	<pwd.h>
#ifdef SHADOW_PASSWD
#include	<shadow.h>
#endif

#if defined(SYS5_4)
#include	<sys/filio.h>
#include	<sys/stropts.h>
#include	<stdlib.h>
#endif

#include	<fcntl.h>
#include	<ctype.h>
#include	<string.h>
#include	<signal.h>
#ifdef	RIDGE
#include	<bsdioctl.h>
#endif	/* RIDGE */

#ifdef	ETHNETPCI
#ifdef	ETHLOCUS
#include	<eth.h>			/* LOCUS Ethernet structs/constants */
#endif	  /* ETHLOCUS  */

#ifdef	ETH3BNET
#include	<ni.h>			/* AT&T Ethernet structs/constants */
#endif	  /* ETH3BNET  */

#endif   /* ETHNETPCI  */

#ifdef RS232PCI
#ifdef	XENIX
#include	<ioctl.h>
#endif	  /* XENIX  */
#include	<termio.h>		/* TTY structs/constants */
#endif   /* RS232PCI  */

#include	<stat.h>
#include	"const.h"
#include	"flip.h"

#ifdef	LOCUS	/* Locus site info stuff */
#include	"pw_gecos.h"
#endif	/* LOCUS */

#ifdef RLOCK      /* record locking */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <rlock.h>			/* record locking defs */
#endif /* RLOCK */

#define	pcDbg(dbgArgs)	debug(0x40, dbgArgs)

/*			    External Functions 				*/


#if	defined(SYS5_4)
extern char *ptsname();			/* get pty slave device */
int    	stdin_fd;			/* slave pty file descriptor */
int	maxFiles;			/* Max number of file descriptors */
char	exec_args[20];			/* shell exec argument */
char	*exec_env[6];			/* pointers to exec environment */
char	*exec_cmd_args[3]; 		/* pointers to exec_cmd arguments */
char	ptydesc_buffer[6];		/* string containing ptydesc */
char	env_buffer[6][120];		/* buffer containing environment var */
#endif

extern void
	    exit();
extern void
	    pckRD();                    /* pack a reliable deliv header */
extern	int
	    rd_flag;			/* reliable delivery? true or false */

extern	int 
	    rcvPacket(),		/* Receive packet */
	    xmtPacket();		/* Send packet */

extern unsigned
	    alarm();

#if	!defined(RS232PCI)
extern	int rdsem_init(),		/* init semaphore */
	    rdsem_access(),		/* access the semaphore */
	    rdsem_relinq();		/* relinquish the semaphore */

extern	char	*rd_sdenter(),		/* attach shared memory */
		rd_sdleave();		/* detach shared memory */

static	int rd_rec_data,		/* If true RDR will receive data */
	    semaphore,			/* reliable delivery semaphore id */
	    stream_size;		/* stream size sent from PC */

static	struct rd_shared_mem
	    *sh_mem;			/* pointer to shared memory segment  */

#ifdef  RD3                             /* XENIX Sys III - Rel. Del. */
extern	char	*rd_sdinit();		/* init shared memory */
static	char *rd_shmid;			/* reliable delivery shared mem id */
#else
extern	int	rd_sdinit();		/* init shared memory */
static	int rd_shmid;			/* reliable delivery shared mem id */
#endif

#endif	    /* Xenix or Eth3bnet - reliable delivery stuff   */


#ifdef	POINT_TO_POINT
extern void
	    protInit(), protPoll(), protStart(); /* copy protection funcs */
char	*mydevname=0;		/* name of tca device to talk on */
char	myserial[SERIALSIZE];		/* serial number of our pc bridge */
char	myser[SERIALSTR];	/* string represetation of myserial */
int	already_connected = 0;	/* turns connect when connected to disconnect */
int	probeCount = 0;	/* number of timeouts since last probe */

#endif	  /* POINT_TO_POINT  */

#ifdef LOCUS
static char 
		txperms[MAXSITE + 1];
#endif     /* LOCUS   */

extern	void 
	    pckframe(),			/* Copies header data to output buf */
	    server(),			/* Process incoming bridge request */
#ifndef  BERKELEY42
	    bld_disktab(),		/* Builds filesys block count table */
#endif    /* !BERKELEY42  */
	    unlink_on_termsig();	/* Unlinks temporary files on SIGTERM */


extern long
	    time(),
	    lseek();



extern	struct	passwd
	    *getpwuid(),		/* Points to user's password entry */
	    *getpwnam();		/* Returns pointer into password file */


extern char
		*crypt(),		/* Unix password encryptor */
		*nAddrFmt();		/* Format a net address for printing */

#ifndef SIGNAL_KNOWN
#ifndef BSD43
#if	defined(CCI)
extern int
#else
extern void	
#endif	/* CCI */
	   (*signal())();
#endif /* !BSD43 */
#endif /* !SIGNAL_KNOWN */

extern	int	print_desc[NPRINT];	/* print file descriptors */
extern	char	*print_name[NPRINT];	/* print file names */

#ifdef	RESTRICT_USER
int	    restrictUser();		/* Checks for restricted login */
#endif	  /* RESTRICT_USER  */

void	    sig_catcher();		/* Signal catching routine */

int	
	    bridge = INITIALIZED,	/* Indicates bridge access is active */
	    swap_how,			/* Byte order for auto-sense flipping */
	    emulator = INITIALIZED,	/* Indicates term emulation is active */
	    versNum,			/* Version number of this server */
	    stopGate = 0,               /* Reentry gate for stopService */

	    netdesc = -1,		/* File descriptor of net device */
	    pipedesc,			/* File descriptor of control pipe */
	    ptydesc,			/* File descriptor of pseudo TTY */
	    length,			/* Length of last configuration frame */
	    accepted = -1,		/* Number bytes accepted on PTY write */
	    files_open,			/* Number of files open on Bridge PC */
	    brg_seqnum = -1,		/* Last Bridge frame sequence number */
	    connseqnum,			/* seq number for fake connect */
	    preconnect = 0,		/* flag for fake connect */
	    ti_seqnum,			/* Last emulator input seq number */
	ti_check,			/* counter to check input characters */
	    termoutpid,			/* Id of terminal output process */
	    descriptors[2];		/* File descriptors associated w/pipe */
int	oldserial = 1;			/* nonzero->use old algorithm */

int	noDisconnect;			/* Don't disconnect on probe timeout */

char	
	    cwd[MAX_CWD],		/* Contains current working directory */
	    ptydevice[MAX_PATH + 1],	/* Contains line in /etc/brgptys file */
	    cntrlpty[MAX_PATH + 1],	/* Name of controlling PTY device */

#if defined(SYS5_4)
	    *slvpty,
#else
	    slvpty[MAX_PATH + 1],	/* Name of slave PTY device */
#endif	/* SYS5_4 */

	    copyright[] =		"PC BRIDGE TO UNIX FILE SERVER\
COPYRIGHT (C) 1984, LOCUS COMPUTING CORPORATION.  ALL RIGHTS RESERVED.  USE OF\
THIS SOFTWARE IS GRANTED ONLU UNDER LICENSE FROM LOCUS COMPUTING CORPORATION.\
ANY UNAUTHORIZED USE IS STRICTLY PROHIBITED.";

struct  ni2 ndata;                      /* Ethernet device header structure */
					/* (dummy struct when not ethernet) */
#ifdef ETHNETPCI
struct  ni2 ntmp;                       /* Ethernet device header structure */

#if defined(UDP42) || defined(UDP41C)
static struct ni2 conSvrHeader;
#endif /* UDP42 || UDP41C  */

#ifdef	ETH3BNET
NI_PORT
	    lp;				/* Ethernet ioctl control block */
#endif	  /* ETH3BNET  */
#ifdef	ETHLOCUS
struct lpstatus 
	    lp;				/* Ethernet ioctl control block */
#endif	  /* ETHLOCUS  */
#endif	  /* ETHNETPCI  */

#ifdef RS232PCI
struct	termio
	    ttysave,			/* Saves TTY modes for later */
	    ttymodes;			/* TTY modes structure */

#ifdef RS232_7BIT
int
	    using_7_bits = 0;		/* Serial line only supports 7 bits */
#endif /* RS232_7BIT */
#endif   /* RS232PCI  */

static struct	input				/* Input buffer for RS-232 */
#if defined(UDP42) || defined(UDP41C)
	    cibuf,			/* Input buffer for Consvr comm */
#endif /* UDP42  || UDP41C */
	    in;

static struct	output
#if defined(UDP42) || defined(UDP41C)
	    conbuf,			/* Output buffer for Consvr comm */
#endif /* UDP42 || UDP41C  */
	    out;			/* Output buffer for RS-232 */

extern	int
	    errno;			/* Error return from system calls */

#if defined(UDP42) || defined(UDP41C)
static int login_pid;
#endif /* UDP42 || UDP41C  */

char
	*logStr;			/* Log level string (shows in ps) */


#define	NULLstat	(struct stat *)0

/* problem in excelan code not returning the correct family class
   after a receive. The START parameter is used in the address compare
   after a receive takes place in the main loop.
*/
#ifdef XLN_BUG
#define START 4 
#else
#define START 0
#endif /* XLN_BUG */

/*
 *  main()  arguments:  (any order)
  -ny   ethernet network descriptor y.        : only when ETHNETPCI
  -pz   3B2 ni driver logical port number z.  : only when for 3b2
  -Dn   debug level n          default 0
#ifdef TCA
  -tdev talk on file "dev"
#endif
#ifdef RS232PCI
  -7	use only 7 bits in file protocol
#endif
 */

int  savePid;

struct passwd
	*pptr;			/* Pointer to entry in password file */

#ifdef SHADOW_PASSWD
struct spwd *spptr = NULL;	/* pointer to shadow password entry */

int sVr3_1 = 0;	/* true if on SVR3.1 or later, i.e. /etc/shadow exists */
#endif

char *myname;

main(argc, argv)
int argc;
char *argv[];
{
    char *password;	/* password from /etc/passwd or /etc/shadow */
    char *user_shell;	/* user's login shell */
    int pwlen_pw;	/* length of password from /etc/passwd or /etc/shadow */
    int pwlen_in;	/* length of password transmitted */
    struct stat statbuf;	/* dummy buffer for stat system call */
    register int 
	len,			/* Temporary length of string */
	status,			/* Return value from system call */
	i;			/* Loop counter for loading source addresses */
	int length;
	int savePid;
	int code;		/* Return value after attach to shared memory */

#ifdef	RS232PCI
    char
	ptybuf[10];		/* Character buffer for terminal emulation */
	extern char *ttyname();
#endif

    int
	portnum;
#ifndef	RS232PCI
    struct emhead
        *rcvhead = (struct emhead *) in.text;
    struct emhead
	*sndhead = (struct emhead *) out.text;
#endif
    struct ackcontrol
	control;                /* RD: communication structure onto PIPE */
    int send_ack,               /* RD: need to send ack local protocol   */
	accept,                 /* RD: accpet data f;ag for xmt to pty   */
	old_state;              /* RD: old state protocol was in         */
    unsigned short
        sequence;               /* RD:temp variable                      */

    char
	c;			/* Character for writing onto PIPE */

    FILE 
	*fptr;			/* File pointer into PTY configuration table */

    char
	fileName[MAX_PATH],	/* Logfile temporary name */
	*ptr,			/* Pointer to character array */
	*ptyGetty,		/* Pointer to getty command from pciptys */
	*txtptr,		/* Pointer into input text buffer */
	*cryptPass,		/* Return from crypt(3) */
	fname[120];             /* a file name */

char
	errArg[8],		/* Error descriptor */
	pipeArg[8],		/* Ack pipe descriptor */
	ptyArg[8],		/* Pty descriptor */
	debugArg[8],		/* Debug level */
	rdArg[15],		/* reliable delivery */
	ssArg[15],		/* stream size (RD only) */
	swapArg[8],		/* Byte swapping code */
	descArg[8],		/* Network (eth or 232) descriptor */
	naddrArg[32];		/* Network address (eth) */
int
	argN;			/* Current argument number */
char
	*arg;			/* Current argument */

#ifdef	RIDGE
int off = 0;
#endif	/* RIDGE */

#ifdef XENIX
struct stat sig_buf;
#endif /* XENIX */

#if defined(UDP42) || defined(UDP41C)
struct sockaddr_in vPort;
#endif /* UDP42 || UDP41C  */

#ifdef SHADOW_PASSWD
	sVr3_1 = !stat("/etc/shadow", &statbuf);    /* determine system level */
#endif
	myname = argv[0];
	if (*myname == '\0')
		myname = "unknown";

	/* Extract version number of this server from its name */
	versNum = getVers(argv[0]);

	/* Decode arguments */
	for (argN = 1; argN < argc; argN++) {
		arg = argv[argN];

		if (*arg != '-')
			continue;

		switch (arg[1]) {
		case 'D':		/* Log level */
			logStr = &arg[2];
			dbgSet(strtol(logStr, (char **)NULL, 16));
			break;

		case 'n':		/* Network descriptor */
			netdesc = atoi(&arg[2]);
			log("xx netdesc on %d\n",netdesc);
			break;

#ifdef TCA
		case 't':
			mydevname = arg+2;
			break;
#endif   /* TCA  */
		case '-':
			break;	/* dummy placeholder */
		case 'x':
			log("probe timeouts are disabled\n");
			noDisconnect = 1;
			break;
		case 'P':
			pipedesc = atoi(&arg[2]);
			log("pipedesc is %d\n",pipedesc);
			if (read(pipedesc,&in,sizeof in) != sizeof in) {
				fatal(lmf_format_string((char *) NULL, 0, 
					lmf_get_message("P_CONNECT1",
					"error reading conninfo pipe:%1\n"), 
					"%d", errno));
			}
			close(pipedesc);
			log("about to inputswap\n");
			swap_how = input_swap(&in, in.hdr.pattern);
			brg_seqnum = in.hdr.seq;
			preconnect = 1;
			break;


		case 'p':		/* 3B2 ni driver logical port number */
			portnum = atoi(&arg[2]);
			break;
#if defined(RS232PCI) && defined(RS232_7BIT)
		case '7':
			using_7_bits++;
			break;
#endif /* RS232PCI && RS232_7BIT */
		}
	}

#ifdef TCA

	/* Close all open files */
	for (i = 0; i < uMaxDescriptors(); i++)
		if (i != netdesc)
			close(i);

#endif   /* TCA  */

	/* Initialize print file table */
	for (i = 0; i < NPRINT; i++) {
		print_desc[i] = -1;
		print_name[i] = NULL;
	}

	if (dbgCheck(~0)) {
		logOpen(DOSSVR_LOG, getpid());
		ulog("Debug channels = 0x%x\n", dbgEnable);
	}

    signal(SIGALRM, sig_catcher);	/* Catch the alarm signal. Someone is */
					/* sending this signal which kills */
					/* the dossvr. [ WU 6/6/88 ] */
    signal(SIGTERM, sig_catcher);	/* Catch signal from pcidaemon */
#if	defined(XENIX) && defined(RS232PCI)
    signal(SIGINT, SIG_IGN);		/* Ignore signal from tty (for now) */
#else
    signal(SIGINT, sig_catcher);	/* Catch signal from tty (break) */
#endif
    signal(SIGHUP, sig_catcher);	/* Catch signal from tty (disconnect) */
    signal(SIG_DBG1, sig_catcher);      /* Catch signal to toggle logs */
    signal(SIG_DBG2, sig_catcher);      /* Same */
    signal(SIG_CHILD, sig_catcher);	/* Catch signal if pcitermout dies */

#ifdef	SYS5
    /* Set up my own process group so later when I do a kill(0,15) it won't
       kill other DOSSVR's  */ 
    setpgrp();
#endif	  /* SYS5  */

	/* initialize NLS */
	nls_init();


	/* initialize translation tables */
	table_init();

#ifdef TCA
	if (netdesc == -1) {
		log("netdesc -1\n");
		if(!mydevname)
			fatal(lmf_get_message("P_CONNECT2",
				"TCA device not specified\n"));
		if((netdesc = open(mydevname, 2)) < 0)
			fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("P_CONNECT3",
				"Can't open \"%1\" errno: %2\n"), 
				"%s%d" , mydevname, errno));
	}
	log("about to use netdesc %d\n",netdesc);
	netUse(netdesc);
#endif   /* TCA  */

/* 
 * Configure ethernet device (port/descriptor inherited from daemon).
 */

#ifdef ETHNETPCI

#if defined(UDP42) || defined(UDP41C)


	/* get our own address into vPort.sin_addr.s_addr */
	hostaddr(&vPort);
	vPort.sin_family = AF_INET;
	vPort.sin_port = htons(PCI_CONSVR_PORT);

	/* Save ethernet source address, type, and port for output frames */
	nAddrCpy(conSvrHeader.dst, &vPort);

#else /* !UDP42 || !UDP41C  */

    while ((ioctl(netdesc, NIGETA, &lp)) < 0) {
	if (errno == EINTR)
	    continue;
	fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_CONNECT4","Error %1 on NIGETA; Bye!\n"),
		"%d", errno));
    }

/* 
 * Set-up desired configuration in status structure for ioctl.
 */
    lp.rcvb_sz = MAX_FRAME;			/* Maximum packet size */
    lp.rcvq_sz = 2;                     /* Number of buffers */

#ifdef	ETHLOCUS
	lp.eptype[0] = PCIAID1;
	lp.eptype[1] = PCIAID2;
	lp.lpoff = PCIPORTOFF;
#endif	  /* ETHLOCUS  */

#ifdef ETH3BNET
/*
 * The source address is formed using a daemon assigned port number, a 
 * the PCI application identifier, and a component of the real address.
 */
    lp.srcaddr[0] = ((2 * (portnum + 1)) | 0x01);
    lp.srcaddr[1] = 0x76;		/* Upper byte of PCI Application ID */
    lp.srcaddr[2] = 0x77;		/* Lower byte of PCI Application ID */
    lp.rspq_sz = 2;			/* Number of output buffers */
    lp.type = ETHERTYPE;		/* Application must set this */
    lp.protocol = 0x7677;		/* PCI application identifier */
#endif   /* ETH3BNET  */

    if (ioctl(netdesc, NISETA, &lp) < 0)
	fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_CONNECT5","Error %1 on NISETA; Bye!\n"),
		"%d" , errno));

#ifdef	ETHLOCUS

/* Save ethernet source address, type, and port for bridge output frames. */
    for (i = 0; i < SRC; i++)
	ndata.src[i] = lp.netid[i];

/* Specify ethernet domain space */
    ndata.type[0] = PCIAID1;
    ndata.type[1] = PCIAID2;
    ndata.head[5] = portnum;

#endif	  /* ETHLOCUS  */
#ifdef	ETH3BNET

/* Save ethernet source address, type, and port for bridge output frames */
    for (i = 0; i < SRC; i++)
	ndata.src[i] = lp.srcaddr[i];

/* Specify ethernet domain space */
    ndata.type[0] = 0x76;
    ndata.type[1] = 0x77;

#endif	  /* ETH3BNET  */
#endif	/* UDP42 || UDP41C  */

    netUse(netdesc);

#endif   /* ETHNETPCI  */

#ifdef RS232PCI

    if (netdesc == -1) netdesc = STDIN;
    if (get_tty(netdesc, &ttysave) < 0)
	fatal(lmf_format_string((char *) NULL, 0,
		lmf_get_message("P_CONNECT6","Error %1 on get_tty\n"),
		"%d", errno));

    ttymodes = ttysave;

/* Set up TTY driver for eight bit "raw" mode */
    rawify_tty(&ttymodes);

    if (set_tty(netdesc, &ttymodes) < 0)
	fatal(lmf_format_string((char *) NULL, 0,
		lmf_get_message("P_CONNECT7","Error %1 on STDIN set_tty\n"),
		"%d", errno));

#ifdef TCA
	mydevname = ttyname(netdesc);
#endif   /* TCA  */

    /* Give network (tty) descriptor to network interface */
    netUse(netdesc);

	/* Get current working directory from password file */
	    if ((pptr = getpwuid(getuid())) == (struct passwd *)NULL)
		fatal(lmf_get_message("P_CONNECT8",
			"Error getting current working directory\n"));

	/* Save user's login shell */
	    user_shell = pptr->pw_shell;


	    /* Start bridge in home directory */
	    if (chdir(pptr->pw_dir) < 0) {
		dosSvrAlive(LOGIN_FAILED);
		stopService(1,0);
	    }
	    if (!preconnect) dosSvrAlive(SUCCESS);

    strcpy(cwd, pptr->pw_dir);

    bridge = RUNNING;

#ifdef	XENIX
if  (stat("/usr/pci/config.sig", &sig_buf) != 0) 
    signal(SIGINT, sig_catcher);	/* now catch breaks from tty */
#endif

#endif   /* RS232PCI  */


    /* Initialize virtual file descriptor cache */
    vfInit();

#ifdef RLOCK    /* Record locking from Starlan */

	/* Attach to the shared table segments and the server lock set */
	log("p_connect.c: Attaching to share table segment.\n");
	if (rlockAttach()) {
		log("p_connect.c: can't initialize, %s\n",
				rlockEString(rlockErr));
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("P_CONNECT9",
			"Can't initialize record lock data, %1 \n"),
			"%s", rlockEString(rlockErr)));
	}

#endif /* RLOCK */

#ifdef	POINT_TO_POINT
	/* copy protection initialization */
	log("protInit\n");
	protInit();
	if (preconnect) {
		log("preconnect,do_connect\n");
		do_connect();
	}
#endif	  /* POINT_TO_POINT  */


/*
 * Service configuration requests ,bridge packets,
 * or terminal emulator packets.
 */ 
     for (;;)
     {
	int j;
getrequest:
	swap_how = rcvPacket(&in);


#ifdef ETHNETPCI

    /* Store destination address of PC */


	if (bridge == INITIALIZED) {
	    for (i = 0; i < DST; i++)
		ndata.dst[i] = in.net.src[i];
	}
	else {

	/* Only the connected PC may use this port! */
	    for (i = START; i < START /*DST-1*/; i++) { /* HOOK */
		if (ndata.dst[i] != in.net.src[i]) {
		    log("Bad source %s\n", nAddrFmt(in.net.src));
		    pckframe(&out, CONF, (int)in.hdr.seq, in.hdr.req, NEW,
			INVALID_FUNCTION, NO_DES, NO_CNT, NO_CNT, NO_MODE,
			NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
		    goto getrequest;
		}
	    }
	}

#endif   /* ETHNETPCI  */

/*
 * Retransmit previous response upon receipt of previous sequence number.
 */
	if (in.pre.reset) {
	    brg_seqnum = 0;
	    ti_seqnum  = 0;
	ti_check = 0;
	}
#ifdef	POINT_TO_POINT
	else if ((in.pre.select == CONF || in.pre.select == DAEMON)
	     &&  in.hdr.seq == brg_seqnum)
#else	  /* !POINT_TO_POINT  */
	else if (in.pre.select == CONF && in.hdr.seq == brg_seqnum)
#endif	  /* POINT_TO_POINT  */
	{
	    log("reXmt due to seqnum\n");
	    reXmt(&out, length);
	    goto getrequest;
	}

#ifdef	POINT_TO_POINT
	/* any packet is as good as a probe in this case */
	probeCount = 0;
#endif	  /* POINT_TO_POINT  */

	switch (in.pre.select) {
#ifdef	POINT_TO_POINT
	    /* Respond to some messages normally handled by other servers */
	    case DAEMON:
		if (in.hdr.req == CONNECT) {
		    /* Don't think this is/was right.  Did stopService */
		    /* Guess it is needed for tca but not 232          */
#ifdef TCA
		    do_connect();
#endif    /* TCA  */
		    break;
		}
		else if (in.hdr.req == SEND_MAP)
		    sendMap(&out);
		else if (in.hdr.req == DISCONNECT)
		    stopService(2,0);
		else if (in.hdr.req == PROBE) {
			conProbe();
			break;	/* no response for this one */
		}
		else
		    break;

		length = xmtPacket(&out, &ndata, swap_how);
		break;
#endif	  /* POINT_TO_POINT  */

    /*
     * Request to establish PC-DOS Bridge session (only in ethernet mode.)
     */
	    case CONF:

#if	defined(ETHNETPCI) || defined(TCA)

		if (in.hdr.req == EST_BRIDGE) {
		    brg_seqnum = in.hdr.seq;
		    if (bridge == RUNNING) {
			log("Dup bridge req\n");
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				DUPLICATE_CONNECTION, NO_DES, NO_CNT, NO_CNT,
				NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE,
				NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }

		    log("Establish bridge\n");

		/* Execute login sequence: find entry from passwd file */
		    if (
#ifndef	ALLOW_ROOT_LOGIN
				(strcmp(in.text, "root")) == 0 ||
#endif	/* ALLOW_ROOT_LOGIN */
				!(pptr = getpwnam(in.text))
#ifdef SHADOW_PASSWD
				|| (sVr3_1 && !(spptr = getspnam(in.text)))
#endif
				) {
			cryptPass = crypt(in.text + in.hdr.b_cnt, "//");
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }

		/* Match password */
		    password =
#ifdef SHADOW_PASSWD
				(sVr3_1) ? spptr->sp_pwdp :
#endif
							    pptr->pw_passwd;
		    pwlen_pw = strlen(password);
		    pwlen_in = strlen(in.text + in.hdr.b_cnt);
		    if ((pwlen_pw && !pwlen_in) || (pwlen_in && !pwlen_pw))
			goto bad_passwd;
		    if (!pwlen_in)
			goto good_passwd;

		    cryptPass = crypt(in.text + in.hdr.b_cnt, password);
	   	    if (!strcmp(cryptPass, password))
		    	goto good_passwd;
bad_passwd:
		pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, LOGIN_FAILED,
			NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF,
			NO_ATTR, NO_DATE, NULLstat);
		length = xmtPacket(&out, &ndata, swap_how);
		break;
good_passwd:
#ifdef	RESTRICT_USER
		/* Check RESTRICTFILE for this username, and disallow login
		   if name is found */
		if (restrictUser(in.text))
			goto bad_passwd;
#endif	  /* RESTRICT_USER  */
#ifdef LOCUS	/* Check site permissions and fail if user is not allowed */
		/* to login to this site.  Should have better error codes */
		/* that explain the cause of failure.			  */

		get_valid_sites(txperms);
		if (txperms[site(0L)] == 0)  {	/* must have 0L for xenix */
		    pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, 
		    	LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE, 
		    	NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
		    length = xmtPacket(&out, &ndata, swap_how);
		    break;
		}
#endif     /* LOCUS   */
		/* Start bridge in home directory */
		    if (chdir(pptr->pw_dir) < 0) 
			strcpy(pptr->pw_dir, "/tmp");
		    if (chdir(pptr->pw_dir) < 0)  {
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }

#if	!defined(LOCUS) && !defined(BERKELEY42) && !defined(SYS5_4)
		/* Build table of filesystems block count while still root */
		    bld_disktab();
#endif    /* !BERKELEY42  */

		/* Get current working directory, uid, gid from password file */
		    strcpy(cwd, pptr->pw_dir);

#ifdef SETUTMP
	/* need to set utmp entry so pci user looks like a user */
		    set_utmp(pptr);
#endif	/* SETUTMP */

#ifdef LOGFILE_RWROOT
		    logChown(pptr->pw_uid,pptr->pw_gid);	/* KLO0168 */
#endif

#if defined(BERK42FILE) && !defined(RS232PCI)
			initgroups(pptr->pw_name,pptr->pw_gid);		/* SPR #3520 */
#endif	/* BERK42FILE && !RS232PCI */
		    setgid(pptr->pw_gid);
		    setuid(pptr->pw_uid);

		/* Bridge session established */
		    bridge = RUNNING;
		    pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, SUCCESS,
			NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF,
			NO_ATTR, NO_DATE, NULLstat);
		    length = xmtPacket(&out, &ndata, swap_how);
		    break;
		}

#endif	  /* ETHNETPCI || TCA  */

#ifdef RS232PCI
		if (in.hdr.req == PROBE) {
		    brg_seqnum = in.hdr.seq;
		    log("Bridge: probe\n");
		    pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, SUCCESS,
			NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF,
			NO_ATTR, NO_DATE, NULLstat);
		    length = xmtPacket(&out, &ndata, swap_how);
		    break;	
		}

#endif   /* RS232PCI  */

    /* Request to establish terminal emulator session */
    /* If reliable delivery set rd_flag to true	      */

		else if (in.hdr.req == EST_SHELL) {
#if	!defined(RS232PCI)
		    if (in.hdr.t_cnt > 0) {
			log("Connector using reliable delivery.\n");
			stream_size = in.hdr.t_cnt;
			log ("Stream size: %d\n", stream_size);
			rd_flag = 1;
			old_state = -1; /* undefined state */
		if (emulator != RUNNING) { /* init sem and shm first time */
                    if ((semaphore=rdsem_init()) == -1) {
			  log("Can't get rd semaphore.\n");
			  pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			  length = xmtPacket(&out, &ndata, swap_how);
			  break;
		    }
		    /* initialize and attach to the shared memory */
		    rd_rec_data = FALSE;	/* don't receive yet */
		    if ((rd_shmid = rd_sdinit()) < 0) {
			log("Can't get shared memory: errno %d\n", errno);
			rdsem_unlnk(semaphore);		/* remove semaphore */
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }

		    if ((sh_mem = (struct rd_shared_mem *) rd_sdenter(rd_shmid))
				== 0) {

			log("Can't enter shared memory: errno %d\n", errno);
			rdsem_unlnk(semaphore);	/* remove semaphore */
			rd_shmdel(rd_shmid);	/* remove shared memory */
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }
		} /* emulator != running */
		    sh_mem->FrameExpected = 0;
		    sh_mem->kick_ack = 0;

		    /*
		     * We are passing sh_mem which is a ptr to the
		     * shared memory that we need to dettach.
		     */
		    }  /* if hdr.t_cnt > 0 */
		else
#endif
		    rd_flag = 0;


		    brg_seqnum = in.hdr.seq;

#if	defined(ETHNETPCI) || defined(TCA)
		    if (emulator == RUNNING) {
			log("Dup shell req\n");
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, 
			DUPLICATE_CONNECTION, NO_DES, NO_CNT, NO_CNT, NO_MODE, 
			NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);

#if	!defined(RS232PCI)
			if (rd_flag) {	/* we need init termout */
				if (kill(termoutpid, SIGUSR2) < 0)
					log("Couldn't signal termout.\n");
		    		sh_mem->FrameExpected = 0; /* init shared mem */
		    		sh_mem->kick_ack = 0;
 				control.code = RD_INITTERM;  /* command code */
 				control.num = 1;    /* just to make sure */
 				if (write(pipedesc, &control, 
 					sizeof(struct ackcontrol)) < 0)
 					log("PIPE (INITTERM) write error: %d\n",
 					     errno);
 				log("Wrote INITTERM to pipe.\n");
			}
#endif   /* !RS232PCI  */

			break;
		    }
#endif	  /* ETHNETPCI || TCA  */

		    log("Start Shell\n");

#ifdef RS232PCI
		    if (emulator == RUNNING) {
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				SUCCESS, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			goto runemulator;
		    }
#endif   /* RS232PCI  */

#ifdef UNISOFT
    		    if(bridge != RUNNING) {
			/* This dossvr was started for an Emulator session;
			   on the UniPlus system, we can't allow this because
			   problems with the remlogin program.  So, we dis-
			   allow the emulator session and stopService for this
			   dossvr.  
			*/
			log("No bridge session; Can't start EM.\n");
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			stopService(3,0);
			break;
		    }
#endif   /* UNISOFT  */
		/* Open PIPE to output process */
		    if ((pipe(descriptors)) < 0) {
			log("Can't pipe(): %d\n", errno);
#if	!defined(RS232PCI)
			rdsem_unlnk(semaphore);	/* remove semaphore */
			rd_sdleave(sh_mem);	/* detach shared memory */
			rd_shmdel(rd_shmid);	/* remove shared memory */
#endif
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }

		/* Search for an available PTY */
#ifdef	AIX_RT 
		    if ((fptr = popen(PENABLE, "r")) == (FILE *)NULL) {
#else	/* ! AIX_RT */
		    if ((fptr = fopen(PCIPTYS, "r")) == (FILE *)NULL) {
#endif	/* AIX_RT */
			close(descriptors[0]);
			close(descriptors[1]); /* [1/26/88 PD] */
			log("Can't open %s: %d\n", PCIPTYS, errno);
#if	!defined(RS232PCI)
			rdsem_unlnk(semaphore);	/* remove semaphore */
			rd_sdleave(sh_mem);	/* detach shared memory */
			rd_shmdel(rd_shmid);	/* remove shared memory */
#endif
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }

		/* Search PTY table for available PTY device */
		    ptydesc = -1;
		    while (fgets(ptydevice, MAX_NAME, fptr)) {
#ifdef	AIX_RT
			if (strncmp(ptydevice, "pts", 3))
				continue;
			/* Save the slave pty name */
			strcpy(slvpty, ptydevice);
			/* change 'pts' to 'ptc' */
			ptydevice[2] = 'c';
			len = strlen(ptydevice);
			ptydevice[len-1] = '\0';
			strcpy(cntrlpty, "/dev/");
			strcat(cntrlpty, ptydevice);
			if ((ptydesc = open(cntrlpty, O_RDWR)) >= 0) {
			    /* Sleep for 5 sec for getty to get ready */
			    sleep(5);
			    break;
			}
#else	/* +AIX_RT- */
			len = strlen(ptydevice);
			ptydevice[len-1] = '\0';	/* zap newline */
			if (ptr = strchr(ptydevice, ':')) {
				*ptr++ = '\0';
				if (ptyGetty = strchr(ptr, ':')) {
			    	*ptyGetty++ = '\0';
			    	while (isspace(*ptyGetty))
					ptyGetty++;
			    	if (!(*ptyGetty))
					ptyGetty = NULL;
#if RS232PCI
			    	if (ptyGetty)	/* no consvr to run getty */
					continue;	/*     so skip this pty   */
#endif
			}
		     }
		     strcpy(cntrlpty, "/dev/");
		     strcat(cntrlpty, ptydevice);
		     if ((ptydesc = open(cntrlpty, O_RDWR)) >= 0) {
#if defined(SYS5_4)

			    exec_cmd_args[0] = "/usr/lib/pt_chmod";
			    sprintf(ptydesc_buffer, "%d", ptydesc);
			    exec_cmd_args[1] = ptydesc_buffer;
			    exec_cmd_args[2] = (char *)0;

			    if (exec_cmd(exec_cmd_args[0], exec_cmd_args, 
								ptydesc)
						&& unlockpt(ptydesc) != -1
						&& (slvpty = ptsname(ptydesc)))
				break;

			    close(ptydesc);
			    ptydesc = -1;

#else	/* !SYS5_4 */
			    strcpy(slvpty, ptr);	/* slave device name */
			    break;
#endif	/* SYS5_4 */
			}
#endif 	/* AIX_RT */
		    }
#ifdef	AIX_RT
		    pclose(fptr);
#else	/* AIX_RT */
		    fclose(fptr);
#endif	/* AIX_RT */

		/* Was PTY available? */
		    if (ptydesc < 0) {
			log("No PTYs available\n");
#if	!defined(RS232PCI)
			rdsem_unlnk(semaphore);	/* remove semaphore */
			rd_sdleave(sh_mem);	/* detach shared memory */
			rd_shmdel(rd_shmid);	/* remove shared memory */
#endif
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);

		    /* Clean-up from login attempt */
			close(descriptors[0]);
			close(descriptors[1]);
			break;
		    }

#ifdef	RIDGE
		    /* Turn off packet mode - should be off by default! */
		    ioctl(ptydesc, TIOCPKT, &off);
#endif /* RIDGE */

#if defined(SYS5_4) && defined(RS232PCI)

/*	This section resolves the AT&T and SEQUENT streams issue by 
**	exec'ing a shell instead of running getty on the slave PTY 
**	for concurrent RS232 em.
*/
	if (fork() == 0)	{    /* in child do the following */

#ifdef NOFILES
		maxFiles = NOFILES;
#else
		maxFiles = (int) ulimit(4,0L);
			if (maxFiles < 0)
				maxFiles = 20;
#endif

		/* close all file descriptors except the logfile */
		for (i = 0; i < maxFiles; i++) 
			if (!logFile || i != fileno(logFile))
				close(i);

		setpgrp();
		
#if	defined(SEQUENT)
		/* restore slvpty to its complete pathname */
		slvpty = slvptyPtr;
#endif

		if ((stdin_fd = open(slvpty, O_RDONLY)) != -1) {
			log("stdin fh = %d\n", stdin_fd);

#if 	defined(SYS5_4)
			ioctl(stdin_fd, I_PUSH, "ptem");
			ioctl(stdin_fd, I_PUSH, "ldterm");
			ioctl(stdin_fd, I_PUSH, "ttcompat");

#elif	defined(SEQUENT)
			ioctl(stdin_fd,TCGETA,&b);
			b.c_iflag &= ~(INLCR | BRKINT);
			b.c_iflag |= IXON;
			b.c_iflag |= ICRNL;
			b.c_oflag |= OPOST;
			b.c_oflag |= ONLCR;
			b.c_lflag |= ECHO;
			b.c_oflag &= ~(OLCUC | OCRNL | ONOCR | ONLRET);
			b.c_lflag &= ~(ICANON | ISIG);
			b.c_cc[VMIN] 	 = '\01';
			b.c_cc[VTIME] 	 = '\0';
			ioctl(stdin_fd,TCSETAW,&b);
#endif	/* SYS5_4 , SEQUENT, etc. */

			if ((i=open(slvpty, O_WRONLY)) != -1) {

				log("stdout fh = %d\n", i);

				dup2(1, 2);
				/* reset signals set to SIG_IGN to SIG_DFL */

				/* exec user's shell */
				strcpy(exec_args, "-");
				strcat(exec_args, strrchr(user_shell,'/')+1);

				strcpy(env_buffer[0],"LOGNAME=");
				strcat(env_buffer[0], (char *) getenv("LOGNAME"));
				exec_env[0]= env_buffer[0];
 
				strcpy(env_buffer[1],"PATH=");
				strcat(env_buffer[1], (char *) getenv("PATH"));
				exec_env[1]= env_buffer[1];
 
				strcpy(env_buffer[2],"TZ=");
				strcat(env_buffer[2], (char *) getenv("TZ"));
				exec_env[2]= env_buffer[2];
 
				strcpy(env_buffer[3],"TERM=");
				strcat(env_buffer[3], (char *) getenv("TERM"));
				exec_env[3]= env_buffer[3];
 
				strcpy(env_buffer[4],"HOME=");
				strcat(env_buffer[4], (char *) getenv("HOME"));
				exec_env[4]= env_buffer[4];
 
				exec_env[5]= (char *) 0;
				execle(user_shell, exec_args, (char *) 0, exec_env);
			} /* open */
		} /* open */


		pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
			LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
			NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
		length = xmtPacket(&out, &ndata, swap_how);
		fatal("Can't exec a shell\n");

	}   /* end child */


#elif !defined(DONT_START_GETTY) && defined(UDP42) && !defined(M_386)
		    if (ptyGetty) {

			/* Ask consvr to startup login process */
			strcpy(conbuf.text, slvpty);
			i = strlen(slvpty) + 1;
#if defined(SYS5_4)
			i += sprintf(&conbuf.text[i], ptyGetty, slvpty ) + 1; 
#else
			strcpy(&conbuf.text[i], ptyGetty);
			i += strlen(ptyGetty) + 1;
#endif
			pckframe(&conbuf, DAEMON, brg_seqnum, S_LOGIN, NEW, 0,
				getpid(), 2, i, 0, 0, 0, 0, 0, NULLstat);

			/* Send request to consvr */
	        	xmtPacket(&conbuf, &conSvrHeader, NOFLIP);

			/* Now wait for response from getty-to-be */
			do {
			    rcvPacket(&cibuf);
			    /* Is this the packet I've been waiting for? */
			    if (nAddrEq(cibuf.net.src, conSvrHeader.dst))
				break;
			} while(cibuf.hdr.req != S_LOGIN);

			if (cibuf.hdr.req != S_LOGIN) {
			    /* Something is amis here, let's just forget */
			    /* the whole thing!				 */
			    /* Clean-up from login attempt		 */
#if	!defined(RS232PCI)
			    rdsem_unlnk(semaphore);	/* remove semaphore */
			    rd_sdleave(sh_mem);		/* detach shared mem */
			    rd_shmdel(rd_shmid);	/* remove shared mem */
#endif
			    close(descriptors[0]);
			    close(descriptors[1]);
			    /* we'll just drop the packet and let the pc
			    retry with it's current request */
			    break;
			}

			/* This is our response from the getty-to-be */
			if (cibuf.hdr.res != SUCCESS) {
			    log("Can't start login\n");
#if	!defined(RS232PCI)
			    rdsem_unlnk(semaphore);	/* remove semaphore */
			    rd_sdleave(sh_mem);		/* detach shared mem */
			    rd_shmdel(rd_shmid);	/* remove shared mem */
#endif
			    pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			    length = xmtPacket(&out, &ndata, swap_how);

			    /* Clean-up from login attempt */
			    close(descriptors[0]);
			    close(descriptors[1]);
			    break;
			}
			login_pid = cibuf.hdr.fdsc;
		    }
#endif	/* !DONT_START_GETTY && UDP42 && !M_386 */

		/* Execute terminal emulation output process */
		    if ((termoutpid = fork()) < 0) {
			log("Can't fork: %d\n", errno);

#if	!defined(RS232PCI)
			rdsem_unlnk(semaphore);	/* remove semaphore */
			rd_sdleave(sh_mem);	/* detach shared memory */
			rd_shmdel(rd_shmid);	/* remove shared memory */
#endif
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			close(descriptors[0]);
			close(descriptors[1]);
			break;
		    }

		    else if (termoutpid != 0) {
		    /* Parent */
			close(descriptors[0]);
			pipedesc = descriptors[1];
		    }
		    else {
		    /* Child */
			close(descriptors[1]);
			pipedesc = descriptors[0];

			/* Pass destination network address */
#ifdef	ETHNETPCI
			sprintf(naddrArg, "-a%s", nAddrFmt(in.net.src));
			sprintf(descArg, "-n%d", netdesc);
#endif   /* ETHNETPCI  */
#if defined(RS232PCI) || defined(POINT_TO_POINT)
			sprintf(descArg, "-t%d", netdesc);
#endif

			if (logFile != NULL)
				sprintf(errArg, "-e%d", fileno(logFile));
			sprintf(pipeArg, "-p%d", pipedesc);
			sprintf(ptyArg, "-y%d", ptydesc);
			sprintf(debugArg, "-D%04x", dbgEnable);
			sprintf(swapArg, "-s%d", swap_how);

		    /* Execute terminal output process */

			if (versNum == 0)
				sprintf(fname, "%s/%s", PCIDIR, PCIOUT);
			else
				sprintf(fname, "%s/%s%d", PCIDIR, PCIOUT,
					versNum);
#ifdef	ETHNETPCI
			if (!rd_flag)
			   execl(fname, strrchr(fname, '/') + 1, errArg,
				debugArg, descArg, naddrArg, pipeArg,
				ptyArg, swapArg, (char *)NULL);
			else {
			   sprintf(rdArg, "-R%d", semaphore);
			   sprintf(ssArg, "-S%d", stream_size);
			   stream_size = 0;	/* reset for later code */

			   execl(fname, strrchr(fname, '/') + 1, errArg,
				debugArg, descArg, naddrArg, pipeArg,
				ptyArg, swapArg, rdArg, ssArg, (char *)NULL);
			}

#endif	  /* ETHNETPCI  */
#if defined(RS232PCI) || defined(POINT_TO_POINT)
			execl(fname, strrchr(fname, '/') + 1, errArg,
				debugArg, descArg, pipeArg,
				ptyArg, swapArg, (char *)NULL);
#endif
#if	!defined(RS232PCI)
			rdsem_unlnk(semaphore);	/* remove semaphore */
			rd_sdleave(sh_mem);	/* detach shared memory */
			rd_shmdel(rd_shmid);	/* remove shared memory */
#endif
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);

		    /* Clean-up from login attempt */
			close(ptydesc);
			close(descriptors[0]);
			fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("P_CONNECT11",
				"Can't exec %1: %2\n"), 
				"%s%d", fname, errno));
		    }


		/* Send response to original EST_SHELL request */
		    emulator = RUNNING;
		    ti_seqnum = 0;

		    pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, SUCCESS,
			NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF,
			NO_ATTR, NO_DATE, NULLstat);
		    length = xmtPacket(&out, &ndata, swap_how);
#if	defined(ETHNETPCI) || defined(TCA)
		    break;
#endif	  /* ETHNETPCI || TCA  */
#ifdef RS232PCI

		/* Read raw stream and write unstuffed chars to pty */
runemulator:

		    for (;;) {

			if ((status = read(netdesc, &ptybuf[0], 1)) < 0) {
			    log("TTY Read err: %d\n", errno);
			    continue;
			}
			debug(0, ("Input %c\n", ptybuf[0]));

		    /* Scan stream for termination characters */
			if (ptybuf[0] == SYNC) {
			    if ((status = read(netdesc, &ptybuf[1], 1)) < 0) {
				log("TTY Read err: %d\n", errno);
				continue;
			    }

			/* Break out of Emulator mode inot bridge mode */
			    if (ptybuf[1] != SYNC)
				break;
			    else if ((status = write(ptydesc, &ptybuf[0], 1)) < 0) {
				log("PTY Write err: %d\n", errno);
				continue;
			    }
			}
			else if (ptybuf[0] == SYNC_2) {
			    if ((status = read(netdesc, &ptybuf[1], 1)) < 0) {
				log("TTY Read err: %d\n", errno);
				continue;
			    }
			    if (ptybuf[1] == SYNC_2) {
			        if ((status = write(ptydesc, &ptybuf[0], 1)) < 0) {
				    log("PTY Write err: %d\n", errno);
				    continue;
				}
			    }

			/* Recognized Flow Control character */
			    else {

				debug(0, ("got flow ctl\n"));

				c = SYNC_2;

				while (write(pipedesc, &c, 1) < 0) {
				    if (errno == EINTR)
					continue;
				    log("PIPE Write error: %d\n", errno);
				    break;
				}
			    }
			}
			else {
			    /* give it up, if dosout has exited.  Take data
			       as file service packets (they won't be accepted
			       because of sync/null and checksum.)
			       [2/26/88 PD] */
			    if (termoutpid == 0)
				break;
			    if ((status = write(ptydesc, &ptybuf[0], 1)) < 0) {
				if (errno == EINTR)
				    continue;
				log("PTY Write error: %d\n", errno);
				break;
			    }
			    debug(0, ("Got input %c\n", ptybuf[0]));
			}
		    }
#endif   /* RS232PCI  */
		}

	/* Request to terminate terminal emulator session */
		else if (in.hdr.req == TERM_SHELL) {
		    ti_seqnum = in.hdr.seq;
		    log("Stop Shell\n");


		/* Terminate terminal output process */
		    if(savePid = termoutpid) {
			log("Sending SIGTERM to dosout\n");
			kill(savePid, SIGTERM);
#ifndef XENIX	/* Causes file service dossvr to die: wasn't here in 2_8_1 */
		        u_wait((int *) 0);
#endif	/* XENIX */
		        termoutpid = 0;
		    }

#if	!defined(RS232PCI)
		    log("Deleting IPC members.\n");
		    rd_sdleave(sh_mem);		/* detach shared memory */
		    rd_shmdel(rd_shmid);	/* delete "segment" */
		    rdsem_unlnk(semaphore);	/* get rid of semaphore */  
#endif
		/* Close PTY device and pipe */
		close(ptydesc);
		close(pipedesc);

		/* Send response frame */
		    pckframe(&out, CONF, ti_seqnum, in.hdr.req, NEW, SUCCESS,
			NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF,
			NO_ATTR, NO_DATE, NULLstat);
		    length = xmtPacket(&out, &ndata, swap_how);

		    emulator = INITIALIZED;
		    ti_seqnum = 0;

		    /* If bridge was established only for emulator session,
			the bridge is terminated on a Stop Shell request */

		    if (bridge != RUNNING)
			stopService(3,0);
		}
		break;

	    case BRIDGE:
#ifdef	POINT_TO_POINT
		if (in.hdr.req == PROBE) {
			conProbe();
			break;
		}
#endif	  /* POINT_TO_POINT  */
		if (bridge != RUNNING) {
		    pckframe(&out, BRIDGE, (int)in.hdr.seq, in.hdr.req, NEW,
			NO_SESSION, NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE,
			NO_OFF, NO_ATTR, NO_DATE, NULLstat);
#ifdef ETHNETPCI
		/* Store destination address of PC */
		    for (i = START; i < DST; i++)
			ntmp.dst[i] = in.net.src[i];
#endif   /* ETHNETPCI  */
		    length = xmtPacket(&out, &ndata, swap_how);
		    break;
		}

	    /* Invoke the PC-DOS Bridge server */
		server(&in);
		break;
	

	    case SHELL:

#ifdef ETHNETPCI
	    /* Store destination address of PC */
		for (i = 0; i < DST; i++)
		    ntmp.dst[i] = in.net.src[i];
#endif   /* ETHNETPCI  */

	    /* Is a terminal emulator session established */
		if (!emulator) {
#if	!defined(RS232PCI)
		    /* no need to respond if this is an ACK */
		    if (rcvhead->code == RD_ACK)
			break;
#endif

		    pckframe(&out, SHELL, (int)in.hdr.seq, in.hdr.req, NEW,
			NO_SESSION, NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE,
			NO_OFF, NO_ATTR, NO_DATE, NULLstat);

		    length = xmtPacket(&out, &ndata, swap_how);
		    break;
		}
if (rd_flag) {
#if	!defined(RS232PCI)
/******************************************************************************/
/*                                                                            */
/*                    RELIABLE DELIVERY CODE                                  */
/*                                                                            */
/* Most of the receive logic for the protocol is contained in the confines of */
/* this macro ifdef block.                                                    */
/*                                                                            */
/******************************************************************************/

	log("RDR packet       dnum=%d\n",rcvhead->dnum);
	log("                 anum=%d\n",rcvhead->anum);
	log("                 tcbn=%d\n",rcvhead->tcbn);
	log("                 version=%d\n",rcvhead->version);
	log("                 code=%d\n",rcvhead->code);
	log("                 options=%d\n",rcvhead->options);
	log("                 strsiz=%d\n",rcvhead->strsiz);
	if (rcvhead->code != RD_ACK)
	      log("   %c,%c,%c,%c\n",in.text[sizeof(struct emhead)],
		in.text[sizeof(struct emhead)+1], 
		in.text[sizeof(struct emhead)+2], 
		in.text[sizeof(struct emhead)+3]);

		stream_size = rcvhead->strsiz; /* get stream size */

		if (rcvhead->code == RD_ACK || rcvhead->code == RD_PB)
		{
                   control.code = RD_ACK;
                   control.tcbn = rcvhead->tcbn;
		   control.num  = rcvhead->anum;
		   control.ssiz = stream_size; 	/* setup stream size */
		   stream_size = 0;	        /* reset stream size */
		   if (rcvhead->anum == 0)
			rd_rec_data = TRUE;	/* start after first ack */
		   log("RDR Was ACK, sending it to TERMOUT via pipe.\n");
                   if (write(pipedesc, &control, sizeof (struct ackcontrol)) < 0)
			log("PIPE Write err: %d\n",errno);
		   if (rcvhead->code != RD_PB)
		        break;
                }
		if (rd_rec_data && (rcvhead->code == RD_DATA || 
				    rcvhead->code == RD_PB)      &&
		     (rdsem_access( semaphore ) != -1)) 
		{
  		      log("In Loop: dnum = %d, FE = %d\n", rcvhead->dnum,
			sh_mem->FrameExpected);
                      accept = FALSE;
		      send_ack = FALSE;
		      sequence = 0;
                      if (sh_mem->kick_ack == 0)
		      {
			  if (rcvhead->dnum > sh_mem->FrameExpected)
			  {
			      log("RDR state 0\n");
			      log("    dropped %d\n",sh_mem->FrameExpected);
			      log("    but can accept this one.\n");
			      accept = FALSE;
                              send_ack = TRUE;
			      sequence = sh_mem->FrameExpected-1; /* ack last */
			      old_state = 0;
                          }
			  else if (rcvhead->dnum == sh_mem->FrameExpected)
			  {
			      log("RDR state 1\n");
			      log("    got what was expected.\n");
			      accept = TRUE;
			      sh_mem->kick_ack = 1;
			      sh_mem->FrameExpected++;
			      if (old_state == 0)
				 sh_mem->FrameExpected; /* resize window */
			      old_state = 1;
                          }
			  else
			  {
			      log("RDR state 2\n");
			      log("     duplicate packet.\n");
			      sequence = rcvhead->dnum;
			      accept = FALSE;
			      send_ack = TRUE;
			      old_state = 2;
                          }
                      }
                      else           /* there is a need to ack a prev pkt */
		      {
			  send_ack = TRUE;
			  sh_mem->kick_ack = 0;
			  if (rcvhead->dnum > sh_mem->FrameExpected)
			  {
			      log("RDR state 3\n");
			      log("     lock step detected.\n");
			      sequence = sh_mem->FrameExpected-1;
			      accept = TRUE;
			      old_state = 3;
                          }
			  else if (rcvhead->dnum == sh_mem->FrameExpected)
			  {
			      log("RDR state 4\n");
			      log("    got what was expected.\n");
			      accept = TRUE;
			      sh_mem->kick_ack = TRUE;
			      sequence = rcvhead->dnum-1;
			      sh_mem->FrameExpected++;
			      old_state = 4;
                          }
			  else
                 	  {
			      log("RDR state 5\n"); 
			      log("    duplicate packet.\n");
			      sequence = rcvhead->dnum;
			      accept = FALSE;
			      old_state = 5;
                          }
                      }
		      rdsem_relinq( semaphore );
		      if (send_ack == TRUE)
		      {
			  log("RDR sending ack: %d\n",sequence);
	                  pckRD( sndhead, RD_ACK, 0, sequence, 0, 
			         rcvhead->tcbn, RD_VERSION );
		          pckframe(&out, SHELL, in.hdr.seq, in.hdr.req, 
			               ACK, SUCCESS, NO_DES, NO_CNT, 
				       sizeof (struct emhead), NO_MODE, 
				       NO_SIZE, NO_OFF, NO_ATTR, 
				       NO_DATE, NULLstat);
                          xmtPacket(&out, &ndata, swap_how);
                      } 
		      if (accept)
		      {
		          log("RDR Accept data, send to pty.\n");

/* The alarm breaks us out of blocked writes that result in deadlocks */
			alarm(2);
		          if ((len=write(ptydesc,in.text+sizeof (struct emhead),
                              in.hdr.t_cnt-sizeof (struct emhead))) < 
                              in.hdr.t_cnt-sizeof (struct emhead) &&
			      errno != EINTR)
		                log("PTY Write err: %d\n",errno);
			alarm(0);
/*
 * Sometimes stream size info comes in on a RD_DATA packet. This has to
 * be transferred to termout. It is already handled by piggy back but this
 * case is only for DATA only, when accepted.
 */
			  if (rcvhead->code==RD_DATA && stream_size>0) {
		   log("RDR: GOT STREAM size info on data packet.\n");
                   		control.code = RD_DATA;
                   		control.tcbn = rcvhead->tcbn;
		   		control.num  = rcvhead->anum;
		   		control.ssiz = stream_size; 	
                   		if (write(pipedesc, &control, 
					sizeof (struct ackcontrol)) < 0)
					log("PIPE Write err: %d\n",errno);
		          }

                      }
		}
	     break;
#endif
}
else {
	    /* Pipe flow control to terminal output process */
		if((in.hdr.stat == RFNM) || (in.hdr.stat == 5))
		{
		    c = RFNM;
		    debug(0, ("Got input: send flow ctl\n"));

		    if (write(pipedesc, &c, 1) < 0)
			log("PIPE Write err: %d\n", errno);
		break;		/* don't even try to give text from this to pty.. */
		}

#ifdef SYMETRIC

	    /* Is this a duplicate input frame? */
		if (ti_seqnum != in.hdr.seq-1) {
		    if (acceptence < 0) {

		    /* Lost ACK: re-ACK input frame */
			pckframe(&out, SHELL, in.hdr.seq, in.hdr.req, ACK,
				SUCCESS, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);

			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }
		    else {
		    /* Partial acceptence of data */
			txptr = in.text + accepted;
			in.hdr.t_cnt -= accepted;
		    }
		}
		else {
		    ++ti_seqnum;
		    txtptr = in.text;
		}

#endif   /* SYMETRIC  */

/* we don't look at stat except to see if it is an ack.. */
/* at this point, acks are weeded out - this must be text */
/* we really should check, you know */

/* **** Note ***** Note ***** Note */

/* you may not like wahat this does - I know I don't and I wrote it */
/* the test for duplicate packets looks to see if the currently    */
/* received packet is less than what we expected. if it isn't it must be */
/* at least what we want, in which case we use it. otherwise, it must be */
/* a duplicate packet of some sort. the trouble is that sequence numbers */
/* wrap around at 255 to 0. imagine we want 255 and miss it and get 0 ... */
/* we say "is 0 less than 255?" and we chuck it because it must be a dup */
/* this works for any wrapped pair...  how do we fix this? easy, we have a */
/* "window" of duplicate checking. Anything outside this window is assumed */
/* too old to be a dup of a recent packet and must therefore be a wrapped */
/* packet. we accept it for what it is (if only parents could do the same */
/* with their children) and re-synchronize on it */


	if (++ti_check == TI_CHECK_INTERVAL)
	{
		pcDbg(("ti_check: ti_seqnum %d, actually %d\n",
			ti_seqnum, in.hdr.seq));
		ti_check = 0;	/* reset this */
	}


#if	!(defined(RIDGE) || defined(IX370))
	/*  This is taken out for Ridge(and 370's) due to bug in the PC 
	    bridge where
	    sequence numbers are not reset upon subsequent entries in to 
 	    terminal emulation
	*/
	if ((ti_seqnum != in.hdr.seq) && ((ti_seqnum - (int)in.hdr.seq) <= TI_DUP_WINDOW))
	{
		pcDbg(("em dup %d - dropped\n", in.hdr.seq));
		break;
	}
#endif	/* RIDGE */

 if (ti_seqnum != in.hdr.seq)
	{
		pcDbg(("break in em sequence\n"));
		ti_seqnum = in.hdr.seq;	/* re-sync */
	}

	ti_seqnum = (ti_seqnum + 1) & 0xFF;	/* byte from PC */


	    /* Send any data thru PTY into shell */
		txtptr = in.text;
		if (in.hdr.t_cnt) {
		    if ((len = write(ptydesc, txtptr, in.hdr.t_cnt)) != in.hdr.t_cnt) {

#ifdef SYMETRIC

/*
 * For partial success on write save the amount of data actually written,
 * and do not ACK frame.
 */
			if (accepted < 0)
			    log("PTY Write err: %d\n", errno);
			else {
			    accepted = (accepted < 0) ? len : accepted + len;
			    debug(0, ("Partial input: %d| %s\n", in.hdr.t_cnt,
				in.text));
			}
			break;

#else   /* ~SYMETRIC  */

			log("PTY Write err: %d\n", errno);

#endif   /* SYMETRIC  */
		    }

#ifdef SYMETRIC
		/* Acknowledge input frame and reset acceptence flag */
		    pckframe(&out, SHELL, in.hdr.seq, in.hdr.req, ACK, SUCCESS,
			NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF,
			NO_ATTR, NO_DATE, NULLstat);

		    length = xmtPacket(&out, &ndata, swap_how);

		    accepted = -1;

#endif   /* SYMETRIC  */

		    debug(0, ("input data: %.*s\n", in.hdr.t_cnt, in.text));
		}

}  /* else not reliable delivery */
                   break;

	    default:

		log("Bad select: %d\n", in.pre.select);
		pckframe(&out, CONF, (int)in.hdr.seq, in.hdr.req, NEW,
			INVALID_FUNCTION, NO_DES, NO_CNT, NO_CNT, NO_MODE,
			NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);

#ifdef ETHNETPCI

	    /* Store destination address of PC */
		for (i = 0; i < DST; i++)
		    ntmp.dst[i] = in.net.src[i];

#endif   /* ETHNETPCI  */
		length = xmtPacket(&out, &ndata, swap_how);

		break;
	}/*endswitch*/
    }/*end for(;;)*/
}


/*
   getVers: Extract version number from program name
*/

getVers(progName)
char
	*progName;
{
	register char
		*versPart;		/* Version number in program name */

	/* Find the first digit in program name */
	if ((versPart = strpbrk(progName, "0123456789")) == (char *)NULL)
		return 0;

	/* Avoid the "232" part of the rs232 servers name */
	/* This is a kludgey hack! */
	if (versPart[0] == '2' && versPart[1] == '3' && versPart[2] == '2')
		versPart += 3;

	return atoi(versPart);
}


#ifdef	RS232PCI

/*
   dosSvrAlive: Tell PC that the RS232 server is waiting
*/

dosSvrAlive(status)
int
	status;
{
	pckframe(&out, CONF, 0, RS232_ALIVE, NEW, status, NO_DES, NO_CNT,
		NO_CNT, NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);

	/* The following delay is to workaround a timing problem in the
	   bridge in which very fast UNIX hosts return this packet before
	   the bridge is ready to handle it */
	sleep(1);

	length = xmtPacket(&out, &ndata, HOW_TO_FLIP);
}
#endif    /* RS232PCI  */


#ifdef	POINT_TO_POINT

/*
   sendMap: Construct response to a map request
*/

sendMap(respPkt)
struct output
	*respPkt;
{
	setnameAddr((struct nameAddr *)respPkt->text);
	pckframe(respPkt, DAEMON, in.hdr.seq, SEND_MAP, NEW, 0, 0,
		1, sizeof(struct nameAddr),
		0, 0, 0, 0, 0, NULLstat);
}


setnameAddr(myName)
struct nameAddr
	*myName;
{
	char
		*myhostname();
#ifdef	UDP42
	static struct hostent
		*uName = 0;
	extern struct hostent
		*myhostent();
#endif   /* UDP42  */

	strncpy(myName->name, myhostname(), sizeof myName->name);

#ifndef	POINT_TO_POINT
#ifndef	UDP42
	nAddrCpy(myName->address, mapSvrHeader.src);
#else	  /* UDP42  */
	if (!uName)
		uName = myhostent();
	nAddrCpy(myName->address, uName->h_addr);
#endif   /* UDP42  */
#endif	  /* !POINT_TO_POINT  */
}
do_connect()
{

	if (already_connected) {
		log("logout from CONNECT\n");
		/* pretend this was a DISCONNECT */
		stopService(4,0);
	}
	/* copy protection startup processing */
	serialCpy(myserial, in.text);
	serial2str(myserial,myser);
	if (!validSerial(myserial, swap_how)) {
		killPc();
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("P_CONNECT12",
			"%1:dev %2:Invalid serial number:serial %3\n"),
			"%s%s%s",
			"do_connect:",mydevname,myser));
	}
	already_connected = 1;
	protStart();
	signal(SIGALRM, sig_catcher);
	alarm(POLLINTERVAL);
	pckframe(&out, DAEMON, in.hdr.seq, CONNECT, NEW, 0, 0, 0, 0,
		0, 0, 0, 0, 0, NULLstat);
	length = xmtPacket(&out, &ndata, swap_how);
}

/*
   conProbe: Keep connection alive
*/

conProbe()
{
	log("Probe from %s\n",mydevname);
	probeCount = 0;
}

/*
   ageConnects: Age connection table
*/

ageConnects()
{

	log("ageConnects:probeCount %d\n",probeCount);
	/* Probe timeout reached? */
	if (++probeCount < PROBE_TIMEOUT) return;
	if (noDisconnect) {
		log("Probe timeout - DON'T disconnect %s\n",
			mydevname);
	} else {
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("P_CONNECT13",
			"Probe timeout - disconnect dev %1, pid %2\n"),
			"%s%d", mydevname,getpid()));
		kill(getpid(), SIGTERM);
	}
}
#endif	  /* POINT_TO_POINT  */



/*
 * sig_catcher() -		Signal catcher.
 */

void
sig_catcher(signum)
register int signum;
{
    int
	pid,			/* Process id of children */
	cstat;			/* Contains status of child termination */

    char
	logfilename[MAX_PATH];	/* Logfile temporary name */

    switch (signum) {
	case SIG_DBG1:
	case SIG_DBG2:
	    newLogs(DOSSVR_LOG,getpid(),(long *)NULL,(struct dbg_struct *)NULL);
	    if (logStr != (char *)NULL)
		sprintf(logStr, "%04x", dbgEnable);
	    break;

	case SIGINT:
	case SIGTERM:
	case SIGHUP:
	    log("Signal %d - Quit\n", signum);
	    stopService(3,0);
	    break;

	case SIG_CHILD:
	    pid = u_wait(&cstat);
	    childExit(pid, cstat);
	    break;

	case SIGALRM:
		sched();
	    break;
    }
    signal(signum, sig_catcher);
}

sched()
{
#ifdef	POINT_TO_POINT
	protPoll();	/* security check */
	if (!noDisconnect) ageConnects();
	alarm(POLLINTERVAL);
#endif	  /* POINT_TO_POINT  */
}


/*
   stopService: Shut down dos server and termout process, if any.  Kill all
		children.
*/

stopService(dum1, dum2)
int dum1, dum2; /* these are not used. Other versions stopService do. */
{

	/* The re-entry gate may have a hole in it.  If the hole turns out
	 * to matter, someone better find a way to plug it.
	 */
	if (stopGate++) { 
		stopGate--;
		return;
	}
	/* Clean-up tmp files (pipes, etc...) */
	unlink_on_termsig();

#ifdef RLOCK  /* record locking */
	/* Close all dossvr's files, removing locks */
	/* and removing entry in the global open file table */
	close_all();	
#endif  /* RLOCK */

	/* Kill terminal emulation output process, if it exists */
	if ((savePid = termoutpid) != 0) {
#if	!defined(RS232PCI)
		rdsem_unlnk(semaphore);		/* remove semaphore */
		rd_sdleave(sh_mem);		/* detach if needed */
		rd_shmdel(rd_shmid);		/* remove shared memory */
#endif
		signal(SIG_CHILD, SIG_IGN);
		log("Kill dosout %d\n", savePid);
		kill(savePid, SIGTERM);
		u_wait((int *) 0);
	}

#ifdef	RS232PCI
	/* Reset tty modes */
	drain_tty(netdesc);
	set_tty(netdesc, &ttysave);
#endif	  /* RS232PCI  */
	
	/* Kill all children and wait for their termination status */
	kill_uexecs();
	while (wait((int *)NULL) != -1);

	/* Exit normally */
	log("stopService %d,%d:exit(0)\n",dum1,dum2);
	exit(0);
}


childExit(pid, status)
register int
	pid,				/* Pid of dead child process */
	status;				/* Exit status */
{
register int i;

	if (pid == termoutpid) {
		termoutpid = 0;
		close(ptydesc);
		close(pipedesc);
		emulator = INITIALIZED;
		log("Termout %d; exit %d:%d\n", pid, ((status >> 8) & 0xff),
			(status & 0xff));
#if	defined(UDP42) && !defined(DONT_START_GETTY)
		/* Tell CONSVR dosout quit so login on pty can be killed! */
		pckframe(&conbuf, DAEMON, brg_seqnum, (unsigned char)K_LOGIN,
			 NEW, 0, getpid(),
			login_pid, 0, 0, 0, 0, 0, 0, NULLstat);
		xmtPacket(&conbuf, &conSvrHeader, NOFLIP);
		do {
			rcvPacket(&cibuf);
			/* Is this the packet I've been waiting for? */
			if (nAddrEq(cibuf.net.src, conSvrHeader.dst))
				break;
		    } while(cibuf.hdr.req != (unsigned char)K_LOGIN);

		    if (cibuf.hdr.req != (unsigned char)K_LOGIN) {
			/* Something is amis here, let's just forget 	*/
                        /* the whole thing!                         	*/
			/* we'll just drop the packet and let the pc
			   retry with it's current request */
		    }
#endif   /* UDP42  && !DONT_START_GETTY */
	}
}

#ifdef	RESTRICT_USER

/*
** restrictUser: Opens and searches RESTRICTFILE for name.
**
**	RESTRICTFILE needs to be opened every time because it can change
**	between invocations.
**
**	Returns 1 if name is found
**		0 otherwise
**
**	If RESTRICTFILE can't be opened for some reason, 0 is returned.
*/
#define	NL		'\n'

int
restrictUser(name)
	char *name;
{
	static char buf[BUFSIZ];
	register char *bp, *np;
	register int fd, i;

	if ((fd = open(RESTRICTFILE, O_RDONLY)) < 0)
		return(0);
	while ((i = read(fd, buf, BUFSIZ)) > 0) {
		bp = &buf[i-1];
		for (i = 0; *bp != NL && bp >= buf; i++)
			bp--;	/* move back to end of last name */
		*bp = NULL;
		if (lseek(fd, (long) -i, 1) < 0) {
			close(fd);
			return(0);
		}
		bp = buf;
		while (np = name, *bp) {
			while (*np == *bp && *bp)
				np++, bp++;
			if ((*np == NULL) && (*bp == NL || *bp == NULL)) {
				close(fd);
				return(1);
			}
			else
				while (*bp != NL && *bp)
					bp++;
			bp++;
		}
	}
	close(fd);
	return(0);
}
#endif	  /* RESTRICT_USER  */

#ifdef  LOCUS
#define VALID_SITE 1
#define INVALID_SITE 0
    /* get_valid_sites() places 1's in txperms[] for valid sites and */
    /* 0's for invalid sites.  If valid sites cannot be determined,  */
    /* all sites are considered to be valid.			     */
get_valid_sites(txperms)
char txperms[];		/* array in which we set the site permissions */
{
	register int i;
	struct	passwd *pwd;
	struct	pw_gecos *pwGecos;
	char *a_grp;		/* access group as determined from passwd */

	pwd = getpwuid(pptr->pw_uid);
	    /* site 0 is NEVER valid */
	txperms[0] = INVALID_SITE;
	/* default to all sites are valid */
	for (i = 1; i < MAXSITE; ++i) txperms[i] = VALID_SITE;
	if (pwd == (struct passwd *)NULL) return;

	/* Now parse the pw_gecos field to get the site access perm. */
	pwGecos = parseGecos(pwd->pw_gecos);
	
	a_grp = pwGecos->siteAccessPerm;
	    /* Check for empty string which implies no restrictions */
	if (*a_grp != '\0') {
		    /* try to get access sites from library call */
		switch (get_access_sites(a_grp, txperms)) {
		case 0:
			break;	    /* successful */
		case 1:
			perror("Cannot open /etc/sitegroup");
			break;
		case 2:
			fprintf(stderr, "Access group %s is unknown\n", a_grp);
			break;
		}
	}
}
#endif     /* LOCUS   */

#ifdef	SETUTMP

#include <utmp.h>

struct utmp *getutent();
/*
 * set_utmp		This routine sets the entry in the utmp file that
 *			corresponds to the TCA line.  This will make the
 *			TCA/PCI user look like a "real" user.
 *
 *	Entry:
 *		ptr	Pointer to a passwd table entry
 *
 *	Returns:
 *		Nothing
 */
set_utmp(ptr)
register struct passwd *ptr;
{
	int	pid;		     /* utmp key is either my pid or parent's */
	int	ppid = getppid();
	int	fd;
	struct	utmp	*uptr, ubuf;

	strncpy(ubuf.ut_user, ptr->pw_name,		/* get user name */
		sizeof(ubuf.ut_user));
	strncpy(ubuf.ut_line, (mydevname + sizeof("/dev/")-1),
		sizeof(ubuf.ut_line));
	ubuf.ut_pid = pid = getpid();
	ubuf.ut_type = USER_PROCESS;			/* entry type */
	time(&ubuf.ut_time);				/* set time */

	while ((uptr = getutent()) != (struct utmp *)NULL) {
		if ((uptr->ut_pid == pid) || (uptr->ut_pid == ppid)) {
			ubuf.ut_id[0] = uptr->ut_id[0];
			ubuf.ut_id[1] = uptr->ut_id[1];
			ubuf.ut_id[2] = uptr->ut_id[2];
			ubuf.ut_id[3] = uptr->ut_id[3];
			pututline(&ubuf);	/* write the record */
			endutent();
		/* now write to wtmp */
			if ((fd = open(WTMP_FILE, 2)) != -1) {
				lseek(fd, 0L, 2);
				write(fd, &ubuf, sizeof(ubuf));
				close(fd);
			}
			if (chmod(mydevname, 0600) < 0)
				log("set_utmp: chmod, dev: %s, errno: %d\n",
				    mydevname, errno);
			if (chown(mydevname, ptr->pw_uid, ptr->pw_gid) < 0)
				log("set_utmp: chown, dev: %s, errno: %d\n",
				    mydevname, errno);
			return;
		}
	}
	endutent();
	log("set_utmp: unable to find utmp entry.\n");
}
#endif	/* SETUTMP */
