Changes in StringsUtil
======================


Changes in v1.1
---------------

- Now support building against CUPS 2.x or libcups 3.x.
- When exporting a C header/source file, the variable name no longer includes
  directory information.
- Fixed decoding of JSON Unicode escapes ("\uXXXX").
- Fixed exporting of quotes in ".strings" files in C header/source files.
- Now preserve formatting strings when translating.
