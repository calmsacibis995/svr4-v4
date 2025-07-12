/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/pp_consvr.c	1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)pp_consvr.c	4.5	LCC);	/* Modified: 16:13:59 6/20/90 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#ifdef	POINT_TO_POINT

#define	TI_CHECK_INTERVAL	5	/* log a check every 5 packets */
#define	TI_DUP_WINDOW		128	/* size of the window to forget about dups */

/*			 PC-INTERFACE  Configuration Manager		*/
#include	"pci_types.h"
#include	<errno.h>
#include	<pwd.h>
#include	<fcntl.h>

#include	<string.h>
#include	<signal.h>


#ifdef RS232PCI
#ifdef	XENIX
#include	<ioctl.h>
#endif	/* XENIX */
#include	<termio.h>		/* TTY structs/constants */
#endif /* RS232PCI */

#include	<stat.h>
#include	"const.h"
#include	"flip.h"

#define	pcDbg(dbgArgs)	debug(0x40, dbgArgs)

/*			    External Functions 				*/

extern void
	    exit();


extern	int 
	    rcvPacket(),		/* Receive packet */
	    xmtPacket();		/* Send packet */

extern unsigned
	    alarm();

extern	void 
	    protInit(), protPoll(), protStart(), /* copy protection funcs */
	    pckframe();			/* Copies header data to output buf */


extern long
	    time(),
	    lseek();



extern	struct	passwd
	    *getpwuid(),		/* Points to user's password entry */
	    *getpwnam();		/* Returns pointer into password file */

extern char
		*crypt(),		/* Unix password encryptor */
		*nAddrFmt();		/* Format a net address for printing */

extern	int
	   (*signal())();


int
	    sig_catcher();		/* Signal catching routine */

int	
	    bridge = INITIALIZED,	/* Indicates bridge access is active */
	    swap_how,			/* Byte order for auto-sense flipping */
	    emulator = INITIALIZED,	/* Indicates term emulation is active */
	    versNum,			/* Version number of this server */
	    netdesc = -1,		/* File descriptor of net device */
	    pipedesc,			/* File descriptor of control pipe */
	    ptydesc,			/* File descriptor of pseudo TTY */
	    length,			/* Length of last configuration frame */
	    accepted = -1,		/* Number bytes accepted on PTY write */
	    files_open,			/* Number of files open on Bridge PC */
	    brg_seqnum,			/* Last Bridge frame sequence number */
	    ti_seqnum,			/* Last emulator input seq number */
	ti_check,			/* counter to check input characters */
	    termoutpid,			/* Id of terminal output process */
	    kidpid,
	    descriptors[2];		/* File descriptors associated w/pipe */

char	
	myserial[SERIALSIZE],		/* serial number of our pc bridge */
	myser[SERIALSTR],	/* hex repr of serial number */
	    cwd[MAX_CWD],		/* Contains current working directory */
	    ptydevice[MAX_FNAME + 1],	/* Contains line in /etc/brgptys file */
	    cntrlpty[MAX_FNAME + 1],	/* Name of controlling PTY device */
	    slvpty[MAX_FNAME + 1],	/* Name of slave PTY device */
	    copyright[] =		"PC BRIDGE TO UNIX FILE SERVER\
COPYRIGHT (C) 1984, LOCUS COMPUTING CORPORATION.  ALL RIGHTS RESERVED.  USE OF\
THIS SOFTWARE IS GRANTED ONLU UNDER LICENSE FROM LOCUS COMPUTING CORPORATION.\
ANY UNAUTHORIZED USE IS STRICTLY PROHIBITED.";

struct  ni2 ndata;                      /* Ethernet device header structure */
					/* (dummy struct when not ethernet) */

#ifdef RS232PCI
struct	termio
	    ttysave,			/* Saves TTY modes for later */
	    ttymodes;			/* TTY modes structure */

#ifdef RS232_7BIT
int	    using_7_bits = 0;		/* Using 7 bits */
#endif
#endif /* RS232PCI */

static struct	input				/* Input buffer for RS-232 */
	    in;

static struct	output
	    out;			/* Output buffer for RS-232 */

extern	int
	    errno;			/* Error return from system calls */
int	myerrno;

char
	*logStr;			/* Log level string (shows in ps) */


#define	NULLstat	(struct stat *)0

char *mydevname = "unknown";

int oldserial= 1;

/*
 *  main()  arguments:  (any order)
 * -ny   ethernet network descriptor y.        : only when ETHERNET
 * -pz   3B2 ni driver logical port number z.  : only when for 3b2
 * -Dn   debug level n          default 0				*/
