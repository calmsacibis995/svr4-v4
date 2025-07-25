/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rfs.cmds:dfmounts/dfmounts.c	1.9.4.3"
#include <stdio.h>
#include <sys/fcntl.h> 
#include <sys/types.h>
#include <sys/nserve.h>
#include <sys/stat.h>
#include <sys/rf_sys.h>
#include <sys/utsname.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define	SHTAB	"/etc/dfs/sharetab"

extern int		open();
extern int		rfsys();

static void		read_advtab();
static void		pr_info();
static void		pr_list();
static char		*pathname();
static void		*Malloc();

static char		*pgm;
static struct utsname	myname;
static struct client	*clientp;	/* for kernel client info */
static char		*advtabp;	/* for info from SHTAB */
static char		*hdr =
		"RESOURCE       SERVER   PATH                 CLIENTS\n";

typedef	char	rf_r_name_t[RFS_NMSZ];

int
main(argc, argv)
	int		argc;
	char		**argv;
{
	int		i;
	int		maxmntsperser;
	int		ncuradvs;
	int		maxrsrcs;
	int		numrsrcs;
	rf_r_name_t 	*k_rsrcp;		/* for kernel resource names */
	int		headerflag	= 1;	/* default is to print header */
	int		errflag		= 0;
	int		c;
	extern int	optind;
	extern char	*optarg;
	char		**rlp;
	char		*dummy = NULL;

#ifdef   OLDSEC
	if (geteuid() != 0) {
		(void)fprintf(stderr, "%s: must be super-user\n", argv[0]);
		exit(1);
	}
#endif /*OLDSEC*/

	pgm = argv[0];

	while ((c = getopt(argc, argv, "h")) != EOF) {
		switch (c) {
		case 'h':	headerflag = 0;
				break;
		case 'F':
				if(strcmp(optarg, "rfs") != 0)
					errflag = 1;
				break;
		case '?':	errflag = 1;
		}
	}
	if (errflag){
		(void)fprintf(stderr,
			"%s: usage:  %s [-F rfs] [-h] [resource_name]...\n",
			pgm, pgm);
		exit(1);
	}
			
	/* initialize globals */
	read_advtab();
	if (uname(&myname) == -1)
		(void)strcpy(myname.nodename, "?");
	if ((maxmntsperser = rfsys(RF_TUNEABLE, T_NSRMOUNT, dummy)) <= 0) {
		perror(pgm);
		exit(1);
	}
	clientp = Malloc(maxmntsperser * sizeof(struct client));

	if (headerflag) 
		(void)printf(hdr);

	if (optind < argc) {	/* print info on resources from argv */
		pr_list(&argv[optind], argc - optind);
		exit(0);
	}

	/* Print info on all known resources advertised and/or mounted  */

	/* rfsys below returns current number, not maximum.
	 * There has been no maximum on advertisement since 5.3.2
	 */
	if ((ncuradvs = rfsys(RF_TUNEABLE, T_NADVERTISE, dummy)) < 0) {
		perror(pgm);
		exit(1);
	}
	if( (maxrsrcs = ncuradvs + maxmntsperser) == 0)
		exit(0);

	k_rsrcp = Malloc( maxrsrcs * sizeof(rf_r_name_t));

	/* hopefully, there are not many advertisements between the above
	 * malloc and the rfsys below
	 */
	if ((numrsrcs = rfsys(RF_RESOURCES, k_rsrcp, dummy)) < 0) {
		perror(pgm);
		exit(1);
	}
	if(numrsrcs == 0)
		exit(0);
	rlp = Malloc(numrsrcs * sizeof(char *));
	for (i = 0; i < numrsrcs; i++)
		rlp[i] = k_rsrcp[i];

	pr_list(rlp, numrsrcs);

	exit(0);
	/* NOTREACHED */
}

void
pr_list(rlist, rcnt)
	char	**rlist;
	int	rcnt;
{
	int	i;
	int	numclients;	

	for (i = 0; i < rcnt; i++) {
		if ((numclients = rfsys(RF_CLIENTS, rlist[i], clientp)) < 0) {
			perror(pgm);
			exit(1);
		}
		else
			pr_info(rlist[i], numclients);
	}
}

void
pr_info(res, numclients)
	char	*res;
	int	numclients;
{
	int i;

	(void)printf("%-14s %-8s %-20s", res, myname.nodename, pathname(res));
	if (numclients == 0) {
		(void)printf("\n");
		return;
	}
	for (i = 0; i < numclients; i++)
		(void)printf(i == 0 ? " %-s" : ",%s", clientp[i].cl_node);
	(void)printf("\n");
}

void
read_advtab()
{
	int		fd;
	struct stat	sbuf;

	if( stat(SHTAB, &sbuf) == -1
	|| (advtabp = malloc((size_t)(sbuf.st_size + 1))) == NULL
	|| (fd = open(SHTAB, O_RDONLY)) == -1
	|| read(fd, advtabp, (unsigned)sbuf.st_size) != sbuf.st_size
	) {
		(void)printf("%s: warning: cannot get information from %s; ",
								pgm, SHTAB);
		(void)printf("pathnames will not be printed\n");
		advtabp = NULL;
	} else {
		*(advtabp + sbuf.st_size) = '\0';
	}
}

#define SHTABMINF	4

char *
pathname(res)
	char	*res;
{
	union	{
		char	*field[SHTABMINF];
		struct	{
			char	*path;
			char	*rname;
			char	*fstype;
			char	*perm;
		} comp;
	}		line;
	char		*fp;
	char		*lp;
	char		*lpn	= NULL;
	char		*fsep	= "\t ";
	char		*lsep	= "\n";
	char		*unk	= "unknown";
	int		i;
	static char	*advcpyp= NULL;

	if (*advtabp == NULL)
		return unk;

	/* strtok is destructive */
	/* make a copy of the advtabp data  so it can be reused */
	if(advcpyp != NULL)
		free(advcpyp);
	advcpyp = Malloc( (size_t)(strlen(advtabp) + 1));
	(void)strcpy(advcpyp, advtabp);


	for (	lp  = strtok(advcpyp,		 lsep);
		lp != NULL;
		lp  = strtok(lpn, lsep)		/* find end of line */
	){
		lpn = lp + strlen(lp) + 1;	/* calc. start of next line */
		line.comp.path	=
		line.comp.rname	=
		line.comp.fstype=
		line.comp.perm	= NULL;

		for (	i = 0,		fp  = strtok(lp,   fsep);
			i<SHTABMINF &&	fp != NULL;
			i++,		fp  = strtok(NULL, fsep)
		){
			line.field[i] = fp;	/* note: this is a union */
		}
		if (line.comp.path   == NULL
		||  line.comp.rname  == NULL
		||  line.comp.fstype == NULL
		||  line.comp.perm   == NULL
		){
			return unk;	/* bad format */
		}

		if ( strcmp(line.comp.fstype, "rfs") == 0
		&&   strcmp(line.comp.rname,   res ) == 0
		) {
			return line.comp.path;
		}
	}
	return unk;
}
void *
Malloc(spcreq)
	size_t	spcreq;
{
	void		*p;

	if( (p = malloc(spcreq)) == NULL){
		(void)fprintf(stderr, "%s:  memory allocation failed\n", pgm);
		exit(1);
	} 
	return p;
}
