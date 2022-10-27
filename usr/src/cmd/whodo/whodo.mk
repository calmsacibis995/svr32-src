#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)whodo:whodo.mk	1.9.1.1"
#       whodo make file

ROOT =
INC = $(ROOT)/usr/include
INSDIR = $(ROOT)/etc
CFLAGS = -O
LDFLAGS = -s
INS=install

all:	whodo

whodo:
	$(CC) -I$(INC) $(CFLAGS) -o whodo whodo.c $(LDFLAGS) $(LDLIBS)

install:	all
	$(INS) -f $(INSDIR) -m 2755 -g sys whodo

clean:

clobber:	clean
	rm -f whodo
