Changes in StringsUtil
======================


v1.2 (YYYY-MM-DD)
-----------------

- Updated the output from `stringsutil translate` to better show progress.
- Updated code to work with latest CUPS 2.5/3.0.
- Fixed a crash bug in `stringsutil report` when checking format strings.


v1.1 (2024-03-23)
-----------------

- Now support building against CUPS 2.x or libcups 3.x.
- When exporting a C header/source file, the variable name no longer includes
  directory information.
- Fixed decoding of JSON Unicode escapes ("\uXXXX").
- Fixed exporting of quotes in ".strings" files in C header/source files.
- Now preserve formatting strings when translating.
