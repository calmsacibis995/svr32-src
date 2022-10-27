/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)as:common/obj.c	1.29.1.1"

#include <stdio.h>
#include <sgs.h>
#include <filehdr.h>
#include <linenum.h>
#include <reloc.h>
#include "scnhdr.h"
#include <syms.h>
#include "systems.h"
#include "symbols.h"
#include "gendefs.h"
#include "codeout.h"
#include "section.h"

#if IAPX | iAPX286
#include "instab.h"
#endif

/*
 *
 *	"obj.c" is a file that contains the routines for
 *	creating the object file during the final pass of the assembler.
 *	These include an assortment of routines for putting
 *	out header information, relocation and line number entries, and
 *	other data that is not part of the object program itself.  The
 *	main routines for producing the object program from the
 *	temporary files can be found in the file "codout.c".
 *
 */

/*
 *
 *	"outblock" is a function that outputs a block "block" with size
 *	"size" bytes to the file whose descriptor appears in "file".
 *
 */

#define outblock(a,b,c)	fwrite((char *)(a),b,1,c)

/*
 *
 *	"inblock" is a function that reads a block "block" of size
 *	"size" bytes from the file whose descriptor appears in "file".
 *
 */

#define inblock(a,b,c)	fread((char *)(a),b,1,c)

#define MAXSTK	20

extern char
	*calloc();

extern short
	transvec;	/* indicates transfer vector program addressing */
#if	M32
extern int
	need_mau;	/* indicates that the mau flag should be set */
	warlevel;	/* indicates level of workarounds generated */
#endif
extern long
	hdrptr,
	symbent,
	gsymbent;

extern long
	 getindx(),
	 time();

extern FILE
	*fdout,
	*fdrel,
	*fdsymb,
	*fdgsymb;

extern upsymins
	lookup();

#if FLEXNAMES
extern char	*strtab;
#endif

extern int seccnt;
extern struct scnhdr sectab[];
extern struct scninfo secdat[];

long	sect_lnnoptrs[NSECTIONS+1];
short	sttop = 0;

stent	firstsym = {
		0L,
		0L,
		NULL };

stent	*symhead = &firstsym,
	*symtail = &firstsym,
	*currsym = &firstsym;
	

stent	*stack[MAXSTK];

symbol	*dot;

FILHDR	filhead = {
#if IAPX | iAPX286
	0,		/* magic number */
			/* adjusted by setmagic() */
#else
	MAGIC,		/* magic number */
#endif
	0,		/* number of sections, 
			adjusted in headers() to seccnt */
	0L,		/* time and date stamp */
	0L,		/* file pointer to symbol table */
	0L,		/* number of symbol table entries */
	0,		/* size of optional header in bytes */
	0 };		/* flags */

static char
	buffer[BUFSIZ];

static unsigned
	undefcnt = 0;

/*
 * "headers" is a function that creates the file header and all of
 * the section headers and writes them to the file whose descriptor
 * appears in "fdout".
 *
 * The file header is built in "filhead".  The size of initialized
 * sections is obtained by adding the individual sizes.  The number
 * of symbol table entries are found in "symbent".  These are
 * stored into the header, and the header is written out using
 * "outblock".
 *
 * The dummy section header that points to the symbol table is
 * built in "symhead".  The file offset is obtained from "offset".
 */

