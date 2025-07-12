/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)eac:maplocale/maplocale.h	1.2"

#define SUCCESS 0		/*  Coversion completed okay  */
#define ERROR 1			/*  Conversion failed  */

#define HASH '#'

#define MODES 0666		/*  Modes for files  */
#define DMODES 0777		/*  Modes for directories  */

#define STR_SIZE 60		/*  General string length  */
#define SHRT_STR 20		/*  Short general string length  */

#define EAC_CONV_ARG "XENIX"		/*  Argument for EAC locale conversion  */

#define CS_SEP '.'
#define LANG_SEP '_'

#define SYSV_LOC_DIR "/usr/lib/locale/"	/*  SVR4 locale directory  */
#define EAC_LOC_DIR "/usr/lib/lang"		/*  EAC locale directory  */
#define EAC_DEF_FILE "/etc/default/lang"		/*  Default file for names  */

#define EAC_LOC_POS 5

/*  Magic numbers for output files  (EAC)  */
#define	MN_CTYPE	1
#define	MN_NUMERIC	2
#define	MN_TIME		3
#define	MN_COLLATE	4
#define	MN_MESSAGES	5
#define	MN_MONETARY	6

/*
 *  #defines for character classifications
 */

#define CTYPE "ctype"
#define CTYPE_TBL_SIZE 257
#define SYSV_CU_OFFST 258

/*
 *  #defines and structs for numeric formating
 */

#define NUMERIC "numeric"

/*  Structure used for numeric formating information  */
struct _lct_numeric {
	char decimal;
	char thousands;
};

/*
 *  #defines and structs for date/time formating
 */

#define TIME "time"
#define N_DT_FIELDS 43		/*  Number of fields  */

struct _lct_time {
	short	date_fmt;		/*  %m/%d/%y  */
	short	time_fmt;		/*  %H:%M:%S  */
	short	f_noon;			/*  AM  */
	short	a_noon;			/*  PM  */
	short	d_t_fmt;		/*  %a %b %d %X %Z %Y  */
	short	day[7];			/*  Sunday, etc.  */
	short	abday[7];		/*  Sun, etc.  */
	short	mon[12];		/*  January, etc.  */
	short	abmon[12];		/*  Jan, etc.  */
	char	strings[512];	/*  308 bytes used  */
};

/*
 *  #defines and structs for collation data
 */

#define COLLATE "collate"
#define LC_COLL "LC_COLLATE"
#define COLL_TBL_SIZE 256	/*  Size of collation table  */

/*  These definitions come from SCO.  The dbl structure has not
 *  been implemented in their coltbl code, but is used by strxfrm.
 */

#define BITMAP "\001\002\004\010\020\040\100\200"
#define	MAXDBL	64

#define BM_FIRST lct_collate.bm_first
#define BM_DIPH  lct_collate.bm_diph

#define SET_FIRST(c) BM_FIRST[(c>>03)&037] |= BITMAP[c & 07]
#define SET_DIPH(c) BM_DIPH[(c>>03)&037] |= BITMAP[c & 07]

struct size {
	short siz_coll;		/*  Size of collation table  */
	short siz_dbl;		/*  Size of double char table  */
};

struct coll {
	unsigned char prim;		/*  Primary weight of character  */
	unsigned char sec;		/*  Secondary weight of character  */
};

struct dbl {
	short chars;	/*  The two char sequence to be collated  */
	short index;	/*  Index to the coll struct entry  */
};

static struct _lct_collate {
	struct size size;
	unsigned char bm_first[32];		/*  Bit map for 1st of 2 char sequence  */
	unsigned char bm_diph[32];		/*  Bit map for 2-to-1 mappings  */
	struct coll coll[256 + MAXDBL];	/*  Collation table  */
	struct dbl dbl[MAXDBL];			/*  Double character table  */
};

/*  These definitions are for the SVR4 LC_COLLATE data
 */

#define FOLLOW_ON 1		/*  This is the first of an n char sequence  */

/* database header */
typedef struct {
	long	coll_offst;	/* offset of collation table from beg. of file */
	long	sub_cnt;	/* number of substitution table entries */
	long	sub_offst;	/* offset of substitution table from beg. of file */
	long	str_offst;	/* offset of substitution strings from beg. of file*/
	long	flags;		/* allows future flexibility in database */
} hd;

/* collation table entry in database */
typedef struct {
	unsigned char		ch;	/* character or number of followers */
	unsigned char		pwt;	/* primary weight */
	unsigned char		swt;	/* secondary weight */
	unsigned char		ns;	/* index of follower state list */
} xnd;

/* subsitution table entry in database */
typedef struct {
	char	*exp;		/*  String to be replaced  */
	long	explen;		/*  Length of exp  */
	char	*repl;		/*  String to replace with  */
} subtent;

/*
 *  #defines and structs for yes/no messages
 */

#define MESSAGES "messages"	/*  Name of output file  */
#define	MESMAX 8		/*  Maximum length of the message string  */

/*  Structure used for yes/no messages  */
struct _lct_messages {
	char yesstr[MESMAX];
	char nostr[MESMAX];
};

/*
 *  #defines for currency formating
 */

#define CURRENCY "currency"
#define CRNCYMAX 8

/*  Functions that don't return anything  */
static void cleanup();
static void conv_eac_locale();
static void error();
static void path_create();
static void eac_map_collate();
static void eac_map_ctype();
static void eac_map_currency();
static void eac_map_date();
static void eac_map_messages();
static void eac_map_numeric();
static void update_default();
static void usage();

/*  These are to keep lint happy  */
unsigned int strlen();
void *malloc();