#ifdef TCA
/* -tdev talk on file "dev"						*/
#endif /* TCA */

char *myname;

main(argc, argv)
int argc;
char *argv[];
{
    register int 
	len,			/* Temporary length of string */
	status,			/* Return value from system call */
	i;			/* Loop counter for loading source addresses */
	int length;
#ifdef	RS232PCI
extern char *ttyname();
    char
	ptybuf[10];		/* Character buffer for terminal emulation */
#endif

    register struct passwd
	*pptr;			/* Pointer to entry in password file */

    int
	portnum;

    char
	c;			/* Character for writing onto PIPE */

    FILE 
	*fptr;			/* File pointer into PTY configuration table */

    char
	fileName[MAX_PATH],	/* Logfile temporary name */
	*ptr,			/* Pointer to character array */
	*txtptr,		/* Pointer into input text buffer */
	*cryptPass,		/* Return from crypt(3) */
	fname[120];             /* a file name */

int	pipedes[2];
int	noDisconnect=1;		/* NO disconnect on probe timeout MUST be 1 */
char
	errArg[8],		/* Error descriptor */
	pipeArg[8],		/* Ack pipe descriptor */
	disArg[8],		/* nodisconnect arg */
	ptyArg[8],		/* Pty descriptor */
	swapArg[8],		/* Byte swapping code */
	descArg[8],		/* Network (eth or 232) descriptor */
	naddrArg[32];		/* Network address (eth) */
char
	svrProg[64],			/* Name of server program */
	debugArg[16],			/* Debug level argument */
	netArg[16],			/* Network descriptor number */
	portArg[16],			/* Port number */
	netDev[64],			/* Network device name */
	queueSize[16];
int
	argN;			/* Current argument number */
char
	*arg;			/* Current argument */
#ifdef VERSION_MATCHING
struct connect_text *connText;
#endif /* VERSION_MATCHING */
int	conNum;				/* Servers port number */
int	qsize = DSVR_QSIZE;

	/* initialize NLS */
	nls_init();

	myname = argv[0];
	if (*myname == '\0')
		myname = "unknown";

#ifdef TCA

	/* Close all open files */
	for (i = 0; i < uMaxDescriptors(); i++)
		close(i);

#endif /* TCA */


	/* Decode arguments */
	for (argN = 1; argN < argc; argN++) {
		arg = argv[argN];

		if (*arg != '-')
			continue;

		switch (arg[1]) {
		case 'D':		/* Log level */
			logStr = &arg[2];
			dbgSet(strtol(logStr, NULL, 16));
			break;

		case 'n':		/* Network descriptor */
			netdesc = atoi(&arg[2]);
			break;
#ifdef TCA
		case 't':
			mydevname = arg+2;
			break;
#endif /* TCA */
		case 'x':
			noDisconnect = 1;
			break;
		case 'p':		/* 3B2 ni driver logical port number */
			portnum = atoi(&arg[2]);
			break;
#if	defined(RS232PCI) && defined(RS232_7BIT)
		case '7':
			using_7_bits++;
			break;
#endif	/* RS232PCI && RS232_7BIT */
		}
	}

    if (dbgCheck(~0))
	logOpen("consvr", getpid());

    signal(SIGTERM, sig_catcher);	/* Catch signal from pcidaemon */
    signal(SIGINT, sig_catcher);	/* Catch signal from tty (break) */
    signal(SIGHUP, sig_catcher);	/* Catch signal from tty (disconnect) */
    signal(SIG_DBG1, sig_catcher);      /* Catch signal to toggle logs */
    signal(SIG_DBG2, sig_catcher);      /* Same */
    signal(SIG_CHILD, sig_catcher);	/* Catch signal if pcitermout dies */


#ifdef TCA
	if (netdesc == -1) {
		if(!mydevname)
			fatal(lmf_get_message("PP_CONSVR1",
				"TCA device not specified\n"));
		if((netdesc = open(mydevname, 2)) < 0)
			fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("PP_CONSVR2",
				"Can't open \"%1\" errno: %2\n"),
				"%s%d" , mydevname, errno));
	}
	netUse(netdesc);
#endif /* TCA */

#ifdef RS232PCI

    netdesc = STDIN;
    if (get_tty(netdesc, &ttysave) < 0)
	fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("PP_CONSVR3","Error %1 on get_tty\n"),
		"%d", errno));

    ttymodes = ttysave;

    mydevname = ttyname(netdesc);
