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
static int	import_strings(strings_file_t *sf, const char *sfname, const char *filename);
static int	merge_strings(strings_file_t *sf, const char *sfname, const char *filename);
static int	report_strings(strings_file_t *sf, int num_files, const char *files[]);
static int	scan_files(strings_file_t *sf, const char *sfname, const char *funcname, int num_files, const char *files[]);
static int	usage(FILE *fp, int status);


//
// 'main()' - Main entry.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int		i,			// Looping var
		num_files = 0;		// Number of files
  const char	*files[1000],		// Files
		*command = NULL,	// Command
		*funcname = NULL,	// Function name
		*opt,			// Pointer to option
		*sfname;		// Strings filename
  strings_file_t *sf = NULL;		// Strings file

  // Parse command-line...
  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--help"))
    {
      return (usage(stdout, 0));
    }
    else if (!strcmp(argv[i], "--version"))
    {
      puts(VERSION);
      return (0);
    }
    else if (!strncmp(argv[i], "--", 2))
    {
      fprintf(stderr, SFSTR("stringsutil: Unknown option '%s'.\n"), argv[i]);
      return (usage(stderr, 1));
    }
    else if (argv[i][0] == '-')
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
        switch (*opt)
        {
          case 'f' : // -f FILENAME.strings
              i ++;
              if (i >= argc)
              {
                fputs(SFSTR("stringsutil: Expected strings filename after '-f'.\n"), stderr);
                return (usage(stderr, 1));
              }

              sf     = sfNew();
              sfname = argv[i];

              if (!sfLoadFromFile(sf, argv[i]) && errno != ENOENT)
              {
                fprintf(stderr, SFSTR("stringsutil: Unable to load '%s': %s\n"), argv[i], sfGetError(sf));
                return (1);
              }
              break;

          case 'n' : // -n FUNCTION-NAME
              i ++;
              if (i >= argc)
              {
                fputs(SFSTR("stringsutil: Expected function name after '-n'.\n"), stderr);
                return (usage(stderr, 1));
              }
              funcname = argv[i];
              break;

	  default :
	      fprintf(stderr, SFSTR("stringsutil: Unknown option '-%c'.\n"), *opt);
	      return (usage(stderr, 1));
        }
      }
    }
    else if (!strcmp(argv[i], "export") || !strcmp(argv[i], "import") || !strcmp(argv[i], "merge") || !strcmp(argv[i], "report") || !strcmp(argv[i], "scan"))
    {
      command = argv[i];
    }
    else if (num_files < (int)(sizeof(files) / sizeof(files[0])))
    {
      files[num_files ++] = argv[i];
    }
    else
    {
      fputs(SFSTR("stringsutil: Too many files.\n"), stderr);
      return (1);
    }
  }

  // Do the command...
  if (!command)
  {
    fputs(SFSTR("stringsutil: Expected command name.\n"), stderr);
    return (usage(stderr, 1));
  }
  else if (!strcmp(command, "report"))
  {
    return (report_strings(sf, num_files, files));
  }
  else if (!strcmp(command, "scan"))
  {
    if (funcname)
    {
      return (scan_files(sf, sfname, funcname, num_files, files));
    }
    else
    {
      fputs(SFSTR("stringsutil: Expected '-n FUNCTION-NAME' option.\n"), stderr);
      return (usage(stderr, 1));
    }
  }
  else if (num_files == 0)
  {
    fprintf(stderr, SFSTR("stringsutil: Expected %s filename.\n"), command);
    return (usage(stderr, 1));
  }
  else if (num_files > 1)
  {
    fputs(SFSTR("stringsutil: Too many files.\n"), stderr);
    return (1);
  }
  else if (!strcmp(command, "export"))
  {
    return (export_strings(sf, files[0]));
  }
  else if (!strcmp(command, "import"))
  {
    return (import_strings(sf, sfname, files[0]));
  }
  else if (!strcmp(command, "merge"))
  {
    return (merge_strings(sf, sfname, files[0]));
  }

  return (0);
}


//
// 'export_strings()' - Export strings to a PO or C header file.
//

static int				// O - Exit status
export_strings(strings_file_t *sf,	// I - Strings
               const char     *filename)// I - Export filename
{
  (void)sf;
  (void)filename;

  return (1);
}


//
// 'import_strings()' - Import strings from a PO file.
//

static int				// O - Exit status
import_strings(strings_file_t *sf,	// I - Strings
               const char     *sfname,	// I - Strings filename
               const char     *filename)// I - Import filename
{
  (void)sf;
  (void)sfname;
  (void)filename;

  return (1);
}


//
// 'merge_strings()' - Merge strings from another strings file.
//

static int				// O - Exit status
merge_strings(strings_file_t *sf,	// I - Strings
              const char     *sfname,	// I - Strings filename
              const char     *filename)	// I - Merge filename
{
  (void)sf;
  (void)sfname;
  (void)filename;

  return (1);
}


//
// 'report_strings()' - Report how many strings are translated.
//

static int				// O - Exit status
report_strings(strings_file_t *sf,	// I - Strings
               int            num_files,// I - Number of files
               const char     *files[])	// I - Files
{
  (void)sf;
  (void)num_files;
  (void)files;

  return (1);
}


//
// 'scan_files()' - Scan source files for localization strings.
//

static int				// O - Exit status
scan_files(strings_file_t *sf,		// I - Strings
           const char     *sfname,	// I - Strings filename
           const char     *funcname,	// I - Localization function name
           int            num_files,	// I - Number of files
           const char     *files[])	// I - Files
{
  (void)sf;
  (void)sfname;
  (void)funcname;
  (void)num_files;
  (void)files;

  return (1);
}


//
// 'usage()' - Show program usage.
//

static int				// O - Exit status
usage(FILE *fp,				// I - Where to send usage
      int  status)			// I - Exit status
{
  fputs(SFSTR("Usage: stringsutil [OPTIONS] COMMAND FILENAME(S)\n"), fp);
  fputs(SFSTR("Options:\n"), fp);
  fputs(SFSTR("  -f FILENAME.strings  Specify strings file.\n"), fp);
  fputs(SFSTR("  -n NAME              Specify function/macro name for localization.\n"), fp);
  fputs(SFSTR("  --help               Show program help.\n"), fp);
  fputs(SFSTR("  --version            Show program version.\n"), fp);
  fputs(SFSTR("Commands:\n"), fp);
  fputs(SFSTR("  export               Export strings to GNU gettext .po or C header file.\n"), fp);
  fputs(SFSTR("  import               Import strings from GNU gettext .po file.\n"), fp);
  fputs(SFSTR("  merge                Merge strings from another strings file.\n"), fp);
  fputs(SFSTR("  report               Report untranslated strings in the specified strings file(s).\n"), fp);
  fputs(SFSTR("  scan                 Scan C/C++ source files for strings.\n"), fp);

  return (status);
}
