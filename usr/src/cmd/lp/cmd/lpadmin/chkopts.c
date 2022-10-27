/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:cmd/lpadmin/chkopts.c	1.9"

#include "stdio.h"
#include "string.h"
#include "pwd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "errno.h"

#include "lp.h"
#include "printers.h"
#include "form.h"
#include "class.h"

#define	WHO_AM_I	I_AM_LPADMIN
#include "oam.h"

#include "lpadmin.h"


extern PRINTER		*printer_pointer;

extern PWHEEL		*pwheel_pointer;

extern struct passwd	*getpwnam();

void			chkopts2(),
			chkopts3();

FORM			formbuf;

char			**f_allow,
			**f_deny,
			**u_allow,
			**u_deny;

PRINTER			*oldp		= 0;

PWHEEL			*oldS		= 0;

unsigned short		daisy		= 0;

static int		root_can_write();

static char		*unpack_sdn();

/**
 ** chkopts() -- CHECK LEGALITY OF COMMAND LINE OPTIONS
 **/

void			chkopts ()
{
	/*
	 * Check -d.
	 */
	if (d) {
		if (
			a || c || f || j || m || M || p || r || u || x
#if	defined(DIRECT_ACCESS)
		     || C
#endif
		     || strlen(modifications)
		) {
			LP_ERRMSG (ERROR, E_ADM_DALONE);
			done (1);
		}

		if (
			*d
		     && !STREQU(d, NAME_NONE)
		     && !isprinter(d)
		     && !isclass(d)
		) {
			LP_ERRMSG1 (ERROR, E_ADM_NODEST, d);
			done (1);
		}
		return;
	}

	/*
	 * Check -x.
	 */
	if (x) {
		if (
			a || c || f || j || m || M || p || r || u || d
#if	defined(DIRECT_ACCESS)
		     || C
#endif
		     || strlen(modifications)
		) {
			LP_ERRMSG (ERROR, E_ADM_XALONE);
			done (1);
		}

		if (
			!STREQU(NAME_ALL, x)
		     && !STREQU(NAME_ANY, x)
		     && !isprinter(x)
		     && !isclass(x)
		) {
			LP_ERRMSG1 (ERROR, E_ADM_NODEST, x);
			done (1);
		}
		return;
	}

	/*
	 * Problems common to both -p and -S (-S alone).
	 */
	if (A && STREQU(A, NAME_LIST) && (W != -1 || Q != -1)) {
		LP_ERRMSG (ERROR, E_ADM_LISTWQ);
		done (1);
	}

	/*
	 * Check -S.
	 */
	if (!p && S) {
		if (M || a || f || c || r || e || i || m || h || l || v || I || T || D || F || u || U || o || j) {
			LP_ERRMSG (ERROR, E_ADM_SALONE);
			done (1);
		}
		if (!A && W == -1 && Q == -1) {
			LP_ERRMSG (ERROR, E_ADM_NOAWQ);
			done (1);
		}
		if (S[0] && S[1])
			LP_ERRMSG (WARNING, E_ADM_ASINGLES);
		if (!STREQU(NAME_ALL, *S) && !STREQU(NAME_ANY, *S)) 
			chkopts3(1);
		return;
	}

	/*
	 * At this point we must have a printer (-p option).
	 */
	if (!p) {
		LP_ERRMSG (ERROR, E_ADM_NOACT);
		done (1);
	}


	/*
	 * Mount but nothing to mount?
	 */
	if (M && (!f && !S)) {
		LP_ERRMSG (ERROR, E_ADM_MNTNONE);
		done (1);
	}

	/*
	 * -Q isn't allowed with -p.
	 */
	if (Q != -1) {
		LP_ERRMSG (ERROR, E_ADM_PNOQ);
		done (1);
	}

	/*
	 * Fault recovery.
	 */
	if (
		F
	     && !STREQU(F, NAME_WAIT)
	     && !STREQU(F, NAME_BEGINNING)
	     && (
			!STREQU(F, NAME_CONTINUE)
		     || j
		     && STREQU(F, NAME_CONTINUE)
		)
	) {
#if	defined(J_OPTION)
		if (j)
			LP_ERRMSG (ERROR, E_ADM_FBADJ);
		else
#endif
			LP_ERRMSG (ERROR, E_ADM_FBAD);
		done (1);
	}

#if	defined(J_OPTION)
	/*
	 * The -j option is used only with the -F option.
	 */
 	if (j) {
		if (M || a || f || c || r || e || i || m || h || l || v || I || T || D || u || U || o) {
			LP_ERRMSG (ERROR, E_ADM_JALONE);
			done (1);
		}
		if (j && !F) {
			LP_ERRMSG (ERROR, E_ADM_JNOF);
			done (1);
		}
		return;
	}
#endif

#if	defined(DIRECT_ACCESS)
	/*
	 * -C is only used to modify -u
	 */
	if (C && !u) {
		LP_ERRMSG (ERROR, E_ADM_CNOU);
		done (1);
	}
#endif

	/*
	 * The -a option needs the -M and -f options,
	 * Also, -ofilebreak is used only with -a.
	 */
	if (a && (!M || !f)) {
		LP_ERRMSG (ERROR, E_ADM_MALIGN);
		done (1);
	}
	if (filebreak && !a)
		LP_ERRMSG (WARNING, E_ADM_FILEBREAK);

	/*
	 * If old printer, make sure it exists. If new printer,
	 * check that the name is okay, and that enough is given.
	 */

	if (
		(
			STREQU(NAME_NONE, p)
		     || STREQU(NAME_ANY, p)
		     || STREQU(NAME_ALL, p)
		)
	     && (
			a || h || l || M || D || e || f || i
		     || I || m || S || T || u || U || v
		     || banner != -1 || cpi || lpi || width || length || stty
		)
	) {
		LP_ERRMSG (ERROR, E_ADM_ANYALLNONE);
		done (1);

	} 

	if (!STREQU(NAME_ALL, p) && !STREQU(NAME_ANY, p)) 
		chkopts2(1);

	/*
	 * Only one of -i, -m, -e.
	 */
	if ((i && e) || (m && e) || (i && m)) {
		LP_ERRMSG (ERROR, E_ADM_INTCONF);
		done (1);
	}

	/*
	 * Check -e arg.
	 */
	if (e) {
		if (!isprinter(e)) {
			LP_ERRMSG1 (ERROR, E_ADM_NOPR, e);
			done (1);
		}
		if (strcmp(e, p) == 0) {
			LP_ERRMSG (ERROR, E_ADM_SAMEPE);
			done (1);
		}
	}

	/*
	 * Check -m arg.
	 */
	if (m && !ismodel(m)) {
		LP_ERRMSG1 (ERROR, E_ADM_NOMODEL, m);
		done (1);
	}

	/*
	 * Need exactly one of -h or -l (but will default -h).
	 */
	if (h && l) {
		LP_ERRMSG2 (ERROR, E_ADM_CONFLICT, 'h', 'l');
		done (1);
	}
	if (!h && !l)
		h = 1;

	/*
	 * Check -c and -r.
	 */
	if (c && r && strcmp(c, r) == 0) {
		LP_ERRMSG (ERROR, E_ADM_SAMECR);
		done (1);
	}

	/*
	 * Are we creating a class with the same name as a printer?
	 */
	if (c) {
		if (STREQU(c, p)) {
			LP_ERRMSG1 (ERROR, E_ADM_CLNPR, c);
			done (1);
		}
		if (isprinter(c)) {
			LP_ERRMSG1 (ERROR, E_ADM_CLPR, c);
			done (1);
		}
	}

	/*
	 * The device must be writeable by root.
	 */
	if (v && root_can_write(v) == -1)
		done (1);

	/*
	 * Can't have both device and dial-out.
	 */
	if (v && U) {
		LP_ERRMSG (ERROR, E_ADM_BOTHUV);
		done (1);
	}

	/*
	 * We need the printer type for some things, and the boolean
	 * "daisy" (from Terminfo) for other things.
	 */
	if (!T && oldp)
		T = oldp->printer_type;
	if (
		T
	     && (
			(chkprinter(T, (char *)0, (char *)0, (char *)0, (char *)0, (char *)0) & PCK_TYPE)
		     || tidbit(T, "daisy", &daisy) == -1
		)
	) {
		LP_ERRMSG1 (ERROR, E_ADM_BADTYPE, T);
		done (1);
	}
	if (cpi || lpi || length || width || S || f || filebreak)
		if (!T) {
			LP_ERRMSG (ERROR, E_ADM_TOPT);
			done (1);

		}

	/*
	 * If we'll be inserting page breaks between alignment patterns,
	 * look up the control sequence for this.
	 */
	if (filebreak && tidbit(T, "ff", &ff) == -1) {
		LP_ERRMSG1 (ERROR, E_ADM_BADTYPE, T);
		done (1);
	}

	/*
	 * Check -o cpi=, -o lpi=, -o length=, -o width=
	 */
	if (cpi || lpi || length || width) {
		unsigned	long	rc;

		if ((rc = chkprinter(T, cpi, lpi, length, width, NULL)) != 0) {
			if ((rc & PCK_CPI) && cpi)
				LP_ERRMSG1 (ERROR, E_ADM_BADCAP, "cpi=");

			if ((rc & PCK_LPI) && lpi)
				LP_ERRMSG1 (ERROR, E_ADM_BADCAP, "lpi=");

			if ((rc & PCK_WIDTH) && width)
				LP_ERRMSG1 (ERROR, E_ADM_BADCAP, "width=");

			if ((rc & PCK_LENGTH) && length)
				LP_ERRMSG1 (ERROR, E_ADM_BADCAP, "length=");

			LP_ERRMSG1 (ERROR, E_ADM_BADCAPS, T);
			done(1);
		}
	}

	/*
	 * MOUNT:
	 * Only one print wheel can be mounted at a time.
	 */
	if (M && S && S[0] && S[1])
		LP_ERRMSG (WARNING, E_ADM_MSINGLES);

	/*
	 * NO MOUNT:
	 * If the printer takes print wheels, the -S argument
	 * should be a simple list; otherwise, it must be a
	 * mapping list. (EXCEPT: In either case, "none" alone
	 * means delete the existing list.)
	 */
	if (S && !M) {
		register char		**item,
					*cp;

		if (STREQU(S[0], NAME_NONE) && !S[1]) {
			;

		/*
		 * For us to be here, "daisy" must have been set.
		 * (-S requires knowing printer type (T), and knowing
		 * T caused call to "tidbit()" to set "daisy").
		 */
		} else if (daisy) {
			for (item = S; *item; item++) {
				if (strchr(*item, '=')) {
					LP_ERRMSG1 (ERROR, E_ADM_PWHEELS, T);
					done (1);
				}
				if (!syn_name(*item)) {
					LP_ERRMSG1 (ERROR, E_LP_NOTNAME, *item);
					done (1);
				}
			}
		} else {
			register int		die = 0;

			for (item = S; *item; item++) {
				if (!(cp = strchr(*item, '='))) {
					LP_ERRMSG1 (ERROR, E_ADM_CHARSETS, T);
					done (1);
				}

				*cp = 0;
				if (!syn_name(*item)) {
					LP_ERRMSG1 (ERROR, E_LP_NOTNAME, *item);
					done (1);
				}
				if (PCK_CHARSET & chkprinter(T, (char *)0, (char *)0, (char *)0, (char *)0, *item)) {
					LP_ERRMSG1 (ERROR, E_ADM_BADSET, *item);
					die = 1;
				}
				*cp++ = '=';
				if (!syn_name(cp)) {
					LP_ERRMSG1 (ERROR, E_LP_NOTNAME, cp);
					done (1);
				}
			}
			if (die) {
				LP_ERRMSG1 (ERROR, E_ADM_BADSETS, T);
				done (1);
			}
		}
	}

	/*
	 * NO MOUNT:
	 * The -f option restricts the forms that can be used with
	 * the printer.
	 *	- construct the allow/deny lists
	 *	- check each allowed form to see if it'll work
	 *	  on the printer
	 */
	if (f && !M) {
		register char		*type	= strtok(f, ":"),
					*str	= strtok((char *)0, ":"),
					**pf;

		register int		die	= 0;


		if (STREQU(type, NAME_ALLOW) && str) {
			if ((pf = f_allow = getlist(str, LP_WS, LP_SEP))){
				while (*pf) {
					if (
						!STREQU(*pf, NAME_NONE)
					     && verify_form(*pf) < 0
					)
						die = 1;
					pf++;
				}
				if (die) {
					LP_ERRMSG (ERROR, E_ADM_FORMCAPS);
					done (1);
				}

			} else
				LP_ERRMSG1 (WARNING, E_ADM_MISSING, NAME_ALLOW);

		} else if (STREQU(type, NAME_DENY) && str) {
			if (!(f_deny = getlist(str, LP_WS, LP_SEP)))
				LP_ERRMSG1 (WARNING, E_ADM_MISSING, NAME_DENY);

		} else {
			LP_ERRMSG (ERROR, E_ADM_FALLOWDENY);
			done (1);
		}
	}

	/*
	 * The -u option is setting use restrictions on printers.
	 *	- construct the allow/deny lists
	 */
	if (u) {
		register char		*type	= strtok(u, ":"),
					*str	= strtok((char *)0, ":");

		if (STREQU(type, NAME_ALLOW) && str) {
			if (!(u_allow = getlist(str, LP_WS, LP_SEP)))
				LP_ERRMSG1 (WARNING, E_ADM_MISSING, NAME_ALLOW);

		} else if (STREQU(type, NAME_DENY) && str) {
			if (!(u_deny = getlist(str, LP_WS, LP_SEP)))
				LP_ERRMSG1 (WARNING, E_ADM_MISSING, NAME_DENY);

		} else {
			LP_ERRMSG (ERROR, E_LP_UALLOWDENY);
			done (1);
		}
	}

	return;
}