/* Set up TTY driver for eight bit "raw" mode */
    rawify_tty(&ttymodes);

    if (set_tty(netdesc, &ttymodes) < 0)
	fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("PP_CONSVR4","Error %1 on STDIN set_tty\n"),
		"%d", errno));

    /* Give network (tty) descriptor to network interface */
    netUse(netdesc);

/* Get current working directory from password file */
    if ((pptr = getpwuid(getuid())) == NULL)
	fatal(lmf_get_message("PP_CONSVR5",
		"Error getting current working directory\n"));

    /* Start bridge in home directory */
    if (chdir(pptr->pw_dir) < 0) {
	dosSvrAlive(LOGIN_FAILED);
	stopService(0,0);
    }
    else
        dosSvrAlive(SUCCESS);

    strcpy(cwd, pptr->pw_dir);

    bridge = RUNNING;

#endif /* RS232PCI */


    /* copy protection initialization */
    protInit();

/*
 * Service configuration requests ,bridge packets,
 * or terminal emulator packets.
 */ 
     for (;;)
     {
	int j;
getrequest:
	swap_how = rcvPacket(&in);

/*
 * Retransmit previous response upon receipt of previous sequence number.
 */
	if (in.pre.reset) {
	    brg_seqnum = 0;
	    ti_seqnum  = 0;
	ti_check = 0;
	}
	else if ((in.pre.select == CONF || in.pre.select == DAEMON)
	     &&  in.hdr.seq == brg_seqnum)
	{
	    log("reXmt due to seqnum\n");
	    reXmt(&out, length);
	    goto getrequest;
	}

	switch (in.pre.select) {
	    /* Respond to some messages normally handled by other servers */
	    case DAEMON:
		if (in.hdr.req == CONNECT) {
		    /* copy protection startup processing */
		    serialCpy(myserial, in.text);
		    serial2str(myserial,myser);
		    if (!validSerial(myserial, swap_how)) {
			log("copy protect violation myser %s\n",myser);
			killPc();
			fatal(lmf_get_message("PP_CONSVR6",
				"Copy protection violation, PC zapped.\n"));
		    }
		log("myser %s\n",myser);
		/*
		 * Start putting in the hooks to start up alternate servers.
		 * Right now we are just printing out the PCs version number.
		 */

		if (pipe(pipedes) < 0) {
			fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("PP_CONSVR7",
				"Can't create pipe:errno %1\n"),
				"%d",errno));
		}

		log("pipe[0] = %d, pipe[1]= %d\n",pipedes[0],pipedes[1]);


		log("consvr forking kid\n");
		if ((kidpid = fork()) < 0 ) {
			fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("PP_CONSVR8",
				"Can't fork:errno %1\n"),
				"%d", errno));
		}

		if (kidpid == 0) {
			int cnt = sizeof in;
			int pcnt;
			/* this one is the kid */
			/* give the server the input packet and go away */
			output_swap(&in,swap_how);	/* put it back */
			if ((pcnt=write(pipedes[1],&in,cnt)) !=  cnt) {
				fatal(lmf_format_string((char *) NULL, 0, 
					lmf_get_message("PP_CONSVR9",
					"Can't write connmsg to pipe:errno %1,cnt %2, pcnt %3\n"),
					"%d%d%d", myerrno = errno,cnt,pcnt));
			}
			close(pipedes[1]);
			exit(0);
		}

		/* this code runs in the parent */

#ifdef VERSION_MATCHING
		connText = (struct connect_text *) in.text;
		log("PC version: %d.%d.%d \n", (int) connText->vers_major,
		    (int) connText->vers_minor, (int) connText->vers_submin);
#endif /* VERSION_MATCHING */

/*
 * Here is where we look for a matching server. If none that is executable
 * then inform the user about that.
 */
	if (in.hdr.versNum == 0) {	/* only on standard startup */
#ifdef VERSION_MATCHING
		sprintf(svrProg, "%s/%d/%d/%d/%s", PCIDIR,
			(int) connText->vers_major, (int) connText->vers_minor,
			(int) connText->vers_submin,PCIDOSSVR);
		if (access(svrProg, 0x01) < 0) {
			pckframe(&out, DAEMON, (int)in.hdr.seq, in.hdr.req, 
				 NEW, FAILURE, NO_DES, NO_CNT, NO_CNT,
				 NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, 
				 NULLstat);
			out.hdr.b_cnt = 1;	/* version error */
			length = xmtPacket(&out, &ndata, swap_how);
			fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("PP_CONSVR10",
				"Can't access DOS server `%1'\n"),
				"%s", svrProg));
		}
