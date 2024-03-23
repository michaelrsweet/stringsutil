dnl
dnl Configuration script for StringsUtil.
dnl
dnl Copyright © 2024 by Michael R Sweet.
dnl
dnl Licensed under Apache License v2.0.  See the file "LICENSE" for more
dnl information.
dnl


dnl ***********************************************************************
dnl
dnl Note: Using autoheader or automake on this project will break the
dnl StringsUtil build system.  Use "autoconf -f" to regenerate the
dnl configure script if you make changes to this file.
dnl
dnl ***********************************************************************


dnl We need at least autoconf 2.69 for the AS_HELP_STRING macro...
AC_PREREQ([2.69])


dnl Package name and version...
AC_INIT([StringsUtil], [1.1], [https://github.com/michaelrsweet/stringsutil/issues], [stringsutil], [https://www.msweet.org/stringsutil])

VERSION="AC_PACKAGE_VERSION"
AC_SUBST([VERSION])


dnl Get the operating system and version number...
dnl Get the build and host platforms and split the host_os value
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

[host_os_name=`echo $host_os | sed -e '1,$s/[0-9.]*$//g'`]
[host_os_version=`echo $host_os | sed -e '1,$s/^[^0-9.]*//g' | awk -F. '{print $1 $2}'`]
# Linux often does not yield an OS version we can use...
AS_IF([test "x$host_os_version" = x], [
    host_os_version="0"
])


dnl Compiler options...
CFLAGS="${CFLAGS:=}"
CPPFLAGS="${CPPFLAGS:=}"
LDFLAGS="${LDFLAGS:=}"
LIBS="${LIBS:=}"
OPTIM="${OPTIM:=}"

AC_SUBST([OPTIM])


dnl Standard programs...
AC_PROG_CC
AC_PROG_RANLIB
AC_PATH_PROG([AR], [ar])
AC_PATH_PROGS([CODE_SIGN], [codesign true])
AC_PATH_PROG([MKDIR], [mkdir])
AC_PATH_PROG([RM], [rm])
AC_PATH_PROG([RMDIR], [rmdir])
AC_PATH_PROG([LN], [ln])


dnl Figure out the correct "ar" command flags...
AS_IF([test "$ac_cv_prog_ranlib" = ":"], [
    ARFLAGS="crs"
], [
    ARFLAGS="cr"
])
AC_SUBST([ARFLAGS])


dnl install-sh
AC_MSG_CHECKING([for install-sh script])
INSTALL="$(pwd)/install-sh"
AC_SUBST([INSTALL])
AC_MSG_RESULT([using $INSTALL])


dnl Check for pkg-config, which is used for some other tests later on...
AC_PATH_TOOL([PKGCONFIG], [pkg-config])

PKGCONFIG_CFLAGS="-I\${includedir}"
PKGCONFIG_LIBS="-L\${libdir} -lsf"
PKGCONFIG_LIBS_PRIVATE=""
PKGCONFIG_REQUIRES=""
AC_SUBST([PKGCONFIG_CFLAGS])
AC_SUBST([PKGCONFIG_LIBS])
AC_SUBST([PKGCONFIG_LIBS_PRIVATE])
AC_SUBST([PKGCONFIG_REQUIRES])


dnl CUPS
AC_PATH_TOOL([CUPSCONFIG], [cups-config])
AC_MSG_CHECKING([for CUPS library v2.2 or higher])
AS_IF([$PKGCONFIG --exists cups3], [
    CUPS_VERSION="$($PKGCONFIG --modversion cups3)"
    AC_MSG_RESULT([yes, v$CUPS_VERSION])

    CPPFLAGS="$CPPFLAGS $($PKGCONFIG --cflags cups3)"
    LIBS="$LIBS $($PKGCONFIG --libs cups3)"
], [$PKGCONFIG --exists cups], [
    CUPS_VERSION="$($PKGCONFIG --modversion cups)"
    AC_MSG_RESULT([yes, v$CUPS_VERSION])

    CPPFLAGS="$CPPFLAGS $($PKGCONFIG --cflags cups)"
    LIBS="$LIBS $($PKGCONFIG --libs cups)"
], [
    AS_IF([test "x$CUPSCONFIG" = x], [
    	AC_MSG_RESULT([no])
	AC_MSG_ERROR([Sorry, this software requires libcups-dev.])
    ])

    CUPS_VERSION="$($CUPSCONFIG --api-version)"
    AS_CASE(["$CUPS_VERSION"],
	[1.* | 2.0 | 2.1], [
	    AC_MSG_RESULT([no, only v$CUPS_VERSION])
	    AC_MSG_ERROR([Sorry, you need CUPS 2.2.0 or higher.])
    ])

    AC_MSG_RESULT([yes, v$CUPS_VERSION])

    CPPFLAGS="$CPPFLAGS $($CUPSCONFIG --cflags)"
    LDFLAGS="$LDFLAGS $($CUPSCONFIG --ldflags)"
    LIBS="$LIBS $($CUPSCONFIG --image --libs)"
])


dnl Extra compiler options...
AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [turn on debugging, default=no]))
AC_ARG_ENABLE([maintainer], AS_HELP_STRING([--enable-maintainer], [turn on maintainer mode, default=no]))
AC_ARG_WITH([sanitizer], AS_HELP_STRING([--with-sanitizer], [build with address, leak, memory, thread, or undefined sanitizer, default=no]), [], [with_sanitizer=no])
AS_IF([test "x$with_sanitizer" = xyes], [
    with_sanitizer="address"
], [test "$with_sanitizer" != address -a "$with_sanitizer" != leak -a "$with_sanitizer" != memory -a "$with_sanitizer" != no -a "$with_sanitizer" != thread -a "$with_sanitizer" != undefined], [
    AC_MSG_ERROR([Unsupported --with-sanitizer value "$with_sanitizer" specified.])
])

