StringsUtil - Strings File Library and Utility
==============================================

![Version](https://img.shields.io/github/v/release/michaelrsweet/stringsutil?include_prereleases)
![Apache 2.0](https://img.shields.io/github/license/michaelrsweet/stringsutil)
[![Build Status](https://img.shields.io/github/workflow/status/michaelrsweet/stringsutil/Build)](https://github.com/michaelrsweet/stringsutil/actions/workflows/build.yml)
[![Coverity Scan Status](https://img.shields.io/coverity/scan/NNNNN.svg)](https://scan.coverity.com/projects/michaelrsweet-stringsutil)
[![LGTM Grade](https://img.shields.io/lgtm/grade/cpp/github/michaelrsweet/stringsutil)](https://lgtm.com/projects/g/michaelrsweet/stringsutil/context:cpp)
[![LGTM Alerts](https://img.shields.io/lgtm/alerts/github/michaelrsweet/stringsutil)](https://lgtm.com/projects/g/michaelrsweet/stringsutil/)

StringsUtil provides a library for using Apple strings file and a utility for
creating and merging strings from C code.


Requirements
------------

You'll need a C compiler.


How to Incorporate in Your Project
----------------------------------

Add the `strings.c` and `strings.h` files to your project.  Include the `mmd.h`
header in any file that needs to read/convert markdown files.


"Kicking the Tires"
-------------------

The supplied makefile allows you to build the unit tests on Linux and macOS (at
least), which verify that all of the functions work as expected to produce a
HTML file called `testmmd.html`:

    make test

The makefile also builds the `mmdutil` program.


Installing `mmdutil`
--------------------

You can install the `mmdutil` program by copying it to somewhere appropriate or
run:

    make install

to install it in `/usr/local` along with a man page.


Legal Stuff
-----------

Copyright © 2022 by Michael R Sweet.

stringsutil is licensed under the Apache License Version 2.0 with an (optional)
exception to allow linking against GPL2/LGPL2-only software.  See the files
"LICENSE" and "NOTICE" for more information.