headers()
{
	register int		i;
	long			lin, rel, siz;
	long			oldptr, address, secptr, linptr, relptr;
	register struct scnhdr	*p;

	oldptr = ftell(fdout);
	fseek(fdout, hdrptr, 0);
	lin = rel = siz = 0;
	for (i = 1, p = &sectab[1]; i <= seccnt; i++, p++)
	{
		lin += p->s_nlnno;
		rel += p->s_nreloc;
		if (secdat[i].s_typ != BSS) siz += p->s_size;
	}
	address = 0;
	secptr = FILHSZ + seccnt * SCNHSZ;
	relptr = secptr + siz;
	linptr = relptr + rel * RELSZ;
	filhead.f_nscns = seccnt;
	filhead.f_symptr = linptr + lin * LINESZ;
	filhead.f_nsyms = symbent;
	time(&(filhead.f_timdat));
	filhead.f_flags = 0;
	if (!undefcnt && !transvec) filhead.f_flags |= F_EXEC;
	if (!lin) filhead.f_flags |= F_LNNO;

	/*
	 * determine the conversion flag based on source machine  
	 */

#if AR16WR
	filhead.f_flags |= F_AR16WR;
#endif
#if AR32WR
	filhead.f_flags |= F_AR32WR;
#endif
#if AR32W
	filhead.f_flags |= F_AR32W;
#endif
#if BMAC
	filhead.f_flags |= F_BM32B;
	if (need_mau)
		filhead.f_flags |= F_BM32MAU;
#endif
#if ABWRMAC
	if (warlevel >= NO_AWAR)
		filhead.f_flags |= F_BM32B;
	else
		filhead.f_flags |= F_BM32RST;
	if (need_mau)
		filhead.f_flags |= F_BM32MAU;
#endif
	outblock(&filhead, FILHSZ, fdout);
	for (i = 1, p = &sectab[1]; i <= seccnt; i++, p++)
	{
		p->s_paddr = p->s_vaddr = address;
		if (p->s_nreloc) p->s_relptr = relptr;
		if (p->s_nlnno) p->s_lnnoptr = sect_lnnoptrs[i] = linptr;
		if (secdat[i].s_typ != BSS)
		{
			if (p->s_size) p->s_scnptr = secptr;
			secptr += p->s_size;
		}
		address += p->s_size;
		relptr += p->s_nreloc * RELSZ;
		linptr += p->s_nlnno * LINESZ;
		if (p->s_flags & STYP_LIB)
			p->s_flags &= ~STYP_DATA;
		outblock(p, SCNHSZ, fdout);
	}
	fseek(fdout, oldptr, 0);
}

/*
 *
 *	"copysect" is a function that is used to copy section
 *	information from an intermediate file to the object file.  It
 *	is passed the name of the intermediate file as a parameter.
 *	The object file should be open when this function is called,
 *	and its descriptor should appear in "fdout".  The intermediate
 *	file is opened, and then copied to "fdout" in a loop that
 *	alternately calls "fread" and "fwrite".
 *
 */

copysect(file)
	char *file;
{
	register FILE *fd;
	register short numelmts;

	if((fd = fopen(file,"r"))==NULL)
		aerror("Cannot Open Temporary File");
	do {
		numelmts = fread(buffer,sizeof(*buffer),BUFSIZ,fd);
		fwrite(buffer,sizeof(*buffer),numelmts,fdout);
	} while (numelmts == BUFSIZ);
	fclose(fd);
}

/*
 *
 *	"reloc" is a function that reads preliminary relocation entries
 *	from the file whose descriptor appears in "fdrel", processes
 *	them to produce the final relocation entries, and writes them
 *	out to the file whose descriptor appears in "fdout".  It is
 *	passed the following parameter:
 *
 *	num	This gives the number of relocation entries to process.
 *		This is needed because the relocation entries for both
 *		the text section and the data section are written to
 *		the same intermediate file.  When final processing takes
 *		place, these relocation entries need to go into
 *		different places.  Hence the text section entries and
 *		the data section entries must be copied with different
 *		calls.
 *
 *	This function reads preliminary relocation entries in a loop.
 *	A preliminary relocation entry consists of an address, followed
 *	by a tab, followed by the symbol with respect to which the
 *	relocation is to take place, followed by a tab, followed by
 *	the relocation type. "getindx" is used to determine the
 *	symbol table index of the symbol to which the relocation is
 *	to take place.
 *
 */

reloc(num)
	register long num;
{
	RELOC relentry;
	prelent trelent;
	long syment;

	while (num-- > 0L) {
		fread((char *)(&trelent),sizeof(prelent),1,fdrel);
		relentry.r_vaddr = trelent.relval;
		relentry.r_type = (unsigned)trelent.reltype;
		if (trelent.relname[0] == '=')
			/* The VAX case; formerly the "null name" ==>	*/
			/* -1 as symndx; now flagged by special char.	*/
			relentry.r_symndx = -1L;
#if FLEXNAMES
		else if (!*trelent.relname)
			/* A "null name" now implies a name in the	*/
			/* table.  For that case, we need to use the	*/
			/* offset into the string table found in the	*/
			/* last four bytes of the name as a long offset.*/
		{
			register int	i;
			union
			{
				long	offsets[2];
				char	dummy[8];
			} kludge;

			for (i = 0; i < 8; i++)
				kludge.dummy[i] = trelent.relname[i];
			if (((syment = getindx(&strtab[kludge.offsets[1]],C_EXT)) < 0L) &&
				((syment = getindx(&strtab[kludge.offsets[1]],C_STAT)) < 0L))
				aerror("reloc:Reference to symbol not in symbol table");
			relentry.r_symndx = syment;
		}
#endif
		else
		{
			if (((syment = getindx(trelent.relname,C_EXT)) < 0L) &&
				((syment = getindx(trelent.relname,C_STAT)) < 0L))
				aerror("reloc:Reference to symbol not in symbol table");
			relentry.r_symndx = syment;
		}
		outblock(&relentry, RELSZ, fdout);
	}
}

