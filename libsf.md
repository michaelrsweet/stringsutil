Using the `libsf` Library
=========================

The `libsf` library has a single header file:

```c
#include <sf.h>
```

Use the `pkg-config` command to get the proper compiler and linker options:

```
CFLAGS = `pkgconfig --cflags libsf`
LIBS = `pkgconfig --libs libsf`
```
