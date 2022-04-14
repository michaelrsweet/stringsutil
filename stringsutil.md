Introduction
============

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
- Tool for doing a first pass machine translation.


Apple ".strings" Files
-----------------------

Apple ".strings" files are localization files used on macOS and iOS, as well as
for localizing Internet Printing Protocol (IPP) attributes and values.  The
format consists of lines containing key/text pairs:

    "key string" = "localized text value";

C-style comments can be included before a pair to provide information to a
localizer:

    /* A whitty comment for the localizer about this string */
    "key string" = "localized text value";

Each ".strings" file represents a single language or locale.

Most open source software uses the GNU gettext library which supports a
different ".po" file format:

    # A whitty comment for the localizer about this string
    msgid "key string"
    msgstr "localized text value"

This format is "compiled" into binary ".mo" files that are typically stored in
a system directory.  Like ".strings" files, one ".po" file is used for every
language or locale.

StringUtils supports importing and exporting ".po" files, when needed, but uses
the Apple ".strings" format exclusively both on disk and in memory.


The `stringsutil` Tool
----------------------

The `stringsutil` tool allows you to manage your ".strings" files.

Create a ".strings" file by scanning source files in the current directory:

    stringsutil -f base.strings scan *.[ch]

Create a ".po" file for external localizers to work with:

    stringsutil -f base.strings export es.po

Import the ".po" file when the localizer is done:

    cp base.strings es.strings
    stringsutil -f es.strings import es.po

See how well the localizer did:

    stringsutil -f base.strings report es.strings

Update the ".strings" file for changes to the source files:

    stringsutil -f base.strings scan *.[ch]

Merge those changes into the "es.strings" file:

    stringsutil -f es.strings -c merge base.strings


The `libsf` Library
-------------------

