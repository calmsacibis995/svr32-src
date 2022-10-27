/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)vm.vt:src/vt/vcurrent.c	1.6";

#include	<curses.h>
#include	"wish.h"
#include	"vt.h"
#include	"vtdefs.h"

/* vt which is in "front" of others (and head of linked list) */
vt_id	VT_front;
/* tail of linked list */
vt_id	VT_back;
/* vt which is "current" (ie operations default to this one) */
vt_id	VT_curid;
struct vt	*VT_array;

extern int Color_terminal;

/*
 * makes the given vt current and in front of all others (also makes
 * old vt noncurrent if there is a current one
 */
vt_id
vt_current(vid)
vt_id	vid;
{
	register vt_id	n;
	register vt_id	oldvid;
	register struct vt	*v;
	struct	vt *curvt;

/* debug stuff

	fprintf( stderr, "\t\t\t\t\tInto vt_current( %d )\n", vid );

	fprintf( stderr, "VT_front = %d\n", VT_front );
	fprintf( stderr, "VT_back =  %d\n", VT_back );
	fprintf( stderr, "VT_curid = %d\n\n", VT_curid );

        for ( n = VT_front; n != VT_UNDEFINED; n = v->next )
	{
		v = &VT_array[ n ];

		fprintf( stderr, "prev = %d\n", v->prev );
		fprintf( stderr, "VT_array index = %d\n", n );
		fprintf( stderr, "next = %d\n\n", v->next );
	}
/**/

	if ( VT_curid == vid && VT_front == vid )
		return VT_curid;

/*
 * makes current vt noncurrent
 */
	if (VT_curid >= 0) {
		curvt = &VT_array[VT_curid];
		curvt->flags |= VT_TDIRTY;
		/*
		 * Since active/inactive border colors can be specified
		 * for color terminals, border should also be marked dirty
		 * on NON-currency.
		 */
		if ((!(curvt->flags & VT_NOBORDER)) && Color_terminal == TRUE)
			curvt->flags |= VT_BDIRTY;	
	}

/*
 * moves vt to front (without making it current)
 */

	if (VT_front != vid)
	{
		for (n = VT_front; n != VT_UNDEFINED; n = v->next)
		{
			v = &VT_array[n];
	
			if (v->next == vid)
			{
				v->next = VT_array[vid].next;
	
				if ( VT_back == vid )
					VT_back = VT_array[ VT_back ].prev;
				else
				{
					v = &VT_array[ vid ];
					VT_array[ v->next ].prev = n;
				}
	
				break;
			}
		}

		v = &VT_array[vid];
		v->flags |= VT_BDIRTY;
		VT_array[vid].next = VT_front;
		VT_array[ vid ].prev = VT_UNDEFINED;
	
		if ( VT_front != VT_UNDEFINED )
			VT_array[ VT_front ].prev = vid;
	
		VT_front = vid;
	}

/*
 * makes vt current without moving it to front
 */
	oldvid = VT_curid;
	v = &VT_array[VT_curid = vid];
	v->flags |= VT_TDIRTY;
	/*
	 * Since active/inactive border colors can be specified
	 * for color terminals, border should also be marked dirty
	 * on NON-currency.
	 */
	if ((!(v->flags & VT_NOBORDER)) && Color_terminal == TRUE)
		v->flags |= VT_BDIRTY;	

/* debug stuff

	fprintf( stderr, "\t\t\tAfter change\n" );
	fprintf( stderr, "VT_front = %d\n", VT_front );
	fprintf( stderr, "VT_back =  %d\n", VT_back );
	fprintf( stderr, "VT_curid = %d\n\n", VT_curid );

        for ( n = VT_front; n != VT_UNDEFINED; n = v->next )
	{
		v = &VT_array[ n ];

		fprintf( stderr, "prev = %d\n", v->prev );
		fprintf( stderr, "VT_array index = %d\n", n );
		fprintf( stderr, "next = %d\n\n", v->next );
	}
/**/
	return oldvid;
}

/* used for debugging (LES)

pr_VT_array()
{
	FILE	*fp, *fopen();
	struct vt	*v;
	int	n;

        fp = fopen( "VT_ARRAY", "a" );

	fprintf( fp, "\nVT_front = %d\n", VT_front );
	fprintf( fp, "VT_back =  %d\n", VT_back );
	fprintf( fp, "VT_curid = %d\n\n", VT_curid );

        for ( n = VT_front; n != VT_UNDEFINED; n = v->next )
	{
		v = &VT_array[ n ];

		fprintf( fp, "VT_array index = %d\n", n );
		fprintf( fp, "next = %d\n", v->next );
		fprintf( fp, "prev = %d\n\n", v->prev );
	}

	fclose( fp );
}
*/
