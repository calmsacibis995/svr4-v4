/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:filter/postscript/postio/slowsend.c	1.2.2.1"
/*
 *
 * Stuff that slows the transmission of jobs to PostScript printers. ONLY use it
 * if you appear to be having trouble with flow control. The idea is simple - only
 * send a significant amount of data when we're certain the printer is in the
 * WAITING state. Depends on receiving status messages and only works when the
 * program is run as a single process. What's done should stop printer generated
 * XOFFs - provided our input buffer (ie. blocksize) is sufficiently small. Was
 * originally included in the postio.tmp directory, but can now be requested with
 * the -S option. Considered eliminating this code, but some printers still depend
 * on it. In particular Datakit connections made using Datakit PVCs and DACUs seem
 * to have the most problems. Much of the new stuff that was added can't work when
 * you use this code.
 *
 */


#include <stdio.h>

#include "gen.h"
#include "postio.h"


extern char	*block;
extern int	blocksize;
extern int	head;
extern int	tail;
extern char	*line;
extern char	mesg[];
extern int	ttyo;


/*****************************************************************************/


slowsend(fd_in)


    int		fd_in;			/* next input file */


{


/*
 *
 * A slow version of send() that's very careful about when data is sent to the
 * printer. Should help prevent overflowing the printer's input buffer, provided
 * blocksize is sufficiently small (1024 should be safe). It's a totally kludged
 * up routine that should ONLY be used if you have constant transmission problems.
 * There's really no way it will be able to drive a printer much faster that about
 * six pages a minute, even for the simplest jobs. Get it by using the -S option.
 *
 */


    while ( readblock(fd_in) )

	switch ( getstatus(0) )  {

	    case WAITING:
		    writeblock(blocksize);
		    break;

	    case BUSY:
	    case IDLE:
	    case PRINTING:
		    writeblock(30);
		    break;

	    case NOSTATUS:
	    case UNKNOWN:
		    break;

	    case PRINTERERROR:
		    sleep(30);
		    break;

	    case ERROR:
		    fprintf(stderr, "%s", mesg);	/* for csw */
		    error(FATAL, "PostScript Error");
		    break;

	    case FLUSHING:
		    error(FATAL, "Flushing Job");
		    break;

	    case DISCONNECT:
		    error(FATAL, "Disconnected - printer may be offline");
		    break;

	    default:
		    sleep(2);
		    break;

	}   /* End switch */

}   /* End of send */


/*****************************************************************************/


static writeblock(num)


    int		num;			/* most bytes we'll write */


{


    int		count;			/* bytes successfully written */


/*
 *
 * Called from send() when it's OK to send the next block to the printer. head
 * is adjusted after the write, and the number of bytes that were successfully
 * written is returned to the caller.
 *
 */


    if ( num > tail - head )
	num = tail - head;

    if ( (count = write(ttyo, &block[head], num)) == -1 )
	error(FATAL, "error writing to %s", line);
    else if ( count == 0 )
	error(FATAL, "printer appears to be offline");

    head += count;
    return(count);

}   /* End of writeblock */


/*****************************************************************************/

