//
// Strings file utility for StringsUtil.
//
// Copyright © 2022 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//
// Usage:
//
//   stringsutil scan -f FILENAME.strings SOURCE-FILE(S)
//   stringsutil merge -f FILENAME.strings FILENAME2.strings
//   stringsutil export -f FILENAME.strings FILENAME.{po,h}
//   stringsutil import -f FILENAME.strings FILENAME.po
//   stringsutil report -f FILENAME.strings FILENAME2.strings
//

#include "strings-file-private.h"


//
// Local functions...
//

static int	export_strings(strings_file_t *sf, const char *filename);
static int	import_strings(strings_file_t *sf, const char *filename);
static int	merge_strings(strings_file_t *sf, const char *filename);
static int	report_strings(strings_file_t *sf, const char *filename);
static int	scan_files(strings_file_t *sf, int num_files, const char *files[]);
static int	usage(FILE *fp, int status);


//
// 'main()' - Main entry.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  (void)argc;
  (void)argv;

  return (1);
}


//
// '()' - .
//

static int
export_strings(strings_file_t *sf, const char *filename)
{
}


//
// '()' - .
//

static int
import_strings(strings_file_t *sf, const char *filename)
{
}


//
// '()' - .
//

static int
merge_strings(strings_file_t *sf, const char *filename);


//
// '()' - .
//

static int
report_strings(strings_file_t *sf, const char *filename);


//
// '()' - .
//

static int
scan_files(strings_file_t *sf, int num_files, const char *files[]);


//
// '()' - Show program usage.
//

static int				// O - Exit status
usage(FILE *fp,				// I - Where to send usage
      int  status)			// I - Exit status
{
  fputs("Usage: stringsutil [OPTIONS] COMMAND FILENAME(S)\n", fp);
  fputs("Options:\n", fp);
  fputs("  -f FILENAME.strings  Specify strings file.\n", fp);
  fputs("  -s NAME              Specify function/macro name for localization.\n", fp);
  fputs("  --help               Show program help.\n", fp);
  fputs("  --version            Show program version.\n", fp);
  fputs("Commands:\n", fp);
  fputs("  export               Export strings to GNU gettext .po or C header file.\n", fp);
  fputs("  import               Import strings from GNU gettext .po file.\n", fp);
  fputs("  merge                Merge strings from another strings file.\n", fp);
  fputs("  report               Report untranslated strings in the specified strings file(s).\n", fp);
  fputs("  scan                 Scan C/C++ source files for strings.\n", fp);

  return (status);
}
