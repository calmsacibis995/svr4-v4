/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:pkg_lmf/lmfdump.c	1.1"
/* SCCSID(@(#)lmfdump.c	4.3	LCC)	/* Modified: 09:31:30 8/23/89 */

/*
 *  LMF message file dumper
 */

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include "lmf_int.h"

struct lmf_file_header hdr;

unsigned char buf[4096];
char name_buf[128];
char *name;
int fd;

char *malloc();


main(argc, argv)
int argc;
char **argv;
{
	if (argc == 1) {
		fprintf(stderr, "usage: lmfdump file [file] ...\n");
		exit(1);
	}
	while (--argc)
		process_file(*++argv);
}


process_file(file)
char *file;
{
	if ((fd = open(file, O_RDONLY|O_BINARY)) < 0) {
		fprintf(stderr, "lmfdump: Can't open file %s\n", file);
		return;
	}
	if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr) ||
	    strcmp(hdr.lmfh_magic, "LMF")) {
		fprintf(stderr, "lmfdump: invalid file format: %s\n", file);
		close(fd);
		return;
	}
	name_buf[0] = '\0';
	name = &name_buf[1];
	*name = '\0';
	printf("$domain %s\n\n", hdr.lmfh_base_domain);
	if (hdr.lmfh_base_attr & 0x01)
		process_domain(hdr.lmfh_base_offset, hdr.lmfh_base_length);
	else
		process_message(hdr.lmfh_base_offset, hdr.lmfh_base_length);
	close (fd);
}

process_domain(off, len)
long off;
int len;
{
	char *dom;
	char *namep;
	struct lmf_dirent *de;
	int i, l;

	if ((dom = (char *)malloc(len)) == NULL) {
		fprintf(stderr, "lmfdump: can't allocate domain %s\n", name);
		return;
	}
	namep = &name_buf[strlen(name_buf)];
	lseek(fd, off, 0);
	if (read(fd, dom, len) != len) {
		fprintf(stderr, "lmfdump: read error on domain %s\n", name);
		free(dom);
		return;
	}
	for (i = 0; i < len; i += l) {
		de = (struct lmf_dirent *)&dom[i];
		l = de->lmfd_len;
		sprintf(namep, ".%s", de->lmfd_name);
		if (de->lmfd_attr & LMFA_DOMAIN)
			process_domain(de->lmfd_offset, de->lmfd_length);
		else
			process_message(de->lmfd_offset, de->lmfd_length);
	}
	*namep = '\0';
	free(dom);
}


process_message(off, len)
long off;
int len;
{
	register unsigned char *bp, *ebp;

	bp = buf;
	if (len >= 4096 && (bp = (unsigned char *)malloc(len + 1)) == NULL) {
		fprintf(stderr, "lmfdump: Can't allocate space for message %s\n", name);
		return;
	}
	lseek(fd, off, 0);
	if (read(fd, bp, len) != len) {
		fprintf(stderr, "lmfdump: Error reading message %s\n", name);
		if (len >= 4096)
			free(bp);
		return;
	}
	ebp = &bp[len];
	*ebp = '\0';
	printf("%s\t", name);
	while (bp < ebp) {
		switch (*bp) {
		case '\n': printf(((bp+1) != ebp) ? "\\n\\\n\t" : "\\n"); break;
		case '\r': putchar('\\'); putchar('r'); break;
		case '\t': putchar('\\'); putchar('t'); break;
		case '\v': putchar('\\'); putchar('v'); break;
		case '\b': putchar('\\'); putchar('b'); break;
		case '\f': putchar('\\'); putchar('f'); break;
		case '\\': putchar('\\'); putchar('\\'); break;
		default:
			if (*bp < ' ')
				printf("\\%o", *bp);
			else
				putchar(*bp);
			break;
		}
		bp++;
	}
	putchar('\n');
	if (len >= 4096)
		free(bp);
}
