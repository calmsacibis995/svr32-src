/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libld:common/ldfhread.c	1.3"
#include    <stdio.h>
#include    "filehdr.h"
#include    "ldfcn.h"

int
ldfhread(ldptr, filehead)

LDFILE    *ldptr;
FILHDR    *filehead; 

{
    extern int		fread( );
    extern int		fseek( );

    extern int		vldldptr( );

    if (vldldptr(ldptr) == SUCCESS) {
	if (FSEEK(ldptr, 0L, BEGINNING) == OKFSEEK) {
	    if (FREAD(filehead, FILHSZ, 1, ldptr) == 1) {
	    	return(SUCCESS);
	    }
	}
    }

    return(FAILURE);
}

