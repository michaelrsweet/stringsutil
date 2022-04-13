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

.SILENT:
.SUFFIXES:	.c .o
.c.o:
	echo "Compiling $<..."
	$(CC) $(CFLAGS) -c -o $@ $<


all:		$(TARGETS)


clean:
	rm -f $(TARGETS) $(OBJS)


install:	$(TARGETS)
	echo "Installing stringsutil to $(bindir)..."
	mkdir -p $(bindir)
	cp stringsutil $(bindir)
	echo "Installing header to $(includedir)..."
	mkdir -p $(includedir)
	cp strings-file.h $(includedir)
	echo "Installing library to $(libdir)..."
	mkdir -p $(libdir)
	cp libsf.a $(libdir)
	$(RANLIB) $(libdir)/libsf.a
	echo "Installing man pages to $(mandir)..."
	mkdir -p $(mandir)/man1
	cp stringsutil.1 $(mandir)/man1


sanitizer:
	$(MAKE) clean
	$(MAKE) OPTIM="-g -fsanitize=address" all


# Analyze code with the Clang static analyzer <https://clang-analyzer.llvm.org>
clang:
	echo "Analyzing code with Clang..."
	clang $(CPPFLAGS) --analyze $(OBJS:.o=.c) 2>clang.log
	rm -rf $(OBJS:.o=.plist)
	test -s clang.log && (echo "$(GHA_ERROR)Clang detected issues."; echo ""; cat clang.log; exit 1) || exit 0


# Analyze code using Cppcheck <http://cppcheck.sourceforge.net>
cppcheck:
	echo "Analyzing code with cppcheck..."
	cppcheck $(CPPFLAGS) --template=gcc --addon=cert.py --suppressions-list=.cppcheck $(OBJS:.o=.c) 2>cppcheck.log
	test -s cppcheck.log && (echo "$(GHA_ERROR)Cppcheck detected issues."; echo ""; cat cppcheck.log; exit 1) || exit 0

# Make various bits...
stringsutil:	stringsutil.o libsf.a
	echo "Linking $@..."
	$(CC) $(LDFLAGS) -o stringsutil stringsutil.o libsf.a $(LIBS)

libsf.a:	strings-file.o
	echo "Archiving $@..."
	$(AR) $(ARFLAGS) $@ strings-file.o
	$(RANLIB) $@

test:		stringsutil
	rm -f test.strings
	echo "Scan test: \c"
	./stringsutil -f test.strings -n SFSTR scan $(OBJS:.o=.c) >test.log 2>&1
	if test -f test.strings -a $$(wc -l <test.strings 2>/dev/null) = 42; then \
		echo "PASS"; \
	else \
		echo "FAIL (Did not scan the expected number of strings)"; \
		cat test.log; \
		exit 1; \
	fi
	echo "Export test (C code): \c"
	./stringsutil -f test.strings export test.c >test.log 2>&1
	if test -f test.c -a $$(wc -l <test.c 2>/dev/null) = 42; then \
		echo "PASS"; \
	else \
		echo "FAIL (Did not export the expected number of strings)"; \
		cat test.log; \
		exit 1; \
	fi
	echo "Compile test: \c"
	if gcc -o test.o -c test.c >test.log 2>&1; then \
		echo "PASS"; \
	else \
		echo "FAIL"; \
		cat test.log; \
		exit 1; \
	fi
	echo "Export test (GNU gettext po): \c"
	./stringsutil -f test.strings export test.po >test.log 2>&1
	if test -f test.po -a $$(wc -l <test.po 2>/dev/null) = 124; then \
		echo "PASS"; \
	else \
		echo "FAIL (Did not export the expected number of lines)"; \
		cat test.log; \
		exit 1; \
	fi
	echo "Report test: \c"
	if ./stringsutil -f test.strings report test-zz.strings >test.log 2>&1; then \
		echo "PASS"; \
	else \
		echo "FAIL"; \
		cat test.log; \
		exit 1; \
	fi
	echo "Import test (test-zz.po): \c"
	if ./stringsutil -f test.strings import test-zz.po >test.log 2>&1; then \
		if test $$(wc -l <test.strings 2>/dev/null) = 42; then \
			echo "PASS"; \
		else \
			echo "FAIL (did not preserve strings)"; \
			cat test.log; \
			exit 1; \
		fi \
	else \
		echo "FAIL"; \
		cat test.log; \
		exit 1; \
	fi
	echo "Import test (test-zz.po -a): \c"
	if ./stringsutil -f test.strings import -a test-zz.po >test.log 2>&1; then \
		if test $$(wc -l <test.strings 2>/dev/null) = 44; then \
			echo "PASS"; \
		else \
			echo "FAIL (did not add new strings)"; \
			cat test.log; \
			exit 1; \
		fi \
	else \
		echo "FAIL"; \
		cat test.log; \
		exit 1; \
	fi
	echo "Import test (test-zz.strings): \c"
	if ./stringsutil -f test.strings import test-zz.strings >test.log 2>&1; then \
		if test $$(wc -l <test.strings 2>/dev/null) = 44; then \
			echo "PASS"; \
		else \
			echo "FAIL (did not preserve strings)"; \
			cat test.log; \
			exit 1; \
		fi \
	else \
		echo "FAIL"; \
		cat test.log; \
		exit 1; \
	fi
	rm -f test.c test.log test.o test.po test.strings
	echo "All tests passed."


$(OBJS):	strings-file.h strings-file-private.h Makefile
