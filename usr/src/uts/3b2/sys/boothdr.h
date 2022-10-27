/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-port:sys/boothdr.h	10.5"
/*
 * Each object file existing in the /boot directory contains an 
 * optional header containing the following information.  This header
 * is created by the mkboot(1M) program from the /etc/master data base.
 */

#define	OFFSET(ptr,base)	((offset) ((char*)(ptr) - (char*)(base)))
#define	POINTER(offset,base)	((char*)(base) + (offset))

#define	ROUNDUP(p)		(((int)(p)+sizeof(int)-1) & ~(sizeof(int)-1))

typedef	unsigned short		offset;

#define	PARAMNMSZ	8	/* maximun size of a parameter name in /etc/master data base */

/*
 * Optional header for object files (all offsets are computed
 * from the base of this structure)
 */

struct	master
	{
	unsigned short		magic;		/* "3b"; to distinguish from UNIX a.out header */
	unsigned short		flag;		/* /etc/master: flags */
	unsigned char		vec;		/* /etc/master: first vector for integral device */
	unsigned char		nvec;		/* /etc/master: number of interrupt vectors */
	char			prefix[4+1];	/* /etc/master: module prefix, '\0' terminated */
	unsigned char		soft;		/* /etc/master: software module external major number */
	unsigned char		ndev;		/* /etc/master: number of devices/controller */
	unsigned char		ipl;		/* /etc/master: interrupt priority level */
	short			ndep;		/* number of dependent modules */
	short			nparam;		/* number of parameters */
	short			nrtn;		/* number of routine names */
	short			nvar;		/* number of variables */
	offset			o_depend;	/* ==> additional modules required */
	offset			o_param;	/* ==> parameters for this module */
	offset			o_routine;	/* ==> routines to be stubbed if module is not loaded */
	offset			o_variable;	/* ==> variables to be generated */
	};

/*
 *	Magic number to distinguish from UNIX optional a.out header
 */
#define	MMAGIC	0x3362

/*
 *	FLAG bits
 */
#define	KERNEL	0x8000		/* this is a kernel a.out file */
#define	FUNDRV	0x0200		/* (f) framework/stream type device */
#define	FUNMOD	0x0100		/* (m) framework/stream module */
#define	ONCE	0x0080		/* (o) allow only one specification of device */
#define	REQADDR	0x0040		/* (a) xx_addr array must be generated */
#define	TTYS	0x0020		/* (t) cdevsw[].d_ttys mustpoint to first generated data structure */
#define	REQ	0x0010		/* (r) required device */
#define	BLOCK	0x0008		/* (b) block type device */
#define	CHAR	0x0004		/* (c) character type device */
#define	SOFT	0x0002		/* (s) software device driver */
#define	NOTADRV	0x0001		/* (x) not a driver; a configurable module */
#define FSTYP	0x0400

/*
 * Dependencies: if the current module is loaded, then the following
 *               modules must also be loaded
 */
struct	depend
	{
	offset		name;		/* module name */
	};

/*
 * Parameters: each parameter defined for this module is saved to allow
 *             references from other modules
 */
struct	param
	{
	char		name[PARAMNMSZ];/* parameter name */
	char		type;		/* string ("), integer (N) */
	union	{
		int number;		/* integer value */
		offset string;		/* ==> string */
		}
			value;
	};

/*
 * Routines: if the current module is not loaded, then the following
 *           routines must be stubbed off
 */
struct	routine
	{
	char		id;		/*  routine type */
	offset		name;		/* ==> routine name */
	};

/*
 *	Routine types
 */
#define RNULL	0		/* void rtn() {} */
#define RNOSYS	1		/* rtn() { return(nosys()); } */
#define RNODEV	2		/* rtn() { return(nodev()); } */
#define RTRUE	3		/* rtn() { return(1); } */
#define RFALSE	4		/* rtn() { return(0); } */
#define RFSNULL 5
#define RFSTRAY	6
#define NOPKG   7
#define NOREACH	8

/*
 * Variables: each variable to be generated by the boot program is
 *            described by this structure
 */
struct	variable
	{
	int		size;		/* total size of single element */
	offset		name;		/* ==> variable name */
	offset		dimension;	/* ==> array dimension; NULL if not an array */
	short		ninit;		/* number of elements to be initialized */
	offset		initializer;	/* ==> initialization */
	};

/*
 * Initializer structure: a variable is initialized by an array of initializer
 *                        elements; each element contains the type of the field
 *                        and either the initial value, or the offset to the
 *                        expression which, when evaluated, will be the initial
 *                        value.
 */
struct	format
	{
	unsigned char	type;		/* flags and type of the field */
	unsigned char	strlen;		/* %<strlen>c */

					/* type&FEXPR:  ==> expression */
	offset		value;		/* type&FSKIP:  %<number> */
					/* otherwise:   literal value */
	};

/*
 * Format types
 */
#define	FSKIP	0x80			/* %<n>  no initialization; skip `value' bytes */
#define	FEXPR	0x40			/* POINTER((offset)value, &opthdr) -> expression */
#define	FTYPE	0x3F			/* One of:                     */
#define	FCHAR	1			/*	%c    character        */
#define	FSHORT	2			/*	%s    short integer    */
#define	FINT	3			/*	%i    integer          */
#define	FLONG	4			/*	%l    long integer     */
#define	FSTRING	5			/*	%<n>c character string */

/*
 * Expressions are stored in prefix polish notation.  Each element of
 * the expression is one of the elements below.
 *
 *		operator:  +, -, * or /
 *		function:  <, or >	(min or max)
 *		builtin:   #Dname\0
 *		builtin:   #Cname\0
 *		builtin:   #Sname\0
 *		builtin:   #Mname\0
 *		sizeof:    #name\0
 *		address:   &name\0
 *		string:    "string\0
 *		identifier:Iname\0
 *		number:    Ninteger
 */

#define	ELENGTH	256

union	element
	{
	char		operator;		/* +, -, * or / */
	char		function;		/* < or > */
	char		nD[ELENGTH];		/* D number of subdevices */
	char		nC[ELENGTH];		/* C number of controllers */
	char		nS[ELENGTH];		/* S number of logical units */
	char		nM[ELENGTH];		/* M internal major number */
	char		size_of[ELENGTH];	/* # name */
	char		address_of[ELENGTH];	/* & name */
	char		string[ELENGTH];	/* " name */
	char		identifier[1+PARAMNMSZ];/* I name */
	char		literal[1+sizeof(int)];	/* N integer */
	};

#define	XBUMP(p,what)	(union element *)((int)(p) + ((sizeof((p)->what)==ELENGTH)? \
							1+strlen((p)->what) : \
							sizeof((p)->what)))