/**
 ** root_can_write() - CHECK THAT "root" CAN SENSIBLY WRITE TO PATH
 **/

static int		root_can_write (path)
	char			*path;
{
	static int		lp_uid		= -1;

	struct passwd		*ppw;

	struct stat		statbuf;


	if (Stat(path, &statbuf) == -1) {
		LP_ERRMSG1 (ERROR, E_ADM_NOENT, v);
		return (-1);
	}

	if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
		LP_ERRMSG1 (WARNING, E_ADM_ISDIR, v);

	else if ((statbuf.st_mode & S_IFMT) == S_IFBLK)
		LP_ERRMSG1 (WARNING, E_ADM_ISBLK, v);

	if (lp_uid == -1) {
		if (!(ppw = getpwnam(LPUSER)))
			ppw = getpwnam(ROOTUSER);
		endpwent ();
		if (ppw)
			lp_uid = ppw->pw_uid;
		else
			lp_uid = 0;
	}
	if (STREQU(v, "/dev/null"))
		;
	else if (
		(statbuf.st_uid && statbuf.st_uid != lp_uid)
	     || (statbuf.st_mode & (S_IWGRP|S_IRGRP|S_IWOTH|S_IROTH))
	)
		LP_ERRMSG1 (WARNING, E_ADM_DEVACCESS, v);

	return (0);
}

