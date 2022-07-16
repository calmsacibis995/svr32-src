/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:cmd/lpsched/dowait.c	1.6"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "lpsched.h"
#include "ctype.h"
#include "sys/stat.h"

/*
 * Macro to test if we should notify the user.
 */
#define SHOULD_NOTIFY(PRS) \
	( \
		(PRS)->request->actions & (ACT_MAIL|ACT_WRITE) \
	     || (PRS)->request->alert \
	)

static char		*geterrbuf();

/**
 ** dowait() - WAIT FOR CHILD THAT HAS FINISHED, RESCHEDULE ANOTHER TASK
 **/

void
dowait ()
{
	int			exited,
				killed,
				canned,
				i;

	EXEC			*ep;

	char			*errbuf;

	struct stat		stbuf;
	
	register RSTATUS	*prs;

	register PSTATUS	*pps;

	register ALERT		*pas;


	TRACE_ON ("dowait");

	while (DoneChildren > 0) {
		DoneChildren--;

		for (i = 0; i < ET_Size; i++)
			if (Exec_Table[i].pid == -99)
				break;
		if (i >= ET_Size)
			continue;

		ep = Exec_Table + i;
		ep->pid = 0;
		ep->key = 0;	/* avoid subsequent sneaks */

		killed = KILLED(ep->status);
		exited = EXITED(ep->status);

		switch (ep->type) {

		case EX_INTERF:
			/*
			 * WARNING: It could be that when we get here
			 *
			 *	pps->request->printer != pps
			 *
			 * because the request has been assigned to
			 * another printer.
			 */
			pps = ep->ex.printer;
			prs = pps->request;
			pps->request = 0;
			pps->status &= ~PS_BUSY;

			/*
			 * If the interface program exited cleanly
			 * or with just a user error, the printer
			 * is assumed to be working.
			 */
			if (0 <= exited && exited < EXEC_EXIT_USER) {
				pps->status &= ~PS_FAULTED;
				if (pps->alert->active)
					cancel_alert (A_PRINTER, pps);
			}

			/*
			 * If the interface program was killed with
			 * SIGTERM, it may have been because we canceled
			 * the request, disabled the printer, or for some
			 * other reason stopped the request.
			 * If so, clear the "killed" flag because that's
			 * not the condition of importance here.
			 */
			canned = 0;
			if (killed == SIGTERM) {
				if (prs->request->outcome & RS_CANCELLED)
					canned = 1;

				if (
					canned
				     || pps->status & (PS_DISABLED|PS_FAULTED)
				     || prs->request->outcome & RS_STOPPED
#if	defined(CHECK_CHILDREN)
				     || ep->flags & EXF_GONE
#endif
# if defined(NEW_LPSHUT)
				     || Shutdown
# endif
				)
					killed = 0;
			}
					
			/*
			 * If there was standard error output from the
			 * interface program, or if the interface program
			 * exited with a (user) exit code, the user should
			 * be notified.
			 * NOTE: Ignore error output with no printable
			 * text. This hides problems we have with some
			 * shell scripts that occasionally cause spurious
			 * newlines when stopped via SIGTERM. Without
			 * this check for non-blank output, stopping
			 * a request sometimes causes a request failure.
			 */
			errbuf = geterrbuf(prs);
			if (
				errbuf
			     || (0 < exited && exited <= EXEC_EXIT_USER)
			     || killed
			) {
				prs->request->outcome |= RS_FAILED;
				prs->request->outcome |= RS_NOTIFY;
				notify (prs, errbuf, killed, exited, 0);
				if (errbuf)
					free (errbuf);


			/*
			 * If the request was canceled, call "notify()"
			 * in case we're to notify the user.
			 */
			} else if (canned) {
				if (SHOULD_NOTIFY(prs))
					prs->request->outcome |= RS_NOTIFY;
				notify (prs, (char *)0, 0, 0, 0);

			/*
			 * If the request finished successfully, call
			 * "notify()" in case we're to notify the user.
			 */
			} else if (exited == 0) {
				prs->request->outcome |= RS_PRINTED;

				if (SHOULD_NOTIFY(prs))
					prs->request->outcome |= RS_NOTIFY;
				notify (prs, (char *)0, 0, 0, 0);
			}

			/*
			 * If the interface program exits with an
			 * exit code higher than EXEC_EXIT_USER, it's
			 * a special case. The "if" starts again
			 * because of possible overlapping cases
			 * (e.g. standard error AND fault).
			 */

			if (-1 <= exited && exited <= EXEC_EXIT_USER) {
				;

			} else if (exited == EXEC_EXIT_FAULT) {
				printer_fault (pps, prs, 0, 0);

			} else if (exited == EXEC_EXIT_HUP) {
				printer_fault (pps, prs, HANGUP_FAULT, 0);

			} else if (exited == EXEC_EXIT_INTR) {
				printer_fault (pps, prs, INTERRUPT_FAULT, 0);

			} else if (exited == EXEC_EXIT_PIPE) {
				printer_fault (pps, prs, PIPE_FAULT, 0);

			} else if ((exited & EXEC_EXIT_NMASK) == EXEC_EXIT_NDIAL) {
				dial_problem (pps, prs, exited & ~EXEC_EXIT_NMASK);

			} else if (exited == EXEC_EXIT_TMOUT) {
				printer_fault (pps, prs, TIMEOUT_FAULT, 0);

			} else if (exited == EXEC_EXIT_NPORT) {
				printer_fault (pps, prs, OPEN_FAULT, ep->errno);

			} else if (exited == EXEC_EXIT_EXIT) {
				note (BAD_EXIT_FROM_INTERFACE, pps->printer->name, ep->errno);
				printer_fault (pps, prs, EXIT_FAULT, 0);

			} else if (exited == EXEC_EXIT_NOPEN) {
				errno = ep->errno;
				note (EXEC_NOPEN, PERROR);

			} else if (exited == EXEC_EXIT_NEXEC) {
				errno = ep->errno;
				note (EXEC_FAILED, PERROR);

			} else if (exited == EXEC_EXIT_NOMEM) {
				fail (MEMORY_ALLOC_FAILED);

			} else if (exited == EXEC_EXIT_NFORK) {
				errno = ep->errno;
				note (CHILD_FORK_FAILED, PERROR);

			} else {
				note (BAD_EXIT_FROM_EXEC, pps->printer->name, exited);

			}

			/*
			 * Being in the "dowait()" routine means the
			 * interface (and fast filter!) have stopped.
			 * If we have a fault and we're expected to try
			 * again later, make sure we try again later.
			 */
			if (
				(pps->status & PS_FAULTED)
			     && !STREQU(pps->printer->fault_rec, NAME_WAIT)
			     && !(pps->status & (PS_LATER|PS_DISABLED))
			) {
				load_str (&pps->dis_reason, CUZ_STOPPED);
				schedule (EV_LATER, WHEN_PRINTER, EV_INTERF, pps);
			}

			prs->request->outcome &= ~(RS_PRINTING|RS_STOPPED);

			/*
			 * If the printer to which this request was
			 * assigned is not able to handle requests now,
			 * push waiting requests off on to another
			 * printer.
			 */
			if (prs->printer->status & (PS_FAULTED|PS_DISABLED|PS_LATER))
				(void)queue_repel (prs->printer, 0, (int (*)())0);

			/*
			 * If the request is now assigned to a different
			 * printer, call "schedule()" to fire up an
			 * interface. If this request also happens to
			 * be dead, or in need of refiltering, it won't
			 * get scheduled.
			 */
			if (
				prs->printer != pps
#if	defined(CHECK_CHILDREN)
			     || ep->flags & EXF_GONE
#endif
			)
				schedule (EV_INTERF, prs->printer);

			check_request (prs);

			/*
			 * Attract the FIRST request that is waiting to
			 * print to this printer, unless the printer isn't
			 * ready to print another request. We do this
			 * even though requests may already be assigned
			 * to this printer, because a request NOT assigned
			 * might be ahead of them in the queue.
			 */
			if (!(pps->status & (PS_FAULTED|PS_DISABLED|PS_LATER)))
				queue_attract (pps, qchk_waiting);

			break;
		
		case EX_SLOWF:
			prs = ep->ex.request;
			ep->ex.request = 0;
			prs->exec = 0;
			prs->request->outcome &= ~RS_FILTERING;

			/*
			 * If the slow filter was killed with SIGTERM,
			 * it may have been because we canceled the
			 * request, stopped the filtering, or put a
			 * change hold on the request. If so, clear
			 * the "killed" flag because that's not the
			 * condition of importance.
			 */
			canned = 0;
			if (killed == SIGTERM){
				if (prs->request->outcome & RS_CANCELLED)
					canned = 1;

				if (
					canned
				     || prs->request->outcome & RS_STOPPED
#if	defined(CHECK_CHILDREN)
				     || ep->flags & EXF_GONE
#endif
#if defined(NEW_LPSHUT)
				     || Shutdown
#endif
				)
					killed = 0;
			}
					
			/*
			 * If there was standard error output from the
			 * slow filter, or if the interface program exited
			 * with a non-zero exit code, the user should
			 * be notified.
			 * NOTE: Ignore error output with no printable
			 * text. This hides problems we have with some
			 * shell scripts that occasionally cause spurious
			 * newlines when stopped via SIGTERM. Without
			 * this check for non-blank output, stopping
			 * a request sometimes causes a request failure.
			 */
			errbuf = geterrbuf(prs);
			if (
				errbuf
			     || 0 < exited && exited <= EXEC_EXIT_USER
			     || killed
			) {
				prs->request->outcome |= RS_FAILED;
				prs->request->outcome |= RS_NOTIFY;
				notify (prs, errbuf, killed, exited, 1);
				if (errbuf)
					free (errbuf);


			/*
			 * If the request was canceled, call "notify()"
			 * in case we're to notify the user.
			 */
			} else if (canned) {
				if (SHOULD_NOTIFY(prs))
					prs->request->outcome |= RS_NOTIFY;
				notify (prs, (char *)0, 0, 0, 1);

			/*
			 * If the slow filter exited normally, mark
			 * the request as finished slow filtering.
			 */
			} else if (exited == 0) {
				prs->request->outcome |= RS_FILTERED;

			} else if (exited == -1) {
				;

			} else if (exited == EXEC_EXIT_NOPEN) {
				errno = ep->errno;
				note (EXEC_NOPEN, PERROR);

			} else if (exited == EXEC_EXIT_NEXEC) {
				errno = ep->errno;
				note (EXEC_FAILED, PERROR);

			} else if (exited == EXEC_EXIT_NOMEM) {
				fail (MEMORY_ALLOC_FAILED);

			}

			prs->request->outcome &= ~RS_STOPPED;

			schedule (EV_INTERF, prs->printer);
			if (
				prs->request->outcome & RS_REFILTER
#if	defined(CHECK_CHILDREN)
			     || ep->flags & EXF_GONE
#endif
			)
				schedule (EV_SLOWF, prs);
			else
				schedule (EV_SLOWF, (RSTATUS *)0);

			check_request (prs);
			break;
		
		case EX_NOTIFY:
			prs = ep->ex.request;
			ep->ex.request = 0;
			prs->exec = 0;

			prs->request->outcome &= ~RS_NOTIFYING;
#if	defined(CHECK_CHILDREN)
			if (!(ep->flags & EXF_GONE))
#endif
#if defined(NEW_LPSHUT)
			    if (!Shutdown || !killed)
#endif
				prs->request->outcome &= ~RS_NOTIFY;

#if	defined(CHECK_CHILDREN)
			if (ep->flags & EXF_GONE)
				schedule (EV_NOTIFY, prs);
			else
#endif
				schedule (EV_NOTIFY, (RSTATUS *)0);

			check_request (prs);
			break;
		
		case EX_ALERT:
			pas = ep->ex.printer->alert;
			goto CleanUpAlert;

		case EX_FALERT:
			pas = ep->ex.form->alert;
			goto CleanUpAlert;

		case EX_PALERT:
			pas = ep->ex.pwheel->alert;
			/*
			 * CAUTION: It may well be that we've removed
			 * the print wheel by the time we get here.
			 * Only the alert structure (and exec structure)
			 * can be considered okay.
#if	defined(CHECK_CHILDREN)
			 * For this reason, we ignore the EXF_GONE flag,
			 * that is, we don't try to reschedule a lost
			 * alert.
#endif
			 */
#if	defined(CHECK_CHILDREN)
			ep->flags &= ~EXF_GONE;
#endif

CleanUpAlert:
#if defined(NEW_LPSHUT)
			if (Shutdown)
				break;
#endif
#if	defined(CHECK_CHILDREN)
			if (ep->flags & (EXF_RESTART|EXF_GONE)) {
#else
			if (ep->flags & EXF_RESTART) {
#endif
				ep->flags &= ~(EXF_RESTART|EXF_GONE);
				if (exec(ep->type, ep->ex.form) == 0) {
					pas->active = 1;
					break;
				}
			}
			(void)Unlink (pas->msgfile);
			break;
		
		}
	}

	TRACE_OFF ("dowait");
	return;
}

/**
 ** geterrbuf() - READ NON-BLANK STANDARD ERROR OUTPUT
 **/

static char		*geterrbuf (prs)
	register RSTATUS	*prs;
{
	register char		*cp;

	int                     fd,
				n;
	
	char                    *buf    = 0,
				*file;

	struct stat             statbuf;


	file = makereqerr(prs->secure->req_id);
	if (
		Stat(file, &statbuf) == 0
	     && statbuf.st_size
	     && (fd = Open(file, O_RDONLY)) != -1
	) {
		if ((buf = malloc(statbuf.st_size + 1)))
			if ((n = Read(fd, buf, statbuf.st_size)) > 0) {
				buf[n] = 0;
				for (cp = buf; *cp && isspace(*cp); cp++)
					;
				if (!*cp) {
					free (buf);
					buf = 0;
				}
			} else {
				free (buf);
				buf = 0;
			}
		Close(fd);
	}
	if (file)
		free (file);

	return (buf);
}

/**
 ** check_request() - CLEAN UP AFTER REQUEST
 **/

void			check_request (prs)
	RSTATUS			*prs;
{
	/*
	 * If the request is done, decrement the count of requests
	 * needing the form or print wheel. Update the disk copy of
	 * the request. If we're finished with the request, get rid of it.
	 */
	if (prs->request->outcome & RS_DONE) {
		unqueue_form (prs);
		unqueue_pwheel (prs);
		putrequest (prs->req_file, prs->request);
		if (!(prs->request->outcome & (RS_ACTIVE|RS_NOTIFY))) {
			rmfiles (prs, 1);
			freerstatus (prs);
		}
	}
	return;
}

/**
 ** check_children()
 **/

#if defined(NEW_LPSHUT)
check_children()
{
	register int		i;
    
	for (i = 0; i < ET_Size; i++)
		if (Exec_Table[i].pid > 0)
			break;

	if (i >= ET_Size)
		Shutdown = 2;
}
#endif
