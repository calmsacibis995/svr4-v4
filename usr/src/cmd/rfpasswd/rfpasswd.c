/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rfpasswd:rfpasswd.c	1.9.5.1"


#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <nserve.h>
#include <time.h>
#include <errno.h>

#define ERROR(str)	fprintf(stderr,"%s: %s\n", argv[0], str);
#define	MINLENGTH	6
#define	NETSPEC		"/etc/rfs/netspec"

static char u_newpass[10]; /* encrypted new password */
char *getdomain();
char *strtok();
char *tp;
char tpmaster[BUFSIZ];
extern char *Bypass;

main( argc, argv )
int   argc;
char *argv[];
{
	char *rtn;

	char oldpass[10];
	char *newpass;
	char *p1;
	char *mach_name;
	char *netspec;
	char passfile[BUFSIZ];
	char prompt[BUFSIZ];

	struct utsname utname;

	char *getnewpass();
	char *getpass(), *crypt(), *malloc();
	FILE *nfp;
	char *tname[12];
	int  i,j;

	int  rec;

	if (argc != 1) {
		ERROR("extra arguments given");
		ERROR("usage: rfpasswd");
		exit(1);
	}

	if (geteuid() != 0) {
		ERROR("must be super-user");
		exit(1);
	}

	if (uname(&utname) == -1) {
		ERROR("cannot get machine name");
		exit(1);
	}

	/*
	 *	Determine if RFS is running and exit if not.
	 */

	if (rfs_up() != 0) {
		if (errno == ENONET) {
			ERROR("RFS is not running");
		} else {
			perror("rfpasswd");
		}
		exit(1);
	}

	/*
	 *	Get the network specification to change passwd for each tp.
	 */

	if ((netspec = malloc(BUFSIZ + 1)) == NULL) {
		ERROR("malloc for netspec failed");
		exit(1);
	}

	if (((nfp = fopen(NETSPEC, "r")) == NULL)
	|| (fgets(netspec,BUFSIZ,nfp) == NULL)) {
		ERROR("network specification not set");
		exit(1);
	}
	fclose(nfp);
	for (i=0; tname[i] = strtok(netspec, ",") ; i++) 
		netspec=NULL;
	i--;	

	for (j=0; j <= i; j++) {
		Bypass = tp = tname[j];
		if (!really_primary()) 
			exit(1);

		mach_name = utname.nodename;

		sprintf(prompt, "rfpasswd: Please enter old password on %s:", tp);
		strncpy(oldpass, getpass(prompt), sizeof(oldpass));

		if ((newpass = getnewpass(oldpass)) == NULL) 
			exit(1);

		if (ns_sendpass(mach_name, oldpass, newpass) == FAILURE) {
			ERROR("error in verifying password");
			nserror(argv[0]);
			exit(1);
		}

		/*
	 	*	Replace the password on this machine...
	 	*/
	
		sprintf (passfile, PASSFILE, tp);
		if ((rec = creat(passfile, S_IRUSR|S_IWUSR)) < 0) { 
			ERROR("warning: cannot create password file");
			exit(0);
		}
		if (write(rec, u_newpass, strlen(u_newpass)) < 0) {
			ERROR("warning cannot write password");
			unlink(passfile);
		}
	}
	exit(0);
}

/*
 *	really_primary() checks to make sure that the machine
 *	listed as primary in rfmaster is acting as primary
 *	(i.e., a secondary is not acting as primary)
 */

