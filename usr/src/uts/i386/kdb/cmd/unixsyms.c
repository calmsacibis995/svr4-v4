/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kdb:cmd/unixsyms.c	1.3.1.4"

/* In a cross-environment, make sure these headers are for the host system */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>

/* The remaining headers are for the target system. */
#include <libelf.h>

long lseek();

#ifdef __STDC__
#define _VOID	void
#else
#define _VOID	char
#endif

#ifdef __STDC__
Elf32_Addr addsym(Elf *elf, char *scnname, void *p, unsigned int size);
#else
Elf32_Addr addsym();
#endif

/* Special symbol names */
#define SN_SYMTABLE	"symtable"	/* symbol table array */
#define SN_SYMSIZE	"symtab_size"	/* size of symbol table proper */
#define SYMSECTION	".unixsyms"	/* section name for symbol data */

/* Kernel debugger symbol table entry structure */
typedef struct kdbsyment {
	unsigned long	value;
	char		name[4];
} kdbsym_t;

/* Array of pointers to symbol table entries for sorting */
kdbsym_t **symarray;

/* Generated output values */
int symtab_size;		/* symbol table size */
char *symtable;			/* final sorted, collapsed symbol table */

/* Patch locations for special symbols */
long loc_symtable;
long loc_symsize;

int nksyms;			/* number of symbols in output symbol table */

Elf32_Shdr *esections[20];	/* for ELF section headers */
int verbose;			/* verbose output flag */

int sym_cmp();

#define WORD_ROUNDUP(n)	(((n) + sizeof(long) - 1) & ~(sizeof(long) - 1))


