/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include <stdio.h>
#include <libelf.h>

#define NBPC	0x1000
#define BPCSHIFT	12
#define	btoc(x)	(((unsigned)(x)+(NBPC-1))>>BPCSHIFT)
#define	ctob(x)	((x)<<BPCSHIFT)

#ifdef __STDC__
#define _VOID	void
#else
#define _VOID	char
#endif


Elf32_Addr
#ifdef __STDC__
addsym(Elf *elf, char *scnname, void *p, unsigned int size)
#else
addsym(elf, scnname, p, size)
Elf *elf;
char *scnname;
_VOID *p;
unsigned int size;
#endif
{
	Elf32_Ehdr	*ehdr;
	Elf32_Phdr	*phdr, *phdrout;
	Elf_Scn		*scn;
	Elf32_Shdr	*shdr;
	Elf_Data	*data;
	Elf32_Off	name_off = 0;
	char		*shname;
	Elf32_Half	pnum, opnum, highpnum;
	unsigned int	size1;
	unsigned int	totalsize;


	if ((ehdr = elf32_getehdr(elf)) == NULL) {
		fprintf(stderr, "%s\n", elf_errmsg(-1));
		return (Elf32_Addr)-1;
	}

	phdr = elf32_getphdr(elf);

	totalsize = size = (size + 3) & ~3;

	/*
	 * See if the desired section already exists.
	 */
	shname = NULL;
	scn = NULL;
	while ((scn = elf_nextscn(elf, scn)) != NULL) {

		shdr = elf32_getshdr(scn);
 
		shname = elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name);

		if (shname == NULL)
			continue;

		if ((strcmp(shname, scnname)) == 0)
			break;

		shname = NULL;

	}

	if (shname) {	/* we already have a section */
		for (pnum = 0; pnum < ehdr->e_phnum; ++pnum) {
			if (phdr[pnum].p_vaddr == shdr->sh_addr)
				break;
		}
		phdrout = phdr;
		name_off = shdr->sh_name;
		data = elf_getdata(scn, NULL);
		if ((size1 = data->d_size) > size)
			size1 = size;
		memcpy((char *)data->d_buf, (char *)p, size1);
		p = (_VOID *)((char *)p + size1);
		size -= size1;
		elf_flagdata(data, ELF_C_SET, ELF_F_DIRTY);

	} else {	/* need to create a new section */

		/*
		 * Copy existing program headers to new array,
		 * and find the one with the highest address.
		 */
		opnum = ehdr->e_phnum;
		phdrout = elf32_newphdr(elf, opnum + 1);
		highpnum = (Elf32_Half)-1;
		for (pnum = 0; pnum < opnum; ++pnum) {
			phdrout[pnum] = phdr[pnum];
			if (phdr[pnum].p_type != PT_LOAD)
				continue;
			if (highpnum == (Elf32_Half)-1 ||
			    phdr[pnum].p_paddr > phdr[highpnum].p_paddr ||
			    (phdr[pnum].p_paddr == phdr[highpnum].p_paddr &&
			     phdr[pnum].p_vaddr > phdr[highpnum].p_vaddr)) {
				highpnum = pnum;
			}
		}

		if (highpnum == (Elf32_Half)-1) {
			fprintf(stderr,
				"Can't find a loadable program segment.\n");
			return (Elf32_Addr)-1;
		}

		/*
		 * Place the new program header after the "highest" one,
		 * both virtually and physically.
		 */
		phdrout[pnum].p_vaddr = ctob(btoc(phdr[highpnum].p_vaddr +
						  phdr[highpnum].p_memsz));
		phdrout[pnum].p_paddr = ctob(btoc(phdr[highpnum].p_paddr +
						  phdr[highpnum].p_memsz));

		/*
		 * Append new section name to end of string table.
		 */
		scn = NULL;
		while ((scn = elf_nextscn(elf, scn)) != NULL) {

			char *sname;

			shdr = elf32_getshdr(scn);

			if (shdr->sh_type != SHT_STRTAB)
				continue;

			sname = elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name);

			if (!sname)
				continue;

			if ((strcmp(sname, ".shstrtab")) != 0)
				continue;

			name_off = shdr->sh_size;

			break;
		}

		if (!name_off) {
			fprintf(stderr, "Can't find .shrstrtab section.\n");
			return (Elf32_Addr)-1;
		}

		data = elf_newdata(scn);

		data->d_buf = (_VOID *)scnname;
		data->d_type = ELF_T_BYTE;
		data->d_size = strlen(scnname) + 1;
		data->d_align = 1;

		/* Allocate the new section */
		scn = elf_newscn(elf);
		shdr = elf32_getshdr(scn);
	}

	if (size > 0) {
		data = elf_newdata(scn);
		data->d_buf = (_VOID *)p;
		data->d_type = ELF_T_BYTE;
		data->d_size = size;
		data->d_align = (shname == NULL ? ctob(1) : 4);
	}

	shdr->sh_name = name_off;
	shdr->sh_type = SHT_PROGBITS;
	shdr->sh_flags = SHF_ALLOC;
	shdr->sh_addr = phdrout[pnum].p_vaddr;
	shdr->sh_link = SHN_UNDEF;
	shdr->sh_info = 0;
	shdr->sh_entsize = 0;

	elf_update(elf, ELF_C_NULL);

	phdrout[pnum].p_type = PT_LOAD;
	phdrout[pnum].p_offset = shdr->sh_offset;
	phdrout[pnum].p_filesz = totalsize;
	phdrout[pnum].p_memsz = totalsize;
	phdrout[pnum].p_flags = PF_R|PF_W|PF_X;
	phdrout[pnum].p_align = 0x1000;

	elf_flagphdr(elf, ELF_C_SET, ELF_F_DIRTY);

	elf_update(elf, ELF_C_WRITE);

	return phdrout[pnum].p_vaddr;
}