AS_IF([test x$enable_debug = xyes], [
    CSFLAGS=""
    OPTIM="$OPTIM -g"
    OPTIONS="-DDEBUG -DDEBUG_GUARDS"
], [
    CSFLAGS="-o runtime"
    OPTIM="$OPTIM -g -Os"
    OPTIONS=""
])

AC_SUBST([CSFLAGS])
AC_SUBST([OPTIONS])

WARNINGS=""
AC_SUBST([WARNINGS])

AS_IF([test -n "$GCC"], [
    AS_IF([test x$with_sanitizer != xno], [
	# Use -fsanitize=FOO with debugging...
	OPTIM="$OPTIM -fsanitize=$with_sanitizer"
    ], [echo "$CPPFLAGS $CFLAGS" | grep -q _FORTIFY_SOURCE], [
        # Don't add _FORTIFY_SOURCE if it is already there
    ], [
	# Otherwise use the Fortify enhancements to catch any unbounded
	# string operations...
	CPPFLAGS="$CPPFLAGS -D_FORTIFY_SOURCE=3"
    ])

    dnl Show all standard warnings + unused variables when compiling...
    WARNINGS="-Wall -Wunused"

    dnl Drop some not-useful/unreliable warnings...
    for warning in char-subscripts deprecated-declarations format-truncation format-y2k switch unused-result; do
	AC_MSG_CHECKING([whether compiler supports -Wno-$warning])

	OLDCFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -Wno-$warning -Werror"

	AC_COMPILE_IFELSE([AC_LANG_PROGRAM()], [
	    AC_MSG_RESULT([yes])
	    WARNINGS="$WARNINGS -Wno-$warning"
        ], [
	    AC_MSG_RESULT([no])
	])

	CFLAGS="$OLDCFLAGS"
    done

    dnl Maintainer mode enables -Werror...
    AS_IF([test x$enable_maintainer = xyes], [
	WARNINGS="$WARNINGS -Werror"
    ])

    dnl See if PIE options are supported...
    AC_MSG_CHECKING(whether compiler supports -fPIE)
    OLDCFLAGS="$CFLAGS"
    AS_CASE(["$host_os_name"],
    [darwin*], [
	CFLAGS="$CFLAGS -fPIC -fPIE -Wl,-pie"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[
	    OLDCFLAGS="-fPIC $OLDCFLAGS"
	    LDFLAGS="-fPIE -Wl,-pie $LDFLAGS"
	    AC_MSG_RESULT([yes])
	],[
	    AC_MSG_RESULT([no])
	])
    ], [*], [
	CFLAGS="$CFLAGS -fPIC -fPIE -pie"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[
	    OLDCFLAGS="-fPIC $OLDCFLAGS"
	    LDFLAGS="-fPIE -pie $LDFLAGS"
	    AC_MSG_RESULT([yes])
	],[
	    AC_MSG_RESULT([no])
	])
    ])
    CFLAGS="$OLDCFLAGS"

    dnl OS-specific compiler options...
    AC_MSG_CHECKING([for OS-specific compiler options])
    AS_CASE(["$host_os_name"], [linux*], [
    	# Make sure we get the full set of Linux APIs from the headers...
	CPPFLAGS="$CPPFLAGS -D__USE_MISC -D_GNU_SOURCE"

	# Mark read-only sections as relocatable to random addresses...
	LDFLAGS="$LDFLAGS -Wl,-z,relro,-z,now"

        AC_MSG_RESULT([-D__USE_MISC -D_GNU_SOURCE -Wl,-z,relro,-z,now])
    ], [darwin*], [
        # When not building for debug, target macOS 11 or later, "universal"
        # binaries when possible...
        AS_IF([echo "$CPPFLAGS $CFLAGS $LDFLAGS $OPTIM" | grep -q "\\-arch "], [
            # Don't add architecture/min-version flags if they are already present
            AC_MSG_RESULT([none])
        ], [echo "$CPPFLAGS $CFLAGS $LDFLAGS $OPTIM" | grep -q "\\-mmacosx-version-"], [
            # Don't add architecture/min-version flags if they are already present
            AC_MSG_RESULT([none])
        ], [test "$host_os_version" -ge 200 -a x$enable_debug != xyes], [
	    # macOS 11.0 and higher support the Apple Silicon (arm64) CPUs
	    OPTIM="$OPTIM -mmacosx-version-min=11.0 -arch x86_64 -arch arm64"
            AC_MSG_RESULT([-mmacosx-version-min=11.0 -arch x86_64 -arch arm64])
	], [
	    # Don't add architecture/min-version flags if debug enabled
            AC_MSG_RESULT([none])
	])
    ], [*], [
        AC_MSG_RESULT([none])
    ])
])


dnl Extra linker options...
AC_ARG_WITH([dsoflags], AS_HELP_STRING([--with-dsoflags=...], [Specify additional DSOFLAGS]), [
    DSOFLAGS="$withval $DSOFLAGS"
])
AC_ARG_WITH([ldflags], AS_HELP_STRING([--with-ldflags=...], [Specify additional LDFLAGS]), [
    LDFLAGS="$withval $LDFLAGS"
])


dnl Generate the Makefile and pkg-config file...
AC_CONFIG_FILES([Makefile libsf.pc])
AC_OUTPUT
