/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)mbus:cmd/mbusutils/mb2.d/cci.d/bin.d/ccilinfo.c	1.3"

static char ccilinfo_copyright[] = "Copyright 1987 Intel Corp. 461781";

#include "common.h"
#include <signal.h>
#include "msg.h"
#include "cci.h"
#include <stdio.h>
#include "main.h"
#include "utils.h"

	/* Global Variables */
	
main(argc, argv)

int 	argc;
char	*argv[];


{
	unsigned long			actual;
	unsigned short			dest_host;
	int				status;
	unsigned int			i;
	unsigned char			line;
	req_rec				req_buf;
	cci_line_info_resp_rec	linfo_rep_buf;
	unsigned short	host_id[MAX_HOSTS];
	char 					*ptr;
	short					errflag = FALSE;
	short					c;
	extern char 			*optarg;
	extern short			optind;
	unsigned short			port_id = DEF_PORT_ID;
	
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, on_intr);
	
	if ((c = getopt(argc,argv,"p:")) != -1) 
		switch (c) {
		case 'p' : 
				port_id = (unsigned short) strtol(optarg,&ptr,BASE_TEN);
				if (*ptr != NULL) {
					printf("%s error: port_id <%s> not an integer\n",argv[0],optarg);
					errflag = TRUE;
				}
				break;
		case '?' : 
		default	 :	
				errflag = TRUE;
				break;
		}
	
	if ((argc - optind) != 2 || errflag) {
		printf("Usage : %s [-p portid] <dest host> <line> \n",
				argv[0]);
		exit(-1);
	}
	
	
	dest_host  = (unsigned short) strtol(argv[optind],&ptr,BASE_TEN);
	if (*ptr != NULL) {
		printf("%s error: dest_host <%s> not an integer\n",argv[0],argv[optind]);
		errflag = TRUE;
	}
	optind++;
	line  = (unsigned short) strtol(argv[optind],&ptr,BASE_TEN);
	if (*ptr != NULL) {
		printf("%s error: line <%s> not an integer\n",argv[0],argv[optind]);
		errflag = TRUE;
	}

	if (errflag == TRUE)
		exit(1);


	if (cci_init(dest_host,port_id) != E_OK) {
		perror("cci init");
		exit(1);
	}

	req_buf.buf[0] = CCI_GET_LINE_INFO;
	req_buf.buf[2] = line;

	actual = cci_send_cmd(CCI_PORTID,  &req_buf, &linfo_rep_buf, NULL, 0L, 
							&host_id[0],
						(unsigned long)(sizeof(host_id)), &status);

	if (status != E_OK) {
		printf("CCI SEND COMMAND Error : %x\n", status);
		myexit(1);
	}

	if (linfo_rep_buf.status != E_OK) {
		printf("CCI Error : %x\n", linfo_rep_buf.status);
		myexit(1);
	}

	printf("Line %d is ", linfo_rep_buf.line);
	if (linfo_rep_buf.state == 0)
		printf("not bound\n");

	else if (linfo_rep_buf.state == 1) {
		printf("bound to ");
		printf("Line Discipline %d\n", linfo_rep_buf.line_disc_id);
		printf("	Number of subchannels = %d\n", 
					linfo_rep_buf.num_subchannels);
	}
	
	else 
		printf("in an illegal state\n");

	if (linfo_rep_buf.state == 1) {
		printf("	Num Hosts = %d\n", linfo_rep_buf.num_hosts);
		if (linfo_rep_buf.num_hosts > 0) {
			printf("	Host Id -> ");
			for (i=0; i < linfo_rep_buf.num_hosts; i++) 
				printf("%d ", host_id[i]);
			printf("\n");
		}
		
	}		
}