static
really_primary()
{
	struct nssend send;
	struct nssend *rtn;
	struct nssend *ns_getblock();
	char   key[MAXDNAME+1];

	FILE *fp;
	char buf[BUFSIZ];
	char morebuf[BUFSIZ];
	char	*dname = NULL, *cmd = NULL, *mname = NULL;
	char	*ptr;
	int i, size;
	int found = 0;
	int line = 0;
	char *thisdom = getdomain();

	sprintf(tpmaster, TPNETMASTER, tp);
	if ((fp = fopen(tpmaster, "r")) == NULL) {
		fprintf(stderr, "rfpasswd: cannot open master file <%s>\n", tpmaster);
		return(0);
	}

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		line ++;
		size = strlen(buf);
		while (buf[size-2] == '\\' && buf[size-1] == '\n') {
			buf[size-2] = '\0';
			if (fgets(morebuf,  sizeof(morebuf), fp) == NULL) {
				break;
			}
			if ((size = (size - 2 + strlen(morebuf))) > BUFSIZ) {
				fprintf(stderr, "rfpasswd: warning: line %d of rfmaster too long\n", line);
				break;
			}
			strcat(buf, morebuf);
		}
		if ((dname = strtok(buf, " \t")) == NULL)
			continue;
		if ((cmd  = strtok(NULL, " \t")) == NULL)
			continue;
		if ((mname = strtok(NULL, " \t\n")) == NULL)
			continue;

		for (i = 0; cmd[i] != '\0'; i ++)
			cmd[i] = tolower(cmd[i]);

		if ((strcmp(cmd, "p") == 0 || strcmp(cmd, "soa") == 0)
		  && (strcmp(dname, thisdom) == 0)) {
			found = 1;
			break;
		}
	}

	if (found == 0) {
		fprintf(stderr, "rfpasswd: cannot obtain the name of the primary name server from %s\n", tpmaster);
		return(0);
	}

	/*
	 *	Initialize the information structure to send to the
	 *	name server.
	 */

	sprintf(key, "%s%s", thisdom, ".");
	send.ns_code = NS_FINDP;
	send.ns_name = key;
	send.ns_flag = 0;
	send.ns_type = 0;
	send.ns_desc = NULL;
	send.ns_path = NULL;
	send.ns_addr = NULL;
	send.ns_mach = NULL;

	/*
	 *	Get the name of the primary from the name server.
	 */

	if ((rtn = ns_getblock(&send)) != (struct nssend *)NULL) {
		if (strcmp(*rtn->ns_mach, mname) == 0) {
			return(1);
		} else {
			fprintf(stderr, "rfpasswd: cannot change the password while %s is the primary name server\n", *rtn->ns_mach);
			return(0);
		}
	}

	fprintf(stderr, "rfpasswd: cannot obtain the name of the primary name server for domain <%s>\n", thisdom == NULL? "unknown": thisdom);
	return(0);
}

/*
 *	getnewpass()
 *	returns an encrypted password obtained from the user.
 *	This code is mostly taken from passwd.c, since all of
 *	the checks are the same.
 */

char *
getnewpass(oldpw)
char	*oldpw;
{
	char	*getpass();
	char	buf[10];
	char	saltc[2];
	char	*p, *o; 
	char	*pw;
	char	prompt[50];
	long	salt;
	int	pwlen;
	int	opwlen;
	int	flags;
	int	len;
	int	diff;
	int	c, i;
	int	insist = 0;
	int	typerror = 0;

	char	*crypt();

	for (;;) {
		if (insist >= 3) {
			fprintf(stderr, "too many failures - try again\n");
			return((char *)NULL);
		}
		if (typerror >= 3) {
			fprintf(stderr, "too many tries - try again\n");
			return((char *)NULL);
		}

		sprintf(prompt, "rfpasswd: Enter new password on %s:", tp);
		strncpy(u_newpass, getpass(prompt), sizeof(u_newpass));
		pwlen = strlen(u_newpass);

		/*
	 	 *	Make sure new password is long enough
	 	 */

		if (pwlen < MINLENGTH) { 
			fprintf(stderr, "Password is too short - must be at least %d characters\n", MINLENGTH);
			insist ++;
			continue;
		}

		/*
		 *	Insure passwords contain at least two alpha
		 *	characters and one numeric or special character
		 */               

		flags = 0;
		p = u_newpass;
		while (c = *p++) {
			if (isalpha(c)) {
				if (flags & 1)
					flags |= 2;
				else
					flags |= 1;
			} else 
				flags |= 4;
		}

		if (flags != 7) {
			fprintf(stderr,"Password must contain at least two alphabetic characters and\n");
			fprintf(stderr,"at least one numeric or special character.\n");
			insist ++;
			continue;
		}

		/*
		 *	If old password is present, check to make sure
		 *	that old and new differ by at least 3 positions.
		 */

		if (*oldpw != '\0') {
			p = u_newpass;
			o = oldpw;
			opwlen = strlen(oldpw);
			if (pwlen >= opwlen) {
				len  = opwlen;
				diff = pwlen - opwlen;
			} else {
				len  = pwlen;
				diff = opwlen - pwlen;
			}
			for (i = 1; i <= len; i++)
				if (*p++ != *o++)
					diff++;
			if (diff < 3) {
				fprintf(stderr, "Passwords must differ by at least 3 positions\n");
				insist ++;
				continue;
			}
		}

		/*
		 *	Insure password was typed correctly, user gets
		 *	three chances
		 */

		sprintf(prompt,"rfpasswd: Re-enter new password on %s:",tp);
		strcpy(buf, getpass(prompt));
		if (strcmp(buf, u_newpass)) {
			fprintf(stderr, "They don't match; try again.\n");
			typerror ++;
			continue;
		}

		/*
		 *	Construct salt, then encrypt the new password
		 */

		time((time_t *)&salt);
		salt += (long)getpid();

		saltc[0] = salt & 077;
		saltc[1] = (salt >> 6) & 077;
		for (i=0; i<2; i++) {
			c = saltc[i] + '.';
			if (c>'9') c += 7;
			if (c>'Z') c += 6;
			saltc[i] = c;
		}
		pw = crypt(u_newpass, saltc);
		return(pw);
	}
}
