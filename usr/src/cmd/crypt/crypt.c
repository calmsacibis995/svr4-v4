/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)crypt:crypt.c	1.9.1.1"
/*
 *	A one-rotor machine designed along the lines of Enigma
 *	but considerably trivialized.
 */

#define ECHO 010
#include <stdio.h>
#include <stdlib.h>
#define ROTORSZ 256
#define MASK 0377
char	t1[ROTORSZ];
char	t2[ROTORSZ];
char	t3[ROTORSZ];

setup(pw)
char *pw;
{
	int ic, i, k, temp;
	unsigned random;
	char buf[13];
	long seed;

	strncpy(buf, pw, 8);
	buf[8] = buf[0];
	buf[9] = buf[1];
	strncpy(buf, crypt(buf, &buf[8]), 13);
	seed = 123;
	for (i=0; i<13; i++)
		seed = seed*buf[i] + i;
	for(i=0;i<ROTORSZ;i++) {
		t1[i] = i;
		t3[i] = 0;
	}
	for(i=0;i<ROTORSZ;i++) {
		seed = 5*seed + buf[i%13];
		random = seed % 65521;
		k = ROTORSZ-1 - i;
		ic = (random&MASK)%(k+1);
		random >>= 8;
		temp = t1[k];
		t1[k] = t1[ic];
		t1[ic] = temp;
		if(t3[k]!=0) continue;
		ic = (random&MASK) % k;
		while(t3[ic]!=0) ic = (ic+1) % k;
		t3[k] = ic;
		t3[ic] = k;
	}
	for(i=0;i<ROTORSZ;i++)
		t2[t1[i]&MASK] = i;
}

main(argc, argv)
char *argv[];
{
	extern int optind;
	register char *p1;
	register i, n1, n2, nchar;
	int c;
	struct { 
		long offset;
		unsigned int count;
	} header;
	int pflag = 0;
	int kflag = 0;
	char *buf;
	char key[8];
	char *keyvar = "CrYpTkEy=XXXXXXXX";

	if (argc < 2){
		if((buf = (char *)getpass("Enter key:")) == NULL) {
			fprintf(stderr, "Cannot open /dev/tty\n");
			exit(1);
		}
		setup(buf);
	}
	else {
		while ((c = getopt(argc, argv, "pk")) != EOF)
			switch (c) {
			case 'p':
			/* notify editor that exec has succeeded */
				if(write(1, "y", 1) != 1)
					exit(1);
				if(read(0, key, 8) != 8)
					exit(1);
				setup(key);
				pflag = 1;
				break;
			case 'k':
				strncpy(key, getenv("CrYpTkEy"),8);
				setup(key);
				kflag = 1;
				break;
			case '?':
				fprintf(stderr,"usage: crypt [ -k ] [ key]\n");
				exit(2);
			}
		if(pflag == 0 && kflag == 0) {
			strncpy(keyvar+9,argv[optind],8);
			putenv(keyvar);
			execlp("crypt","crypt","-k",0);
		}
	}	
	if(pflag)
		while(1) {
			if((nchar = read(0, (char *)&header, sizeof(header))) != sizeof(header))
				exit(nchar);
			n1 = (int) (header.offset&MASK);
			n2 = (int) ((header.offset >> 8) &MASK);
			nchar = header.count;
			buf = (char *)malloc(nchar);
			p1 = buf;
			if(read(0, buf, nchar) != nchar)
				exit(1);
			while(nchar--) {
				*p1 = t2[(t3[(t1[(*p1+n1)&MASK]+n2)&MASK]-n2)&MASK]-n1;
				n1++;
				if(n1 == ROTORSZ) {
					n1 = 0;
					n2++;
					if(n2 == ROTORSZ) n2 = 0;
				}
				p1++;
			}
			nchar = header.count;
			if(write(1, buf, nchar) != nchar)
				exit(1);
			free(buf);
		}
		
	n1 = 0;
	n2 = 0;

	while((i=getchar()) >=0) {
		i = t2[(t3[(t1[(i+n1)&MASK]+n2)&MASK]-n2)&MASK]-n1;
		putchar(i);
		n1++;
		if(n1==ROTORSZ) {
			n1 = 0;
			n2++;
			if(n2==ROTORSZ) n2 = 0;
		}
	}
	exit(0);
}