main(argc, argv)
int argc;
char **argv;
{
	int fd, ifd, efd;
	off_t icmd_len = 0, ecmd_len = 0;
	int i;
	char *name;
	Elf		*elf_file;
	Elf32_Ehdr	*p_ehdr;	/* elf file header */
	unsigned	data_encoding;	/* byte encoding format in file */
	Elf_Scn		*scn;		/* elf section header	*/
	Elf_Scn		*str_scn, *sym_scn;
	Elf_Data	*str_data;	/* info on str_tab	*/
	Elf_Data	*sym_data;	/* info on symtab	*/
	Elf_Data	src_data, dst_data;
	unsigned long	data_word;
	size_t		str_ndx;
	Elf32_Sym	*esym;		/* pointer to ELF symbol	*/
	size_t		symsize = 0;
	int		nsyms;		/* # symbols in input symbol table */
	Elf32_Addr	symtab_addr;
	kdbsym_t	*sym;		/* pointer to symbol info */
	int		slen;
	char		*symp;
	char		*prog = argv[0];
	int		c;
	extern char	*optarg;
	extern int	optind;

	while ((c = getopt(argc, argv, "?vi:e:")) != EOF) {
		switch (c) {
		case 'v':
			verbose = 1;
			break;
		case 'i':
			if ((ifd = open(optarg, O_RDONLY)) < 0)
				errno = 0;
			else {
				if ((icmd_len = lseek(ifd, 0L, 2)) == -1L)
					fatal(7, "%s: seek error on %s\n",
					      prog, optarg);
				lseek(ifd, 0L, 0);
			}
			break;
		case 'e':
			if ((efd = open(optarg, O_RDONLY)) < 0)
				errno = 0;
			else {
				if ((ecmd_len = lseek(efd, 0L, 2)) == -1L)
					fatal(7, "%s: seek error on %s\n",
					      prog, optarg);
				lseek(efd, 0L, 0);
			}
			break;
		default:
			goto usage;
		}
	}

	if (argc - optind != 1) {
usage:
		fatal(99,
	"%s: usage: unixsyms [-v] [[-i|-e] init-cmd-file] kernel-file\n",
			prog);
	}

	if ((fd = open(argv[optind], O_RDWR)) < 0)
		fatal(10, "%s: cannot open %s\n", prog, argv[optind]);

	if (elf_version(EV_CURRENT) == EV_NONE)
		fatal(11, "%s: ELF library is out of date\n", prog);

	if ((elf_file = elf_begin(fd, ELF_C_RDWR, (Elf *)0)) == 0) {
		fatal(12, "%s: ELF error in elf_begin: %s\n", prog,
				elf_errmsg(elf_errno()));
	}

	/*
	 *	get ELF header
	 */
	if ((p_ehdr = elf32_getehdr( elf_file )) == 0) {
		fatal(13, "%s: problem with ELF header: %s\n", prog,
				elf_errmsg(elf_errno()));
	}
	data_encoding = p_ehdr->e_ident[EI_DATA];

	/*
	 *	load section table
	 */
	i=0;
	scn = 0;
	while(( scn =  elf_nextscn( elf_file,scn )) != 0 ) {
		esections[ ++i ] = elf32_getshdr( scn );
		if( esections[ i ]->sh_type == SHT_STRTAB){
			str_scn = scn;

			str_data = 0;
			if(( str_data = elf_getdata( str_scn, str_data ))== 0 ||
			    str_data->d_size == 0) {
				fatal(10,
				     "%s: could not get string section data.\n",
				      prog);
			}
		}
		else if( esections[ i ]->sh_type == SHT_SYMTAB){
			sym_scn = scn;
			str_ndx = esections[ i ]->sh_link;

			esym = NULL;
			sym_data = 0;
			if ((sym_data = elf_getdata(sym_scn, sym_data)) == 0)
				fatal(10, "%s: no symbol table data.\n", prog);
			esym = (Elf32_Sym *)sym_data->d_buf;
			nsyms = sym_data->d_size / sizeof(Elf32_Sym) - 1;
			esym++;	/* first member not used */
		}
	}

	symarray = (kdbsym_t **)malloc(nsyms * sizeof(kdbsym_t *));
	if (symarray == NULL) {
		fatal(20, "%s: not enough memory to build symbol table.\n",
		      prog);
	}

	/* for each symbol in input file... */

	for (i = 0; i < nsyms; i++, esym++) {

		/* we only care about it if it is external */
		if (ELF32_ST_BIND(esym->st_info) != STB_GLOBAL)
			continue;

		name = elf_strptr( elf_file, str_ndx, (size_t)esym->st_name);

		/* skip symbols that start with '.' */
		if (name[0] == '.')
			continue;

		/* if it's a symbol we'll be patching, remember its location */

		if (strcmp(name, SN_SYMTABLE) == 0) {
			loc_symtable = esections[esym->st_shndx]->sh_offset +
			    esym->st_value - esections[esym->st_shndx]->sh_addr;
		} else if (strcmp(name, SN_SYMSIZE) == 0) {
			loc_symsize = esections[esym->st_shndx]->sh_offset +
			    esym->st_value - esections[esym->st_shndx]->sh_addr;
		}

		/* make a symtable entry for the symbol */

		slen = WORD_ROUNDUP(sizeof(kdbsym_t) + strlen(name) - 3);
		sym = (kdbsym_t *)malloc(slen);
		if (sym == NULL) {
			fatal(20,
			      "%s: not enough memory to build symbol table.\n",
			      prog);
		}
		sym->value = esym->st_value;
		strcpy(sym->name, name);
		symsize += slen;
		symarray[nksyms++] = sym;
	}

	/* make sure we found all the symbols we need to patch */

	if (!loc_symtable)
		fatal(1, "no symbol named '%s' found in %s\n",
		      SN_SYMTABLE, argv[optind]);
	if (!loc_symsize)
		fatal(1, "no symbol named '%s' found in %s\n",
		      SN_SYMSIZE, argv[optind]);

	/* if symtable pointer is null, don't insert symbols */

	lseek(fd, loc_symtable, 0);
	read(fd, (char *)&data_word, sizeof data_word);
	if (data_word == 0)
		fatal(1, "symtable pointer is NULL; no symbols inserted\n");

	/* sort symbol array based on value */

	qsort((char *)symarray, nksyms, sizeof(kdbsym_t *), sym_cmp);

	/* prepare for data conversions */

	src_data.d_version = dst_data.d_version = EV_CURRENT;
	src_data.d_buf = dst_data.d_buf = (char *)&data_word;
	src_data.d_size = dst_data.d_size = sizeof(data_word);
	src_data.d_type = ELF_T_WORD;

	/* construct final symbol table image */

	symtab_size = symsize + 4 * sizeof(long) + icmd_len + ecmd_len + 2;
	symtab_size = WORD_ROUNDUP(symtab_size);

	symtable = malloc(symtab_size);

	/* header contains byte-length of symbols followed by # symbols */
	data_word = symsize;
	if (!elf32_xlatetof(&dst_data, &src_data, data_encoding)) {
		fatal(5, "%s: ELF xlate failed: %s\n", prog,
				elf_errmsg(elf_errno()));
	}
	((long *)symtable)[0] = data_word;
	data_word = nksyms;
	if (!elf32_xlatetof(&dst_data, &src_data, data_encoding)) {
		fatal(5, "%s: ELF xlate failed: %s\n", prog,
				elf_errmsg(elf_errno()));
	}
	((long *)symtable)[1] = data_word;

	/* next come symbol name/value pairs */

	symp = symtable + 2 * sizeof(long);

	for (i = 0; i < nksyms; i++) {

		if (verbose) {
			printf("%s: %X\n", symarray[i]->name,
					   symarray[i]->value);
		}

		/* Round string length to multiple of 4, so addresses
		   are always long-word aligned. */
		slen = WORD_ROUNDUP(strlen(symarray[i]->name) + 1);

		memcpy(symp, symarray[i]->name, slen);
		symp += slen;

		/* convert symbol value to native byte order */
		data_word = symarray[i]->value;
		if (!elf32_xlatetof(&dst_data, &src_data, data_encoding)) {
			fatal(5, "%s: ELF xlate failed: %s\n", prog,
					elf_errmsg(elf_errno()));
		}

		*(unsigned long *)symp = data_word;
		symp += sizeof(unsigned long);
	}

	/* Null terminator name/value pair */
	memset(symp, 0, 2 * sizeof(unsigned long));
	symp += 2 * sizeof(unsigned long);

	fprintf(stderr, "%d symbols, table length = %d bytes (decimal)\n",
		nksyms, symsize + 4 * sizeof(long));

	/* next comes initial command string */

	if (icmd_len > 0) {
		if (read(ifd, symp, icmd_len) != icmd_len) {
			fatal(21,
			      "%s: error reading initial command file.\n",
			      prog);
		}
		symp += icmd_len;

		fprintf(stderr,
			"initial command string loaded (%d bytes)\n",
			icmd_len);
		close(ifd);
	}

	*symp++ = '\0';	/* Null terminator */

	/* next comes early-access command string */

	if (ecmd_len > 0) {
		if (read(efd, symp, ecmd_len) != ecmd_len) {
			fatal(21,
			      "%s: error reading early-access command file.\n",
			      prog);
		}
		symp += ecmd_len;

		fprintf(stderr,
			"early-access command string loaded (%d bytes)\n",
			ecmd_len);
		close(efd);
	}

	*symp++ = '\0';	/* Null terminator */

	free((char *)symarray);

	/* Patch the symbol table into the ELF file */

	symtab_addr = addsym(elf_file, SYMSECTION,
			     (_VOID *)symtable, symtab_size);

	if (symtab_addr == (Elf32_Addr)-1)
		fatal(30, "%s: failed to patch symbol table\n", prog);

	elf_end(elf_file);

	/* Patch the values for the symbol table pointer and size */

	if (lseek(fd, loc_symtable, 0) == -1L)
		fatal(7, "%s: seek error on %s\n", prog, argv[optind]);
	data_word = (unsigned long)symtab_addr;
	if (!elf32_xlatetof(&dst_data, &src_data, data_encoding)) {
		fatal(5, "%s: ELF xlate failed: %s\n", prog,
				elf_errmsg(elf_errno()));
	}
	if (write(fd, (char *)&data_word, sizeof(data_word)) == -1)
		fatal(6, "%s: write error on %s\n", prog, argv[optind]);

	if (lseek(fd, loc_symsize, 0) == -1L)
		fatal(7, "%s: seek error on %s\n", prog, argv[optind]);
	data_word = symtab_size;
	if (!elf32_xlatetof(&dst_data, &src_data, data_encoding)) {
		fatal(5, "%s: ELF xlate failed: %s\n", prog,
				elf_errmsg(elf_errno()));
	}
	if (write(fd, (char *)&data_word, sizeof(data_word)) == -1)
		fatal(6, "%s: write error on %s\n", prog, argv[optind]);

	close(fd);
}


fatal(code, format, arg1, arg2, arg3)
char *format;
char *arg1, *arg2, *arg3;
{
	fprintf(stderr, format, arg1, arg2, arg3);
	exit(code);
}


int
sym_cmp(a, b)
kdbsym_t **a, **b;
{
	return ((*a)->value > (*b)->value ? 1 : -1);
}
