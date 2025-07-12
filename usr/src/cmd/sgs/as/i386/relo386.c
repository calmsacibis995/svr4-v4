/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:i386/relo386.c	1.3"
/* relo386.c */

/* Support for i386 relocation entries. */


#include <sys/elf_386.h>
#include "common/as.h"
#include "common/expr.h"
#include "common/nums.h"
#include "common/sect.h"
#include "common/stmt.h"
#include "common/syms.h"
#include "relo386.h"
#include "stmt386.h"

/* Generate code for relocatable expression.  Called only from
** common code.
*/

void
#ifdef __STDC__
relocexpr(Value *vp, const Code *cp, Section *secp)
#else
relocexpr(vp, cp, secp) Value *vp; Code *cp; Section *secp;
#endif
{
    Uchar *p;
    static const char MSGbadreloc[] = "relocatable expression requires .long";

    if ((p = secp->sec_data) == 0)
	fatal("relocexpr(): called in size context");

    /* A relocatable expression can only be recorded in a 4 (or
    ** bigger) byte quantity.
    */
    if (cp->code_size < 4) {
	Ulong fno, lno;
	exprfrom(&fno, &lno, cp->data.code_expr);
	backerror(fno, lno, MSGbadreloc);
    }
    else {
	relocaddr(vp, p += cp->code_addr, secp);
	gen_value(vp, cp->code_size, p); /* output data bits */
    }
    return;
}


/* Do all the grotty bookkeeping to store relocation information
** for expression vp at location p in secp.  Use relocation type
** "type".
*/

static void
#ifdef __STDC__
dorelo(Value *vp, Uchar *p, Section *secp, int type)
#else
dorelo(vp, p, secp, type) Value *vp; Uchar *p; Section *secp; int type;
#endif
{
    Section *rel;

    if (secp->sec_data == 0)
	fatal("dorelo(): no section data");

    /* Create a relocation section associated with secp
    ** if none exists.  Make it relocatable without an
    ** explicit addend, because the addend is always present
    ** in the data or text section.
    */
    if ((rel = secp->sec_relo) == 0)
	rel = relosect(secp, SecTy_Reloc);

    /* Ignore temporary symbols, because they never end up
    ** in the object file symbol table.  The relocation will
    ** thus become section-based, rather than symbol-based.
    */
    if (vp->val_sym != 0 && vp->val_sym->sym_index == 0)
	vp->val_sym = 0;
    
    /* Now write relocation info, either section- or symbol-based.
    ** If possible, base relocations on a symbol.  Have to calculate
    ** offset in section for relocated value.  For symbol-based, have
    ** to calculate symbol-based offset.
    */
    if (vp->val_sym == 0)
	(void) sectrelsec(rel, type, p - secp->sec_data, vp->val_sec);
    else {
	(void) sectrelsym(rel, type, p - secp->sec_data, vp->val_sym);
	/* Current value represents
	**	symbol + <offset in section>
	** Subtract symbol's offset in section to give
	**	symbol + <offset from symbol>
	*/
	if (vp->val_dot)
	    subvalue(vp, vp->val_dot->code_addr);
    }
    return;
}


/* Generate relocation for value vp stored at address p in
** section secp.
*/

void
#ifdef __STDC__
relocaddr(Value *vp, Uchar *p, Section *secp)
#else
relocaddr(vp, p, secp) Value *vp; Uchar *p; Section *secp;
#endif
{
    int type;

    /* Determine relocation type.
    ** Correct application of the PIC modifiers is assured by the
    ** parsing and common expression code.  That is, an expression
    ** can only have a modifier applied to an identifier, and there
    ** can only be one such.
    */
    if ((vp->val_flags & VAL_RELOC) == 0)
	fatal("relocaddr(): not relocatable");

    switch( vp->val_pic ){
    case 0:
	if(vp->val_sym != 0 && vp->val_sym->sym_kind == SymKind_GOT)
		type = R_386_GOTPC;	/* must be GOTPC type */
	else
		type = R_386_32;
	break; /* no modifier */
    case ExpOp_Pic_PLT:
	type = R_386_PLT32; 
	break;
    case ExpOp_Pic_GOT:
	type = R_386_GOT32;
	break;
    case ExpOp_Pic_GOTOFF:
	type = R_386_GOTOFF; 
	break;
    default:
	fatal("relocaddr(): bad PIC modifier %#x", vp->val_pic);
    }
    dorelo(vp, p, secp, type);
    return;
}


void
#ifdef __STDC__
relocpcrel(Value *vp, Uchar *p, Section *secp)
#else
relocpcrel(vp, p, secp) Value *vp; Uchar *p; Section *secp;
#endif
{
    int type;

    /* Determine relocation type.
    ** Correct application of the PIC modifiers is assured by the
    ** parsing and the common expression code.  That is, an
    ** expression can only have a modifier applied to an identifier,
    ** and there can only be one such.  Note that this code also
    ** supports vp's being an absolute number!
    */

    switch( vp->val_pic ){
    case 0:			type = R_386_PC32;  break; /* no modifier */
    case ExpOp_Pic_PLT:		type = R_386_PLT32; break;
    case ExpOp_Pic_GOT:		type = R_386_GOT32; break;
    case ExpOp_Pic_GOTOFF:	type = R_386_GOTOFF; break;
    default:
	fatal("relocpcrel(): bad PIC modifier %#x", vp->val_pic);
    }
    dorelo(vp, p, secp, type);
    return;
}
