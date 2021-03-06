#	Copyright (c) 1984, 1986, 1987, 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nm:nm.mk	2.7"

#	nm.mk nm makefile

ROOT	=
BINDIR	= $(ROOT)/bin
#
# CFLAGS should also be passed down, but this won't be done until
# all the makefiles have been standardized
#

all nm:
	-if vax; then \
		cd vax; $(MAKE) -$(MAKEFLAGS) CC="$(CC)"; \
	elif pdp11; then \
		cd pdp; $(MAKE) -$(MAKEFLAGS) CC="$(CC)"; \
	elif u3b; then \
		cd u3b; $(MAKE) -$(MAKEFLAGS) CC="$(CC)"; \
	else \
		echo 'Cannot make nm command: unknown target procesor.'; \
	fi



clean:
	-if vax; then \
		cd vax; $(MAKE) -$(MAKEFLAGS) clean; \
	elif pdp11; then \
		cd pdp; $(MAKE) -$(MAKEFLAGS) clean; \
	elif u3b; then \
		cd u3b; $(MAKE) -$(MAKEFLAGS) clean; \
	fi


install: 
	-if vax; then \
		cd vax; $(MAKE) -$(MAKEFLAGS) CC="$(CC)" BINDIR=$(BINDIR) install; \
	elif u3b; then \
		cd u3b; $(MAKE) -$(MAKEFLAGS) CC="$(CC)" BINDIR=$(BINDIR) install; \
	elif pdp11; then \
		cd pdp; $(MAKE) -$(MAKEFLAGS) CC="$(CC)" BINDIR=$(BINDIR) install; \
	fi

clobber: 
	-if vax; then \
		cd vax; $(MAKE) -$(MAKEFLAGS) clobber; \
	elif u3b; then \
		cd u3b; $(MAKE) -$(MAKEFLAGS) clobber; \
	elif pdp11; then \
		cd pdp; $(MAKE) -$(MAKEFLAGS) clobber; \
	fi
