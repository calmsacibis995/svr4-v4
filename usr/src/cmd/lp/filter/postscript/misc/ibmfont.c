/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:filter/postscript/misc/ibmfont.c	1.1.2.1"

/*
 *
 * A simple program that converts IBM font files to a format that works on Unix
 * systems.  Essentially all the information needed came from the Adobe paper
 * "Supporting Downloadable PostScript Fonts". To use the program type,
 *
 *	ibmfont font.ibm >font.unix
 *
 * where font.ibm is the font file, exactly as it came over from an IBM PC, and
 * font.unix is equivalent host resident font file usable on Unix systems.
 * 
 */


#include <stdio.h>
#include <signal.h>


#define OFF		0
#define ON		1

#define NON_FATAL	0
#define FATAL		1

#define FALSE		0
#define TRUE		1


char	**argv;				/* global so everyone can use them */
int	argc;

char	*prog_name;

int	x_stat;
int	debug = OFF;
int	ignore = OFF;

FILE	*fp_in = stdin;			/* input */
FILE	*fp_out = stdout;		/* and output files */


/*****************************************************************************/


main(agc, agv)


    int		agc;
    char	*agv[];


{


/*
 *
 * IBM PC to Unix font converter.
 *
 */


    argc = agc;				/* other routines may want them */
    argv = agv;

    prog_name = argv[0];		/* really just for error messages */

    options();				/* first get command line options */
    arguments();			/* then process non-option arguments */

    exit(x_stat);			/* everything probably went OK */

}   /* End of main */


/*****************************************************************************/


options()


{


    int		ch;			/* return value from getopt() */
    char	*names = "DI";

    extern char	*optarg;		/* set by getopt() */
    extern int	optind;


/*
 *
 * Reads and processes the command line options.
 *
 */


    while ( (ch = getopt(argc, argv, names)) != EOF )  {

	switch ( ch )  {

	    case 'D':			/* debug flag */
		    debug = ON;
		    break;

	    case 'I':			/* ignore FATAL errors */
		    ignore = ON;
		    break;

	    case '?':			/* don't understand the option */
		    error(FATAL, "");
		    break;

	    default:			/* don't know what to do for ch */
		    error(FATAL, "missing case for option %c\n", ch);
		    break;

	}   /* End switch */

    }   /* End while */

    argc -= optind;			/* get ready for non-option args */
    argv += optind;

}   /* End of options */


/*****************************************************************************/


arguments()


{


/*
 *
 * Makes sure all the non-option command line arguments are processed. If we get
 * here and there aren't any arguments left, or if '-' is one of the input files
 * we'll process stdin.
 *
 */


    if ( argc < 1 )
	conv();
    else				/* have at least one argument */
	while ( argc > 0 )  {
	    if ( strcmp(*argv, "-") == 0 )
		fp_in = stdin;
	    else if ( (fp_in = fopen(*argv, "r")) == NULL )
		error(FATAL, "can't open %s", *argv);
	    conv();
	    if ( fp_in != stdin )
		fclose(fp_in);
	    argc--;
	    argv++;
	}   /* End while */

}   /* End of arguments */


/*****************************************************************************/


conv()


{


    int		blocksize;
    int		blocktype;
    int		seg;
    long	ftell();


/*
 *
 * Font files on the IBM PC are stored in a compressed binary format. Individual
 * segments in the file are preceeded by a header that looks like,
 *
 *		Byte 1:		128
 *		Byte 2:		segment type (1=ASCII, 2=TOHEX, or 3=EOF)
 *		Bytes 3-6:	length of the segment
 *		Bytes 7 ...	data
 *
 */

    while ( 1 )  {
	seg = ftell(fp_in);
	if ( getc(fp_in) != 128 )
	    error(FATAL, "bad file format");
	blocktype = getc(fp_in);
	blocksize = getint(fp_in);
	if ( debug == ON ) {
	    fprintf(stderr, "blocktype = %d, blocksize = %d\n", blocktype, blocksize);
	    fprintf(stderr, "start=0%o, end=0%o\n", seg, seg+blocksize+6);
	    fprintf(stderr, "start=%d, end=%d\n", seg, seg+blocksize+6);
	}   /* End if */
	switch ( blocktype )  {
	    case 1:
		asciitext(blocksize);
		break;

	    case 2:
		hexdata(blocksize);
		break;

	    case 3:
		return;

	    default:
		error(FATAL, "unknown resource type %d", blocktype);
	}   /* End switch */
    }	/* End while */

}   /* End of conv */


/*****************************************************************************/


asciitext(count)


    int		count;			/* bytes left in the block */


{


    int		ch;
    int		i = 0;


/*
 *
 * Handles type 1 (ie. ASCII text) blocks. Changing carriage returns to newlines
 * is all I've done.
 *
 */


    for ( i = 0; i < count; i++ )  {
	if ( (ch = getc(fp_in)) == '\r' )
	    ch = '\n';
	putc(ch, fp_out);
    }	/* End for */
	
}   /* End of asciitext */


/*****************************************************************************/


hexdata(count)


    int		count;			/* bytes left in the block */


{


    int		i;
    int		n;


/*
 *
 * Reads the next count bytes and converts each byte to hex. Also starts a new
 * line every 80 hex characters.
 *
 */


    for ( i = 0, n = 0; i < count; i++ )  {
	fprintf(fp_out, "%.2X", getc(fp_in));
	if ( (++n % 40) == 0 )
	    putc('\n', fp_out);
    }	/* End for */
	
}   /* End of hexdata */


/*****************************************************************************/


getint()


{


    int		val;


/*
 *
 * Reads the next four bytes into an integer and returns the value to the caller.
 * First two bytes are probably always 0.
 *
 */


    val = getc(fp_in);
    val |= (getc(fp_in) << 8);
    val |= (getc(fp_in) << 16);
    val |= (getc(fp_in) << 24);

fprintf(stderr, "getint: val=0%o val=%d\n", val, val);
    return(val);

}   /* End of getint */ 


/*****************************************************************************/


error(kind, mesg, a1, a2, a3)


    int		kind;			/* FATAL or NON_FATAL error */
    char	*mesg;			/* error message control string */
    unsigned	a1, a2, a3;		/* control string arguments */


{


/*
 *
 * Called when we've run into some kind of program error. *mesg is printed using
 * the control string arguments a?. We'll quit if we're not ignoring errors and
 * kind is FATAL.
 *
 */


    if ( mesg != NULL && *mesg != '\0' )  {
	fprintf(stderr, "%s: ", prog_name);
	fprintf(stderr, mesg, a1, a2, a3);
	putc('\n', stderr);
    }	/* End if */

    if ( kind == FATAL && ignore == OFF )
	exit(x_stat | 01);

}   /* End of error */


/*****************************************************************************/

