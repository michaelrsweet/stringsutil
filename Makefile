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
prefix	=	/usr/local
includedir =	$(prefix)/include
bindir	=	$(prefix)/bin
libdir	=	$(prefix)/lib
mandir	=	$(prefix)/share/man
DESTDIR	=	$(DSTROOT)

AR	=	ar
ARFLAGS	=	crv
CC	=	gcc
CFLAGS	=	$(OPTIM) $(CPPFLAGS) -Wall
CPPFLAGS =	'-DVERSION="$(VERSION)"' `cups-config --cflags`
DOCFLAGS =	--author "Michael R Sweet" \
		--copyright "Copyright (c) 2022 by Michael R Sweet" \
		--docversion $(VERSION)
LDFLAGS	=	$(OPTIM)
LIBS	=	`cups-config --libs` -lpthread
OBJS	=	sf.o stringsutil.o
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


install:	all
	echo "Installing stringsutil to $(DESTDIR)$(bindir)..."
	mkdir -p $(DESTDIR)$(bindir)
	cp stringsutil $(DESTDIR)$(bindir)
	echo "Installing header to $(DESTDIR)$(includedir)..."
	mkdir -p $(DESTDIR)$(includedir)
	cp sf.h $(DESTDIR)$(includedir)
	echo "Installing library to $(DESTDIR)$(libdir)..."
	mkdir -p $(DESTDIR)$(libdir)
	cp libsf.a $(DESTDIR)$(libdir)
	$(RANLIB) $(DESTDIR)$(libdir)/libsf.a
	echo "Installing pkg-config info to $(DESTDIR)$(libdir)/pkgconfig..."
	mkdir -p $(DESTDIR)$(prefix)/lib/pkgconfig
	echo 'prefix="$(prefix)"' >$(DESTDIR)$(prefix)/lib/pkgconfig/libsf.pc
	echo 'Version: $(VERSION)' >>$(DESTDIR)$(prefix)/lib/pkgconfig/libsf.pc
	cat libsf.pc.in >>$(DESTDIR)$(prefix)/lib/pkgconfig/libsf.pc
	echo "Installing man pages to $(DESTDIR)$(mandir)..."
	mkdir -p $(DESTDIR)$(mandir)/man1
	cp stringsutil.1 $(DESTDIR)$(mandir)/man1
	mkdir -p $(DESTDIR)$(mandir)/man3
	cp libsf.3 $(DESTDIR)$(mandir)/man3


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

libsf.a:	sf.o
	echo "Archiving $@..."
	$(AR) $(ARFLAGS) $@ sf.o
	$(RANLIB) $@

test:		all
	rm -f test.strings
	echo "Scan test: \c"
	./stringsutil -f test.strings -n SFSTR scan $(OBJS:.o=.c) >test.log 2>&1
	if test -f test.strings -a $$(wc -l <test.strings 2>/dev/null) = 58; then \
		echo "PASS"; \
	else \
		echo "FAIL (Did not scan the expected number of strings)"; \
		cat test.log; \
		exit 1; \
	fi
	echo "Export test (C code): \c"
	./stringsutil -f test.strings export test.c >test.log 2>&1
	if test -f test.c -a $$(wc -l <test.c 2>/dev/null) = 58; then \
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
	if test -f test.po -a $$(wc -l <test.po 2>/dev/null) = 172; then \
		echo "PASS"; \
	else \
		echo "FAIL (Did not export the expected number of lines)"; \
		cat test.log; \
		exit 1; \
	fi
	echo "Report test (test-yy.strings): \c"
	if ./stringsutil -f test.strings report test-yy.strings >test.log 2>&1; then \
		echo "FAIL"; \
		cat test.log; \
		exit 1; \
	else \
		echo "PASS"; \
	fi
	echo "Report test (test-zz.strings): \c"
	if ./stringsutil -f test.strings report test-zz.strings >test.log 2>&1; then \
		echo "PASS"; \
	else \
		echo "FAIL"; \
		cat test.log; \
		exit 1; \
	fi
	echo "Import test (test-zz.po): \c"
	if ./stringsutil -f test.strings import test-zz.po >test.log 2>&1; then \
		if test $$(wc -l <test.strings 2>/dev/null) = 58; then \
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
		if test $$(wc -l <test.strings 2>/dev/null) = 60; then \
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
		if test $$(wc -l <test.strings 2>/dev/null) = 60; then \
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
	echo "Localization test (es): \c"
	if (LANG=es_ES.UTF-8 ./stringsutil --help | grep -q NOMBRE); then \
		echo "PASS"; \
	else \
		echo "FAIL"; \
		LANG=es_ES.UTF-8 ./stringsutil --help; \
	fi
	echo "Localization test (fr): \c"
	if (LANG=fr_CA.UTF-8 ./stringsutil --help | grep -q NOM); then \
		echo "PASS"; \
	else \
		echo "FAIL"; \
		LANG=fr_CA.UTF-8 ./stringsutil --help; \
	fi
	rm -f test.c test.log test.o test.po test.strings
	echo "All tests passed."


# Update localizations
update:		stringsutil
	echo Updating strings files...
	rm -f base.strings
	./stringsutil -f base.strings -n SFSTR scan stringsutil.c
	./stringsutil -f es.strings merge base.strings
	./stringsutil -f fr.strings merge base.strings

update2:	stringsutil
	./stringsutil -f es.strings export es_strings.h
	./stringsutil -f fr.strings export fr_strings.h


# Make documentation
doc:
	echo Generating stringsutil.html...
	codedoc $(DOCFLAGS) --title "StringsUtil v$(VERSION) Manual" --coverimage stringsutil-512.png --body stringsutil.md stringsutil.xml sf.[ch] >stringsutil.html
	echo Generating libsf.3...
	codedoc $(DOCFLAGS) --title "stringsutil - libsf functions" --man libsf --section 3 --body libsf.md stringsutil.xml >libsf.3
	rm -f stringsutil.xml


$(OBJS):	sf.h sf-private.h Makefile
stringsutil.o:	es_strings.h fr_strings.h
