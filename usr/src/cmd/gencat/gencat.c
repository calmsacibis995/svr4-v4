/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)gencat:gencat.c	1.2.1.3"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <nl_types.h>
#include "gencat.h"

/*
 * gencat : take a message source file and produce a message catalog.
 * usage : gencat catfile [msgfile]
 * if msgfile is not specified, read from standard input.
 * Several message file can be specified
 */
char msg_buf[NL_TEXTMAX];
struct cat_set *sets = 0;
int curset = 1;
int list = 0;
int svr_format = 0;
int malloc_format = 0;
int sco_format = 0;

FILE *tempfile;

static int no_output = 0;


static int skip_blanks();
static void cat_dump();
static void cat_msg_build();
static void copy();
static void usage();

main(argc, argv)
  int argc;
  char **argv;
{
  char catname[MAXNAMLEN];
  char ch;
  FILE *fd;
  register int i;
  int cat_exists;
  
  /*
   * No arguments at all : give usage
   */
  if (argc == 1)
    usage();
    
  /*
   * Create tempfile
   */
  if ((tempfile = tmpfile()) == NULL){
    perror("tempfile");
    exit(1);
  }
  
	/*  Process arguments  */
	while ((ch = getopt(argc, argv, "lmf:")) != EOF)
	{
		switch(ch) {
			case 'l':
				list = 1;
				break;

			case 'm':
				if (svr_format || malloc_format || sco_format)
					usage();

				malloc_format = 1;
				break;

			case 'f':
				if (svr_format || malloc_format || sco_format)
					usage();

				if (strcmp(optarg, "m") == 0)
					malloc_format = 1;
				else if (strcmp(optarg, "SVR4") == 0)
					svr_format = 1;
				else if (strcmp(optarg, "sco") == 0)
					sco_format = 1;
				else
					usage();

				break;

			default:
				usage();
		}
	}

	/*  Check for environment variable to set default output mode
	 *  if no mode specified
	 */
	if (malloc_format == 0 &&
	    svr_format == 0 &&
		sco_format == 0 &&
	    getenv("SCOMPAT") != NULL)
		sco_format = 1;

	argv += optind;
	argc -= optind;
  /*
   * Check catfile name : if file exists, read it.
   */
  strcpy(catname,*argv);
  
  /*
   * use an existing catalog
   */
  if (cat_build(catname))
    cat_exists = 1;
  else
    cat_exists = 0;

  /*
   * Open msgfile(s) or use stdin and call handling proc
   */
  if (argc == 1)
    cat_msg_build("stdin", stdin);
  else {
    for (i = 1 ; i < argc ; i++){
      if ((fd = fopen(argv[i], "r")) == 0){
        /*
         * Cannot open file for reading
         */
        perror(argv[i]);
        continue;
      }
      if (argc > 2)
        fprintf(stdout, "%s:\n", argv[i]);
      cat_msg_build(argv[i], fd);
      fclose(fd);
    }
  }

  if (!no_output){
    /*
     * Work is done. Time now to open catfile (for writing) and to dump
     * the result
     */

	if (list)
		printf("Messages written to catalogue\n\n");

    if (malloc_format) 
	cat_dump(catname);
	else if (sco_format)
	cat_sco_dump(catname);
    else 
	cat_mmp_dump(catname);
  } else
    fprintf(stderr, "%s not %s\n", catname, cat_exists ? "updated":"created");

  exit(0);
}

void
usage()
{
  fprintf(stderr, "Usage: gencat [-l] [-m|-f <format>] catfile [msgfile ...]\n");
  exit(0);
}

/*
 * Scan message file and complete the tables
 */
static
void
cat_msg_build(filename, fd)
  char *filename;
  FILE *fd;
{
  register int i, j;
  char quotechar = 0;
  char c;
  int msg_nr = 0;
  int msg_len;
  char linebuf[BUFSIZ];

  /*
   * always a set NL_SETD
   */
  curset=NL_SETD;
  add_set(curset);
  
  while (fgets(linebuf, BUFSIZ, fd) != 0){


    if ((i = skip_blanks(linebuf, 0)) == -1)
      continue;

    if (linebuf[i] == '$'){
      i += 1;
      /*
       * Handle commands or comments
       */
      if (strncmp(&linebuf[i], CMD_SET, CMD_SET_LEN) == 0){
        i += CMD_SET_LEN;

        /*
         * Change current set
         */
        if ((i = skip_blanks(linebuf, i)) == -1){
          fprintf(stderr, "Incomplete set command -- Ignored\n");
          continue;
        }
        add_set(atoi(&linebuf[i]));

	/*  This issues a warning message if there is text after
	 *  the set number.  This is so SCO message source files that
	 *  use the undocumented extensions warn the user.
	 */
	while (isdigit(linebuf[i]) || isspace(linebuf[i]))
		++i;

	if (linebuf[i] != NULL)
		fprintf(stderr, "Unexpected text after set number - Ignored\n");

        continue;
      }
      if (strncmp(&linebuf[i], CMD_DELSET, CMD_DELSET_LEN) == 0){
  i += CMD_DELSET_LEN;

        /*
         * Delete named set
         */
        if ((i = skip_blanks(linebuf, i)) == -1){
          fprintf(stderr, "Incomplete delset command -- Ignored\n");
          continue;
        }
        del_set(atoi(&linebuf[i]));
        continue;
      }
      if (strncmp(&linebuf[i], CMD_QUOTE, CMD_QUOTE_LEN) == 0){
        i += CMD_QUOTE_LEN;

        /*
         * Change quote character
         */
        if ((i = skip_blanks(linebuf, i)) == -1)
          quotechar = 0;
        else
          quotechar = linebuf[i];
        continue;
      }
      /*
       * Everything else is a comment
       */
      continue;
    }
    if (isdigit(linebuf[i])){
      msg_nr = 0;
      /*
       * A message line
       */
      while(isdigit(c = linebuf[i])){
        msg_nr *= 10;
        msg_nr += c - '0';
        i++;
      }
      j = i;
      if (linebuf[i] == '\n')
        del_msg(curset, msg_nr);
      else
      if ((i = skip_blanks(linebuf, i)) == -1)
	add_msg(curset,msg_nr,1,"");
      else {
        if ((msg_len = msg_conv(filename, curset, msg_nr, fd, linebuf + j,
        BUFSIZ, msg_buf, NL_TEXTMAX, quotechar)) == -1){
    no_output = 1;
          continue;
  }
        add_msg(curset, msg_nr, msg_len + 1, msg_buf);
      }
      continue;
    }
    
    /*
     * Everything else is unexpected
     */
    fprintf(stderr, "Unexpected line -- Skipped\n");
  }
}