/**
 ** unpack_sdn() - TURN SCALED TYPE INTO char* TYPE
 **/

static char		*unpack_sdn (sdn)
	SCALED			sdn;
{
	register char		*cp;
	extern char		*malloc();

	if (sdn.val <= 0 || 99999 < sdn.val)
		cp = 0;

	else if (sdn.val == 9999)
		cp = strdup(NAME_COMPRESSED);

	else if ((cp = malloc(sizeof("99999.999x"))))
		SPRINTF (cp, "%.3f%c", sdn.val, sdn.sc);

	return (cp);
}

/**
 ** verify_form() - SEE IF PRINTER CAN HANDLE FORM
 **/

int			verify_form (form)
	char			*form;
{
	register char		*cpi_f,
				*lpi_f,
				*width_f,
				*length_f,
				*chset;

	register int		rc	= 0;

	register unsigned long	checks;


	if (STREQU(form, NAME_ANY))
		form = NAME_ALL;

#define	NAME_BUG_IN_GETFORM
#if	defined(NAME_BUG_IN_GETFORM)
	formbuf.name = 0;
#endif

	while (getform(form, &formbuf, (FALERT *)0, (FILE **)0) != -1) {

#if	defined(NAME_BUG_IN_GETFORM)
		if (!formbuf.name || !*formbuf.name)
			formbuf.name = form;
#endif

		cpi_f = unpack_sdn(formbuf.cpi);
		lpi_f = unpack_sdn(formbuf.lpi);
		width_f = unpack_sdn(formbuf.pwid);
		length_f = unpack_sdn(formbuf.plen);

		if (
			formbuf.mandatory
		     && !daisy
		     && !search_cslist(
				formbuf.chset,
				(S && !M? S : (oldp? oldp->char_sets : (char **)0))
			)
		)
			chset = formbuf.chset;
		else
			chset = 0;

		if ((checks = chkprinter(
			T,
			cpi_f,
			lpi_f,
			length_f,
			width_f,
			chset
		))) {
			rc = -1;
			if ((checks & PCK_CPI) && cpi_f)
				LP_ERRMSG1 (INFO, E_ADM_BADCAP, "cpi");

			if ((checks & PCK_LPI) && lpi_f)
				LP_ERRMSG1 (INFO, E_ADM_BADCAP, "lpi");

			if ((checks & PCK_WIDTH) && width_f)
				LP_ERRMSG1 (INFO, E_ADM_BADCAP, "width");

			if ((checks & PCK_LENGTH) && length_f)
				LP_ERRMSG1 (INFO, E_ADM_BADCAP, "length");

			if ((checks & PCK_CHARSET) && formbuf.chset) {
				LP_ERRMSG1 (INFO, E_ADM_BADSET, formbuf.chset);
				rc = -2;
			}
			LP_ERRMSG2 (INFO, E_ADM_FORMCAP, formbuf.name, T);
		}

		if (!STREQU(form, NAME_ALL))
			return (rc);

	}
	if (!STREQU(form, NAME_ALL)) {
		LP_ERRMSG1 (ERROR, E_LP_NOFORM, form);
		done (1);
	}

	return (rc);
}

