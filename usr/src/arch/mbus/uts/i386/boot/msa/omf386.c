/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1988  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	


#ident	"@(#)mbus:uts/i386/boot/msa/omf386.c	1.3.2.1"

#include "../sys/boot.h"
#include "../sys/s2main.h"
#include "../sys/error.h"
#include "../sys/omf2386.h"

extern	char	file_read_msg [];
extern	char	no_tss_msg [];

extern	ushort	ourDS;
extern	char	bdevice_name [MAX_BDEVICE_VALUE];
extern	ushort	prot_mode_boot;
extern	ushort	debug_on_boot;
extern	ushort	mem_alias_sel;
extern	ulong	status;
extern	ulong	actual;

#ifndef lint
#pragma pack(1)
#endif

struct desc_str386 {
	ushort	limit ;
	ulong	base ;
} ;

struct seg_desc_str386 {
        ushort seg_limit0_15;
        char seg_base0_7;
        char seg_base8_15;
        char seg_base16_23;
        char attr_1;
        char attr_2;	/* contains limit bits 16-19 */
        char seg_base24_31;
} seg_desc386;

struct f386_header {
	char space [3];				/* remember we read 2 bytes first */
	char date_time [16];
	char creator [41];
	struct desc_str386 gdt_desc;
	struct desc_str386 idt_desc;
	ushort tss_select;
} f386_hdr;
 
struct tss_386_str {
        ushort backlink;
        char stack_ptrs [30];
        ulong  eip;
        ulong  eflag;
        ulong  eax;
        ulong  ecx;
        ulong  edx;
        ulong  ebx;
        ulong  esp;
        ulong  ebp;
        ulong  esi;
        ulong  edi;
        ushort es;
		ushort res1;
        ushort cs;
		ushort res2;
        ushort ss;
		ushort res3;
        ushort ds;
		ushort res4;
        ushort fs;
		ushort res5;
        ushort gs;
		ushort res6;
        ushort ldt;
		ushort res7;
		ushort debug_bit;
		ushort io_map_base;
} tss_386;

struct abstxt386 {
	char phys_addr_b[4];
	unsigned long len;
} abstxt386;

#ifndef lint
#pragma pack()
#endif

struct toc386 {
	long abstxt_offset;
	long debtxt_offset;
	long last_offset;
	long next;
	long os_info_loc;
	long res[2];
} toc386;

struct desc_str386 our386_gdt_desc;
struct desc_str386 our386_idt_desc;

struct desc_str386 t386_gdt_desc;
struct desc_str386 t386_idt_desc;

char target_boot_string [256];
char trg_file_name [MAX_PARAM_VALUE];
struct seg_desc_str386 our386_tss_base;
struct tss_386_str our386_tss_rec;

ulong phys_addr;
ulong phys_len;
ulong target_tss_base;
ulong k386_start;

do_omf386(tfile_name)
register char tfile_name [MAX_PARAM_VALUE];
{	register long offset;
	register long last_offset;
	register int i;	

	/* save the file name being passed in */ 
	strncpy(&trg_file_name[0], &tfile_name[0], MAX_PARAM_VALUE);	

	/* read in the header and save the necessary fields */ 

	BL_file_read((char *)&f386_hdr, ourDS, (ulong)(sizeof(f386_hdr)), 
			&actual, &status);
	if ((status != E_OK) || (actual != (ulong)(sizeof(f386_hdr)))) 
		error(STAGE2, (ulong)status, (char *)file_read_msg);

	t386_gdt_desc.limit = f386_hdr.gdt_desc.limit;
	t386_gdt_desc.base = f386_hdr.gdt_desc.base;
	t386_idt_desc.limit = f386_hdr.idt_desc.limit;
	t386_idt_desc.base = f386_hdr.idt_desc.base;

	/* Read Table of Contents */

	BL_file_read((char *)&toc386, ourDS, (ulong) sizeof(toc386), 
			&actual, &status);
	if ((status != E_OK) || (actual != (ulong)sizeof toc386)) 
		error(STAGE2, (ulong)status, (char *)file_read_msg);

	/* go to start of absolute text */

	scan_to((ulong) ((sizeof(f386_hdr)) + 2) + (ulong) (sizeof(toc386)), 
							toc386.abstxt_offset);
 
	/* if protected mode and no tss then there is an error */
	if (prot_mode_boot) 
		if (f386_hdr.tss_select == 0)
			error(STAGE2, E_NO_TSS, (char *)no_tss_msg);

	last_offset = (toc386.debtxt_offset == 0 ? toc386.last_offset : 
						toc386.debtxt_offset);
	offset = toc386.abstxt_offset;
	i = 0;

	/* find the code to be loaded */
	while (offset < last_offset) {

		char temp_addr[4];
		
		BL_file_read((char *)&abstxt386, ourDS, (ulong) sizeof(abstxt386), 
			&actual, &status);
		if ((status != E_OK) || (actual != (ulong) sizeof(abstxt386))) 
			error(STAGE2, (ulong)status, (char *)file_read_msg);
		
		temp_addr[0] = abstxt386.phys_addr_b[0];
		temp_addr[1] = abstxt386.phys_addr_b[1];
		temp_addr[2] = abstxt386.phys_addr_b[2];
		temp_addr[3] = abstxt386.phys_addr_b[3];
		
		phys_addr = *(ulong *) temp_addr;
		phys_len = (ulong) abstxt386.len;

		/* the real mode start is the address of the first record */
		/* the first time through it is put in correct */
                /* format, sel : offset */ 

		if (i == 0) {
			k386_start = (phys_addr << 12) & 0xF0000000;
			k386_start += phys_addr & 0xFFFF;
		}
		/* load data into correct place */

		BL_file_read((char *)phys_addr, mem_alias_sel, 
                                      phys_len, &actual, &status);
		if ((status != E_OK) || (actual != phys_len)) 
			error(STAGE2, (ulong)status, (char *)file_read_msg);

		/* set new offset */

		offset += phys_len + sizeof(abstxt386);
		i++;
	}
	
	/* these calls do not return */
	
	if (prot_mode_boot)
		do_protected386();
	else
		do_real(k386_start);
}

