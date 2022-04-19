StringsUtil - Strings File Library and Utility
==============================================

![Version](https://img.shields.io/github/v/release/michaelrsweet/stringsutil?include_prereleases)
![Apache 2.0](https://img.shields.io/github/license/michaelrsweet/stringsutil)
[![Build Status](https://img.shields.io/github/workflow/status/michaelrsweet/stringsutil/Build)](https://github.com/michaelrsweet/stringsutil/actions/workflows/build.yml)
[![Coverity Scan Status](https://img.shields.io/coverity/scan/24835.svg)](https://scan.coverity.com/projects/michaelrsweet-stringsutil)
[![LGTM Grade](https://img.shields.io/lgtm/grade/cpp/github/michaelrsweet/stringsutil)](https://lgtm.com/projects/g/michaelrsweet/stringsutil/context:cpp)
[![LGTM Alerts](https://img.shields.io/lgtm/alerts/github/michaelrsweet/stringsutil)](https://lgtm.com/projects/g/michaelrsweet/stringsutil/)

StringsUtil provides a library for using Apple ".strings" localization files and
a utility for managing those files.  It is intended as a free, smaller,
embeddable, and more flexible alternative to GNU gettext.  Key features include:

- Support for localizing using both Apple ".strings" and GNU gettext ".po"
  files.
- Simple C/C++ library with support for embedding localization data in an
  executable and/or loading localizations from external files.
- Tools for exporting, importing, and merging localization files.
- Tool for reporting on the quality of a localization.
- Tool for scanning C/C++ source files for localization strings.
- Tool for doing a first pass machine translation using the LibreTranslate
  service/software.


Requirements
------------

You'll need a C compiler and the CUPS library.


"Kicking the Tires"
-------------------

The supplied makefile allows you to build the unit tests on Linux and macOS (at
least), which verify that all of the functions work as expected:

    make test

The makefile also builds the `stringsutil` program.


Installing
----------

Run:

    make install

to install it in `/usr/local` along with a man page.


Using the `stringsutil` Tool
----------------------------

The `stringsutil` tool allows you to manage your ".strings" files.  Create a
".strings" file by scanning source files in the current directory with the
"scan" sub-command:

    stringsutil -f base.strings scan *.[ch]

Create a ".po" file for external localizers to work with using the "export"
sub-command:

    stringsutil -f base.strings export es.po

When the localizer is done, use the "import" sub-command to import the strings
from the ".po" file:

    cp base.strings es.strings
    stringsutil -f es.strings import es.po

Run the "report" sub-command to see how well the localizer did:

    stringsutil -f base.strings report es.strings

When you have made source changes that affect the localization strings, use the
"scan" sub-command again to update the base strings:

    stringsutil -f base.strings scan *.[ch]

Then add those changes to the "es.strings" file with the "merge" sub-command:

    stringsutil -f es.strings -c merge base.strings

The "translate" sub-command uses a LibreTranslate service to do a first-pass
machine translation of your strings.  For example, the following command will
use a local Docker instance of LibreTranslate:

    stringsutil -f es.strings -l es -T http://localhost:5000 translate base.strings

You also use the "export" command to produce a C header file containing a
strings file that can be embedded in a program:

    stringsutil -f es.strings export es_strings.h


Changes in v1.1
---------------

- When exporting a C header/source file, the variable name no longer includes
  directory information.
- Fixed decoding of JSON Unicode escapes ("\uXXXX").
- Fixed exporting of quotes in ".strings" files in C header/source files.
- Now preserve formatting strings when translating.


Legal Stuff
-----------

Copyright © 2022 by Michael R Sweet.

StringsUtil is licensed under the Apache License Version 2.0 with an (optional)
exception to allow linking against GPL2/LGPL2-only software.  See the files
"LICENSE" and "NOTICE" for more information.
