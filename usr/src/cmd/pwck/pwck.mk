#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pwck:pwck.mk	1.2"

#	Makefile for pwck

ROOT =

DIR = $(ROOT)/etc

INC = $(ROOT)/usr/include

LDFLAGS = -s $(LDLIBS)

CFLAGS = -O -I$(INC)

STRIP = strip

SIZE = size

#top#

MAKEFILE = pwck.mk

MAINS = pwck

OBJECTS =  pwck.o

SOURCES =  pwck.c

ALL:		$(MAINS)

$(MAINS):	pwck.o
		$(CC) $(CFLAGS) -o pwck pwck.o $(LDFLAGS)

pwck.o:		 $(INC)/sys/types.h \
		 $(INC)/sys/param.h \
		 $(INC)/sys/fs/s5param.h \
		 $(INC)/sys/signal.h \
		 $(INC)/sys/sysmacros.h \
		 $(INC)/sys/stat.h \
		 $(INC)/stdio.h \
		 $(INC)/ctype.h 

GLOBALINCS = $(INC)/ctype.h \
	$(INC)/stdio.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/param.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h 


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

all : ALL

install: ALL
	cpset $(MAINS) $(DIR)

size: ALL
	$(SIZE) $(MAINS)

strip: ALL
	$(STRIP) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(DIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(DIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)