/*
 *
 *	"invent" is a function that invents a symbol table entry.  This
 *	is used for necessary entries for which no ".def" appears.
 *	This includes the special symbols ".text", ".data", and ".bss".
 *	It also includes all symbols declared ".globl" for which no
 *	".def" appears, and all undefined external symbols.  This
 *	function is passed the following parameters:
 *
 *	sym	This is a pointer to the symbol name.
 *
 *	val	This gives the value for the symbol.
 *
 *	sct	This gives the number of the section for the symbol.
 *
 *	scl	This gives the storage class of the symbol.
 *
 *	These values are put into the right format by storing into
 *	fields of a structure for the symbol table entry, and written
 *	to the file whose descriptor appears in "fdsymb" using
 *	"outblock".
 *
 */

invent(tsym,val,sct,scl)
	register symbol *tsym;
	long val;
	short sct;
	short scl;
{
	SYMENT sment;
	AUXENT axent;
	register short 	index;	/* loop variable */

#if FLEXNAMES
	if (tsym->_name.tabentry.zeroes == 0)
	{
		sment.n_zeroes = 0L;
		sment.n_offset = tsym->_name.tabentry.offset;
	}
	else
	{
#endif
		for(index = 0; (index < SYMNMLEN) && (sment.n_name[index] = tsym->_name.name[index]); index++)
			;
		for(; index < SYMNMLEN; index++)
			sment.n_name[index] = '\0';
#if FLEXNAMES
	}
#endif
	sment.n_value = val;
	sment.n_scnum = sct;
	sment.n_type = 0;
	sment.n_sclass = scl;
	sment.n_numaux = 0;
	if (transvec && sct && (tsym->styp & TVDEF)) {
		sment.n_numaux = 1;
	}
	outblock(&sment,SYMESZ,fdsymb);
	putindx(tsym,scl,symbent);
	symbent++;
	if (sment.n_numaux) {
		axent.x_sym.x_tagndx = 0L;
		axent.x_sym.x_misc.x_lnsz.x_lnno = 0;
		axent.x_sym.x_misc.x_lnsz.x_size = 0;
		for (index = 0; index < DIMNUM; ++index)
			axent.x_sym.x_fcnary.x_ary.x_dimen[index] = 0;
		axent.x_sym.x_tvndx = N_TV;
		outblock(&axent,AUXESZ,fdsymb);
		++symbent;
	}
}

/*
 *
 *	"outsyms" is a function used to create symbol table entries
 *	for all static, global, and undefined symbols for which no
 *	".def" appears in the source program. The name of this func-
 *	tion should be passed to "traverse" (in symbols.c) to call
 *	this function for each symbol table entry. This function
 *	first examines the first character of the symbol to see if
 *	it is an assembler defined identifier and ignores it if it
 *	is. Then the type of the symbol is checked to see if it is
 *	defined but not global and assumes the symbol to be a static.
 *	If this fails then the type is checked to see if it is both
 *	global and defined. For both static and global symbols,
 *	"getindx" is called to determine if an entry already exists
 *	for the symbol. If it does not, "invent" is called to create
 *	the entry in the object file symbol table. Finally the type
 *	is checked to see if the symbol is undefined and calls
 *	"invent" to create the entry.
 *
 */