#else	/* !VERSION_MATCHING */
		sprintf(svrProg, "%s/%s", PCIDIR,PCIDOSSVR);
#endif	/* VERSION_MATCHING */
#ifdef	IBM_SERCHK
		if (serchk(&in,swap_how) == 0) {
			/* bad serial# for version */
			pckframe(&out, DAEMON, (int)in.hdr.seq, in.hdr.req, 
				 NEW, FAILURE, NO_DES, NO_CNT, NO_CNT,
				 NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, 
				 NULLstat);
			out.hdr.b_cnt = 1;	/* version error */
			length = xmtPacket(&out, &ndata, swap_how);
			fatal(lmf_get_message("PP_CONSVR11",
				"Bad serial number for version\n"));
		}
#endif	/* IBM_SERCHK */
	}
		if (in.hdr.versNum != 0)	/* cmd line "-s<svr number>" */
			sprintf(svrProg, "%s/%s%d", PCIDIR, PCIDOSSVR,
				in.hdr.versNum);

		sprintf(debugArg, "-D%s", logStr);
		sprintf(netArg, "-n%d", netdesc);
		sprintf(portArg, "-p%d", conNum);
		sprintf(queueSize, "-Q%d", qsize);
		sprintf(netDev,"-t%s",mydevname);
		sprintf(pipeArg,"-P%d",pipedes[0]);
		sprintf(disArg,noDisconnect?"-x":"--");
		log("execing %s with %s %s %s %s %s %s\n",svrProg,
			netDev, debugArg, disArg, pipeArg,
			netArg, portArg, queueSize);
			
		logClose();
  execl(svrProg, PCIDOSSVR,
			netDev, debugArg, disArg, pipeArg,
			netArg, portArg, queueSize, (char *) 0);

		myerrno = errno;
		pckframe(&out, DAEMON, (int)in.hdr.seq, in.hdr.req, NEW, INVALID_FUNCTION, NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
		length = xmtPacket(&out, &ndata, swap_how);
		/* All dressed up, no place to go */
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("PP_CONSVR12",
			"Can't exec DOS server `%1': %2\n"),
			"%s%d", svrProg, myerrno));
		}
		else if (in.hdr.req == SEND_MAP)
		    sendMap(&out);
		else if (in.hdr.req == DISCONNECT)
		    stopService(0,0);
		else
		    break;

		length = xmtPacket(&out, &ndata, swap_how);
		break;
	    default:

		log("Bad select: %d\n", in.pre.select);
		pckframe(&out, CONF, (int)in.hdr.seq, in.hdr.req, NEW, INVALID_FUNCTION, NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
		length = xmtPacket(&out, &ndata, swap_how);
		break;
	}/*endswitch*/
    }/*end for(;;)*/
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
		NO_CNT, NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULL);
	
	/* The following delay is to workaround a timing problem in the
	   bridge in which very fast UNIX hosts return this packet before
	   the bridge is ready to handle it */
	sleep(2);

	length = xmtPacket(&out, &ndata, HOW_TO_FLIP);
}
#endif  /* RS232PCI */

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
		0, 0, 0, 0, 0, NULL);
}


setnameAddr(myName)
struct nameAddr
	*myName;
{
	char
		*myhostname();

	strncpy(myName->name, myhostname(), sizeof myName->name);

}

/*
 * sig_catcher() -		Signal catcher.
 */

int
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
	    newLogs("consvr", getpid(), NULL, NULL);
	    if (logStr != NULL)
		sprintf(logStr, "%04x", dbgEnable);
	    break;

	case SIGTERM:
	case SIGINT:
	case SIGHUP:
	    log("Signal %d - Quit\n", signum);
	    stopService(0,0);
	    break;

	case SIG_CHILD:
		log("SIG_CHILD\n");
		wait(0);
    }
    signal(signum, sig_catcher);
}


/*
   stopService: Shut down dos server and termout process, if any.
*/

stopService(dum1, dum2)
int dum1, dum2; /* these are not used. Other versions stopService do. */
{

#ifdef	RS232PCI
	/* Reset tty modes */
	drain_tty(netdesc);
	set_tty(netdesc, &ttysave);
#endif	/* RS232PCI */

	/* Exit normally */
	log("exit(0)\n");
	exit(0);
}


#endif	/* POINT_TO_POINT */
