/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)vm.oh:src/oh/objhelp.h	1.1"

typedef struct {
	int flags;
	struct fm_mn fm_mn;
	char *holdptr;
	int *slks;
	char **args;
} helpinfo;