outsyms(ptr)
	register symbol *ptr;
{
	register short sct;
	register char *strptr;

#if FLEXNAMES
 	strptr = (ptr->_name.tabentry.zeroes == 0) ? &strtab[ptr->_name.tabentry.offset] : ptr->_name.name;
#else
	strptr = ptr->_name.name;
#endif
	sct = (ptr->styp & (~TVDEF));
#if IAPX | iAPX286
	if (sct & HI12TYPE) { /* static X86 symbols */
		if (getindx(strptr,C_STAT) < 0)
			invent(ptr,ptr->value,ptr->sectnum,C_STAT);
		return;
	}
#endif
	if ((sct > ABS) && (sct < EXTERN)) {	/* static symbols */
#if MC68
		if ((strptr[0] == 'L' && strptr[1] == '%') || (strptr[0] == '('))
#else
		if ((strptr[0] == '.') || (strptr[0] == '('))
#endif
			return;	/* ignore compiler generated labels */
		if (getindx(strptr,C_STAT) < 0)
			invent(ptr,ptr->value,ptr->sectnum,C_STAT);
		return;
	}
	if (sct > EXTERN) {	/* global defined symbols */
		if(getindx(strptr,C_EXT)<0)
			invent(ptr,ptr->value,ptr->sectnum,C_EXT);
		return;
	}
	if ((ptr->styp == EXTERN) ||	/* global undefined symbols */
		(transvec && (ptr->styp == (TVDEF | EXTERN)))) {
		/* or tv defined symbols */
		if(getindx(strptr,C_EXT)<0){
			undefcnt++;
			invent(ptr,ptr->value,0,C_EXT);
		}
	}
}


/*
 *
 *	"symout" is the main routine for creating symbol table entries
 *	for which no ".def"s appear in the source program.  At the time
 *	this function is called, the file whose descriptor appears in
 *	"fdsymb" should contain all entries from ".def"s appearing in
 *	the text section, followed by all entries from ".def"s for
 *	static symbols appearing in the data section.  The file whose
 *	descriptor appears in "fdgsymb" should contain all other ".def"s
 *	from the data section.  "fdsymb" should be open for writing,
 *	and "fdgsymb" should be open for reading.  When this function
 *	returns, "fdsymb" will contain the entire symbol table.
 *
 *	This function first creates entries for the special section
 *	name symbols. These are defined as
 *	external static symbols to avoid conflict with similar defin-
 *	itions in other files.  The entries for these symbols are
 *	written to "fdsymb".
 *
 *	Following this, the file "fdgsymb" is copied to the end of
 *	"fdsymb".  This process insures that all global symbols follow
 *	all external static symbols.  The entries are examined as they
 *	are copied from one file to another, so that the correct indices
 *	for their positions in the symbol table can be determined.
 *
 *	The function "traverse" (from symbols.c) is called passing the
 *	function "outsyms" to create entries for all symbols declared
 *	".globl" that have no ".def" for them, any static symbols
 *	(user defined static symbols are generated only if a ".file"
 *	entry has been produced), and all global undefined symbols.
 *	See the comments for "outsyms" for how this works."
 *
 */

symout(){
	register short index, index2;		/* loop variables */
	register symbol *sym;
	register struct scnhdr *sect;
#if FLEXNAMES
	char	*symname;
#else
	char symname[SYMNMLEN+1];
#endif
	static codebuf
		statcbuf= { (long)C_STAT,0,0,0},
		nullcbuf = { 0L,0,0,0 };

/*	nullcbuf.caction = nullcbuf.cindex = nullcbuf.cnbits = 0;
	nullcbuf.cvalue = 0L;
	statcbuf.caction = statcbuf.cindex = statcbuf.cnbits = 0;
	statcbuf.cvalue = (long)C_STAT;
*/
	dot->styp = secdat[1].s_typ;
	dot->sectnum = 1;

	/*
	 * define section name symbols in storage class C_STAT
	 */
	for (index=1, sect= &sectab[1]; index<=seccnt; index++, sect++) {
		sym = lookup(sect->s_name,N_INSTALL,USRNAME).stp;
		nullcbuf.cvalue = 0;
		define(sym,&nullcbuf);
		setval(sym,&nullcbuf);
		setscl(NULLSYM,&statcbuf);		/*Storage Class C_STAT*/
        	dfaxent(sect->s_size,sect->s_nreloc,sect->s_nlnno);
		endef(NULLSYM,&nullcbuf);
		}

	for (index=0;index < gsymbent;index++){
#if FLEXNAMES
		/* We need a null-terminated name string to pass to	*/
		/* getindx.  If the name just fits within the eight	*/
		/* character name space of the syment, then that won't	*/
		/* work as the string.  This kludge gets around that	*/
		/* (efficiently) by copying the name to a location	*/
		/* big enough to handle the eight characters and a	*/
		/* null.  It is used whenever the name is in the	*/
		/* syment, rather than the string table.  WARNING!	*/
		/* Note that this kludge depends on the long == 4 char	*/
		/* identity currently found on all our machines.	*/

		union
		{
			long	l[3];
			char	c[12];
		} kludge;
#endif
		SYMENT sment;
		inblock(&sment,SYMESZ,fdgsymb);
		outblock(&sment,SYMESZ,fdsymb);
#if FLEXNAMES
		if (sment.n_zeroes != 0)
		{
		/* Copy the name to a place that has room for a null.	*/
			kludge.l[0] = sment.n_zeroes;
			kludge.l[1] = sment.n_offset;
			kludge.c[8] = '\0';
			symname = kludge.c;
		}
		else
			symname = &strtab[sment.n_offset];
#else
		symname[SYMNMLEN] = '\0';
		for(index2 = 0; index2 < SYMNMLEN; index2++)
			symname[index2] = sment.n_name[index2];
#endif
		sym = lookup(symname,N_INSTALL,USRNAME).stp;
		if (sym == NULLSYM)
			aerror("symout: Unknown symbol in symbol table");
		putindx(sym,sment.n_sclass,symbent);
		symbent++;
		if (sment.n_numaux > 0){
			AUXENT axent;
			inblock(&axent,AUXESZ,fdgsymb);
			outblock(&axent,AUXESZ,fdsymb);
			symbent++;
			index++;
		}
	}

	traverse(outsyms);
}

