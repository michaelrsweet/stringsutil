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

You also use the "export" command to produce a C header file containing a
strings file that can be embedded in a program:

    stringsutil -f es.strings export es_strings.h


Using the `libsf` Library
-------------------------

The `libsf` library has a single header file:

```c
#include <sf.h>
```

Use the `pkg-config` command to get the proper compiler and linker options:

```
CFLAGS = `pkgconfig --cflags libsf`
LIBS = `pkgconfig --libs libsf`

cc -o myprogram `pkgconfig --cflags libsf` myprogram.c `pkgconfig --libs libsf`
```

A typical program will either have a directory containing ".strings" files
installed on disk or a collection of header files containing ".strings" files
as constant C strings.  Call [`sfSetLocale`](@@) to initialize the current
locale and then [`sfRegisterDirectory`](@@) or [`sfRegisterString`](@@) to
initialize the default localization strings:

```c
// ".strings" files in "/usr/local/share/myapp/strings"...
#include <sf.h>

...

sfSetLocale();
sfRegisterDirectory("/usr/local/share/myapp/strings");


// ".strings" files in constant strings...
#include <sf.h>
#include "de_strings.h"
#include "es_strings.h"
#include "fr_strings.h"
#include "it_strings.h"
#include "ja_strings.h"

...

sfSetLocale();
sfRegisterString("de", de_strings);
sfRegisterString("es", es_strings);
sfRegisterString("fr", fr_strings);
sfRegisterString("it", it_strings);
sfRegisterString("ja", ja_strings);
```

Once the current local is initialized, you can use the [`sfPrintf`](@@) and
[`sfPuts`](@@) functions to display localized messages.  The `SFSTR` macro is
provided by the `<sf.h>` header to identify strings for localization:

```c
sfPuts(stdout, SFSTR("Usage: myprogram [OPTIONS] FILENAME"));

...

sfPrintf(stderr, SFSTR("myprogram: Expected '-n' value %d out of range."), n);

...

sfPrintf(stderr, SFSTR("myprogram: Syntax error on line %d of '%s'."), linenum, filename);
```
