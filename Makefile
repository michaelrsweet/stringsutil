#
# Makefile for StringsUtil.
#
#     https://github.com/michaelrsweet/stringsutil
#
# Copyright © 2022 by Michael R Sweet.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

VERSION	=	1.0
prefix	=	$(DESTDIR)/usr/local
includedir =	$(prefix)/include
bindir	=	$(prefix)/bin
libdir	=	$(prefix)/lib
mandir	=	$(prefix)/share/man

AR	=	ar
ARFLAGS	=	crv
CC	=	gcc
CFLAGS	=	$(OPTIM) $(CPPFLAGS) -Wall
CPPFLAGS =	'-DVERSION="$(VERSION)"' `cups-config --cflags`
LDFLAGS	=	$(OPTIM)
LIBS	=	`cups-config --libs`
OBJS	=	strings-file.o stringsutil.o
OPTIM	=	-Os -g
RANLIB	=	ranlib
TARGETS	=	libsf.a stringsutil

.SUFFIXES:	.c .o
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<


all:		$(TARGETS)


clean:
	rm -f $(TARGETS) $(OBJS)


install:	$(TARGETS)
	mkdir -p $(bindir)
	cp stringsutil $(bindir)
	mkdir -p $(includedir)
	cp strings-file.h $(includedir)
	mkdir -p $(libdir)
	cp libsf.a $(libdir)
	$(RANLIB) $(libdir)/libsf.a
	mkdir -p $(mandir)/man1
	cp stringsutil.1 $(mandir)/man1


sanitizer:
	$(MAKE) clean
	$(MAKE) OPTIM="-g -fsanitize=address" all


# Analyze code with the Clang static analyzer <https://clang-analyzer.llvm.org>
clang:
	clang $(CPPFLAGS) --analyze $(OBJS:.o=.c) 2>clang.log
	rm -rf $(OBJS:.o=.plist)
	test -s clang.log && (echo "$(GHA_ERROR)Clang detected issues."; echo ""; cat clang.log; exit 1) || exit 0


# Analyze code using Cppcheck <http://cppcheck.sourceforge.net>
cppcheck:
	cppcheck $(CPPFLAGS) --template=gcc --addon=cert.py --suppressions-list=.cppcheck $(OBJS:.o=.c) 2>cppcheck.log
	test -s cppcheck.log && (echo "$(GHA_ERROR)Cppcheck detected issues."; echo ""; cat cppcheck.log; exit 1) || exit 0

# Make various bits...
stringsutil:	stringsutil.o libsf.a
	$(CC) $(LDFLAGS) -o stringsutil stringsutil.o libsf.a $(LIBS)

libsf.a:	strings-file.o
	$(AR) $(ARFLAGS) $@ strings-file.o
	$(RANLIB) $@

test:

$(OBJS):	strings-file.h strings-file-private.h Makefile