/*
 *
 *	"push", "pop", and "setsym" are procedures for producing
 *	the forward pointers for elements of the symbol table.
 *	"push" and "pop" maintain a stack that contains the symbol
 *	table index of the symbol that is receive the forward pointer
 *	and the symbol table index that is to become the forward
 *	pointer. "setsym" is called by "xform" (in addr.c) whenever
 *	a symbol is detected that can receive a forward pointer.
 *	It then determines the symbol table index of the auxiliary
 *	entry that is to receive the forward pointer and call "push"
 *	to enter that information onto the stack. "pop" is called
 *	by "xform" whenever the matching symbollic debugging entry
 *	is found to enter the current index plus one as the forward
 *	index.
 *
 */

static
push(value)
	register stent *value;
{
	if (sttop == MAXSTK - 1) {
		aerror("Symbol Table Stack Overflow");
	}
	stack[sttop++] = value;
}

stent *
pop()
{
	if (sttop == 0) {
		aerror("Unbalanced Symbol Table Entries-Too Many Scope Ends");
		return(NULL);
	}
	return(stack[--sttop]);
}

setsym(initval)
short	initval;
{
	currsym->stindex = symbent + 1L;	/* point to auxiliary entry */
	/* -1 indicates this isn't a function entry */
	currsym->fcnlen = (long)initval;
	if (symtail != currsym) {
		symtail->stnext = currsym;
	}
	symtail = currsym;
	currsym = (stent *)calloc(1,sizeof(stent));
	push(symtail);
}

/*
 *
 *	"fixst" is a procedure to fix the symbol table by entering
 *	the forward symbol table indices into the auxiliary entries
 *	of entries that denote the beginning of a logical scope
 *	block. It does this by using "ftell" to determine the present
 *	location in the symbol table, then using the symbol table
 *	element stack (pointed to by symhead) fixst seeks to the
 *	auxiliary entry, and writes the forward symbol table index
 *	found in the stack entry. When fixst is finished, i.e. the
 *	stack is empty, it returns to the original position in the
 *	object file.
 *
 */

fixst(fd)
	register FILE *fd;
{
	register stent *symptr;
	long home;
	AUXENT axent; /* dummy structure for address calculation */

	symptr = symhead;
	if (symptr->stindex > 0L) {
		home = ftell(fd);
		while (symptr != NULL) {
			if (symptr->fcnlen >= 0) {
				/*
				 * a negative value indicates this isn't a
				 * function entry, so don't overwrite
				 * that field of the aux entry
				 */
				fseek(fd,(symptr->stindex * SYMESZ)
					/* add in offset from beginning of structure */
					+ (((char *)&axent.x_sym.x_misc.x_fsize)
					- ((char *)&axent)), 0);
				fwrite((char *)(&(symptr->fcnlen)),
					sizeof(symptr->fcnlen),1,fd);
			}
			fseek(fd,(symptr->stindex * SYMESZ)
				+ (((char *)&axent.x_sym.x_fcnary.x_fcn.x_endndx)
				- ((char *)&axent)), 0);
			fwrite((char *)(&(symptr->fwdindex)),sizeof(symptr->fwdindex),1,fd);
			symptr = symptr->stnext;
		}
		fseek(fd,home,0);
	}
}
