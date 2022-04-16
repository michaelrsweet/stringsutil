INTRODUCTION
============

The `libsf` library has a single header file:

```c
#include <sf.h>
```

Use the `pkg-config` command to get the proper compiler and linker options:

```
CFLAGS = `pkg-config --cflags libsf`
LIBS = `pkg-config --libs libsf`

cc -o myprogram `pkg-config --cflags libsf` myprogram.c `pkg-config --libs libsf`
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