/*
	Second phase of parsing for -p option.
	In a seperate routine so we can call it from other
	routines. This is used when any or all are used as 
	a printer name. main() loops over each printer, and
	must call this function for each printer found.
*/
void
chkopts2(called_from_chkopts)
int	called_from_chkopts;
{
	/*
		Only do the getprinter() if we are not being called
		from lpadmin.c. Otherwise we mess up our arena for 
		"all" processing.
	*/
	if (!called_from_chkopts)
		oldp = printer_pointer;
	else if (!(oldp = getprinter(p)) && errno != ENOENT) {
		LP_ERRMSG2 (ERROR, E_LP_GETPRINTER, p, PERROR);
		done(1);
	}

	if (oldp) {
		if (
			!c && !d && !f && !M && !r && !u && !x && !A
	     		&& !strlen(modifications)
		) {
			LP_ERRMSG (ERROR, E_ADM_PLONELY);
			done (1);
		}

	} else {
		if (getclass(p)) {
			LP_ERRMSG1 (ERROR, E_ADM_PRCL, p);
			done (1);
		}

		if (!syn_name(p)) {
			LP_ERRMSG1 (ERROR, E_LP_NOTNAME, p);
			done (1);
		}

		/*
		 * New printer - if no model, use standard
		 */
		if (!(e || i || m))
			m = STANDARD;

		/*
		 * A new printer requires device or dial info.
		 */
		if (!v && !U) {
			LP_ERRMSG (ERROR, E_ADM_NOUV);
			done (1);
		}

		/*
		 * Can't quiet a new printer,
		 * can't list the alerting for a new printer.
		 */
		if (
			A
		     && (STREQU(A, NAME_QUIET) || STREQU(A, NAME_LIST))
		) {
			LP_ERRMSG1 (ERROR, E_ADM_BADQUIETORLIST, p);
			done (1);
		}
	}
}

/*
	Second phase of parsing for -S option.
	In a seperate routine so we can call it from other
	routines. This is used when any or all are used as 
	a print wheel name. main() loops over each print wheel,
	and must call this function for each print wheel found.
*/
void
chkopts3(called_from_chkopts)
int	called_from_chkopts;
{
	/*
		Only do the getpwheel() if we are not being called
		from lpadmin.c. Otherwise we mess up our arena for 
		"all" processing.
	*/
	if (!called_from_chkopts)
		oldS = pwheel_pointer;
	else
		oldS = getpwheel(*S);

	if (!oldS) {
		if (!syn_name(*S)) {
			LP_ERRMSG1 (ERROR, E_LP_NOTNAME, *S);
			done (1);
		}

		/*
		 * Can't quiet a new print wheel,
		 * can't list the alerting for a new print wheel.
		 */
		if (
			A
		     && (STREQU(A, NAME_QUIET) || STREQU(A, NAME_LIST))
		) {
			LP_ERRMSG1 (ERROR, E_ADM_BADQUIETORLIST, *S);
			done (1);
		}
	}
}