/*
 * Skip blanks in a line buffer
 */
static
int
skip_blanks(linebuf, i)
  char *linebuf;
  int i;
{
  while (linebuf[i] && isspace(linebuf[i]) && !iscntrl(linebuf[i]))
    i++;
  if (!linebuf[i] || linebuf[i] == '\n')
    return -1;
  return i;
}

/*
 * Dump the internal structure into the catfile.
 */
static
void
cat_dump(catalog)
char *catalog;
{
  FILE *fd;
  FILE *f_shdr, *f_mhdr, *f_msg;
  struct cat_set *p_sets;
  struct cat_msg *p_msg;
  struct cat_hdr hdr;
  struct cat_set_hdr shdr;
  struct cat_msg_hdr mhdr;
  int msg_len;
  int nmsg = 0;
  
  if ((fd = fopen(catalog,"w")) == NULL) {
    fprintf(stderr, "cat_dump : Cannot create catalog %s\n",catalog);
    fatal();
  }
    
  if ((f_shdr = tmpfile()) == NULL || (f_mhdr = tmpfile()) == NULL ||
      (f_msg = tmpfile()) == NULL){
    fprintf(stderr, "cat_dump : Cannot create temp files\n");
    fatal();
  }
  
  p_sets = sets;
  hdr.hdr_set_nr = 0;
  hdr.hdr_magic = CAT_MAGIC;
  while (p_sets != 0){
    hdr.hdr_set_nr++;
    p_msg = p_sets->set_msg;
    
    /*
     * Keep offset in shdr temp file to mark the set's begin
     */
    shdr.shdr_msg = nmsg;
    shdr.shdr_msg_nr = 0;
    shdr.shdr_set_nr = p_sets->set_nr;
    while (p_msg != 0){
      shdr.shdr_msg_nr++;
      nmsg++;
      msg_len = p_msg->msg_len;
      /*
       * Get message from main temp file
       */
      if (fseek(tempfile, p_msg->msg_off, 0) != 0){
        fprintf(stderr, "cat_dump : Seek error in temp file\n");
        fatal();
      }
      if (fread(msg_buf, 1, msg_len, tempfile) != msg_len){
        fprintf(stderr, "cat_dump : Read error in temp file\n");
        fatal();
      }
      
      if (list)
        printf("Set %d,Message %d,Length %d\n%.*s\n*\n",
          p_sets->set_nr, p_msg->msg_nr, p_msg->msg_len,
          p_msg->msg_len, msg_buf);

      /*
       * Put it in the messages temp file and keep offset
       */
      mhdr.msg_ptr = (int)ftell(f_msg);
      mhdr.msg_len = msg_len;
      mhdr.msg_nr  = p_msg->msg_nr;
      if (fwrite(msg_buf, 1, msg_len, f_msg) != msg_len){
        fprintf(stderr, "cat_dump : Write error in msg temp file\n");
        fatal();
      }

      /*
       * Put message header
       */
      if (fwrite((char *)&mhdr, sizeof(mhdr), 1, f_mhdr) != 1){
        fprintf(stderr, "cat_dump : Write error in mhdr temp file\n");
        exit(1);
      }
      p_msg = p_msg->msg_next;
    }
    
    /*
     * Put set hdr
     */
    if (fwrite((char *)&shdr, sizeof(shdr), 1, f_shdr) != 1){
      fprintf(stderr, "cat_dump : write error in shdr temp file\n");
      fatal();
    }
    p_sets = p_sets->set_next;
  }
  
  /*
   * Fill file header
   */
  hdr.hdr_mem = ftell(f_shdr) + ftell(f_mhdr) + ftell(f_msg);
  hdr.hdr_off_msg_hdr = ftell(f_shdr);
  hdr.hdr_off_msg = hdr.hdr_off_msg_hdr + ftell(f_mhdr);

  /*
   * Generate catfile
   */
  if (fwrite((char *)&hdr, sizeof (hdr), 1, fd) != 1){
    fprintf(stderr, "cat_dump : hdr write error in catfile\n");
    fatal();
  }
  
  copy(fd, f_shdr);
  copy(fd, f_mhdr);
  copy(fd, f_msg);
  fclose(f_shdr);
  fclose(f_mhdr);
  fclose(f_msg);
}

static char copybuf[BUFSIZ];

static
void
copy(dst, src)
  FILE *dst, *src;
{
  int n;
  
  rewind(src);
  
  while ((n = fread(copybuf, 1, BUFSIZ, src)) > 0){
    if (fwrite(copybuf, 1, n, dst) != n){
      fprintf(stderr, "copy : Write error\n");
      fatal();
    }
  }
}

void
fatal()
{
  exit(1);
}