do_protected386()
{
	/* save gdt, idt */
	gdt_move(&our386_gdt_desc, 0);
	idt_move(&our386_idt_desc, 0);
	
	/* if gdt, idt entry is empty copy from fw table */
        /* to target table */
	copy_desc(our386_gdt_desc, t386_gdt_desc);
	copy_desc(our386_idt_desc, t386_idt_desc);
	
	/* set tss registers, load new gdt and idt, jump to start */
        /* protected mode address */
	set_tss386();
	gdt_move(&t386_gdt_desc, 1);
	idt_move(&t386_idt_desc, 1);
	jump_tss(0, (f386_hdr.tss_select & MASK_INDEX)); /* offset, selector */
}

char temp_base [4];
set_tss386()
{	register char fname_len, dname_len, index;

	/* find physical location of tss */

	target_tss_base = t386_gdt_desc.base + 
                                       (f386_hdr.tss_select & MASK_INDEX); 

	/* make a local copy of the descriptor (our386_tss_base) and */
	/* rearrange those base fields to proper format */

	iomove(target_tss_base, mem_alias_sel, &our386_tss_base, ourDS, 
                                                          sizeof(seg_desc386));
	temp_base[0] = our386_tss_base.seg_base0_7;
	temp_base[1] = our386_tss_base.seg_base8_15;
	temp_base[2] = our386_tss_base.seg_base16_23;
	temp_base[3] = our386_tss_base.seg_base24_31;
	target_tss_base = *(ulong *) temp_base;
	
	/* use the formatted base to make a local copy of the tss segment */
        /* and registers (our386_tss_rec) */

	iomove(target_tss_base, mem_alias_sel, &our386_tss_rec, ourDS, 
							sizeof(tss_386));
	/* make the device name, file name string to be booted */
	/* the format is device name length:device:filename length filename */
	/* device name length includes the :'s */
	
	index = 0;

	dname_len = strlen(bdevice_name);
	fname_len = strlen(trg_file_name);	
	target_boot_string[index++] = dname_len +2;	
	target_boot_string[index++] = ':';	
	strncpy(&target_boot_string[index], bdevice_name, dname_len);	
	index += dname_len;
	target_boot_string[index++] = ':';	
	target_boot_string[index++] = fname_len;	
	strncpy(&target_boot_string[index], trg_file_name, fname_len);	

	/* fill in the local copy of the tss registers */
	/* si contains the selector to the boot name string */
	/* di contains the offset - this works because the string is located */
	/* in the first 64K of memory (this is guaranteed by the position */
  	/* of this file when linked in the makefile) */	

	our386_tss_rec.esi = ourDS;
	our386_tss_rec.edi = (ushort)target_boot_string;
	our386_tss_rec.ecx = 0x1234;
	if (debug_on_boot)
		our386_tss_rec.edx = 0x1235;
	else
		our386_tss_rec.edx = 0x1234;	

	/* move the filled in copy to the physical location of target tss */ 

	iomove(&our386_tss_rec, ourDS, target_tss_base, mem_alias_sel, 
                                                          sizeof(tss_386));
}
