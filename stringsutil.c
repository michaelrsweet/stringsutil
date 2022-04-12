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

static int	export_strings(strings_file_t *sf, const char *sfname, const char *filename);
static int	import_strings(strings_file_t *sf, const char *sfname, const char *filename);
static int	merge_strings(strings_file_t *sf, const char *sfname, const char *filename);
static int	report_strings(strings_file_t *sf, int num_files, const char *files[]);
static int	scan_files(strings_file_t *sf, const char *sfname, const char *funcname, int num_files, const char *files[]);
static int	usage(FILE *fp, int status);
static void	write_string(FILE *fp, const char *s, bool code);
static bool	write_strings(strings_file_t *sf, const char *sfname);


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
    return (export_strings(sf, sfname, files[0]));
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
	       const char     *sfname,	// I - Strings filename
               const char     *filename)// I - Export filename
{
  const char	*ext;			// Filename extension
  bool		code;			// Writing C code?
  FILE		*fp;			// File
  _sf_pair_t	*pair;			// Current pair


  if ((ext = strrchr(filename, '.')) == NULL || (strcmp(ext, ".po") && strcmp(ext, ".h")))
  {
    fprintf(stderr, SFSTR("stringsutil: Unknown export format for '%s'.\n"), filename);
    return (1);
  }

  code = !strcmp(ext, ".h");

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, SFSTR("stringsutil: Unable to export '%s': %s\n"), sfname, strerror(errno));
    return (false);
  }

  if (code)
  {
    fputs("static const char *", fp);
    while (*sfname)
    {
      if (isalnum(*sfname & 255))
        putc(*sfname, fp);
      else
        putc('_', fp);

      sfname ++;
    }
    fputs(" = ", fp);
  }

  for (pair = (_sf_pair_t *)cupsArrayFirst(sf->pairs); pair; pair = (_sf_pair_t *)cupsArrayNext(sf->pairs))
  {
    if (code)
    {
      if (pair->comment)
	fprintf(fp, "/* %s */\n", pair->comment);
      fputs("\"", fp);
    }
    else
    {
      if (pair->comment)
	fprintf(fp, "# %s\n", pair->comment);
      fputs("msgid ", fp);
    }

    write_string(fp, pair->key, code);

    if (code)
      fputs(" = ", fp);
    else
      fputs("\nmsgstr ", fp);

    write_string(fp, pair->text, code);

    if (code)
      fputs(";\\n\"\n", fp);
    else
      fputs("\n\n", fp);
  }

  fclose(fp);

  return (0);
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
  size_t	fnlen;			// Length of function name
  int		i,			// Looping var
		linenum;		// Line number in file
  FILE		*fp;			// Current file
  char		line[1024],		// Line from file
		*lineptr,		// Pointer into line
		comment[1024],		// Comment string (if any)
		text[1024],		// Text string
		*ptr;			// Pointer into comment/text
  _sf_pair_t	pair;			// New pair
  int		changes = 0;		// How many added strings?


  // Scan each file for strings...
  fnlen = strlen(funcname);

  for (i = 0; i < num_files; i ++)
  {
    if ((fp = fopen(files[i], "r")) == NULL)
    {
      fprintf(stderr, SFSTR("stringsutil: Unable to open source file '%s': %s\n"), files[i], strerror(errno));
      return (1);
    }

    linenum = 0;

    while (fgets(line, sizeof(line), fp))
    {
      // Look for the function invocation...
      linenum ++;

      if ((lineptr = strstr(line, funcname)) == NULL)
        continue;

      while (lineptr)
      {
        if ((lineptr == line || strchr(" \t(,{", lineptr[-1]) != NULL) && lineptr[fnlen] == '(')
          break;

        lineptr = strstr(lineptr + 1, funcname);
      }

      if (!lineptr)
        continue;

      // Found "FUNCNAME(", look for comment and text...
      lineptr += fnlen + 1;

      comment[0] = '\0';
      text[0]    = '\0';

      if (!strncmp(lineptr, "/*", 2))
      {
        // Comment, copy it...
        lineptr += 2;
        while (*lineptr && isspace(*lineptr & 255))
          lineptr ++;

        for (ptr = comment; *lineptr; lineptr ++)
        {
          if (!strncmp(lineptr, "*/", 2))
            break;
          else if (ptr < (comment + sizeof(comment) - 1))
            *ptr++ = *lineptr;
        }

        // Strip trailing whitespace in comment
        *ptr = '\0';
        while (ptr > comment && isspace(ptr[-1] & 255))
          *--ptr = '\0';

        // Abort if the comment isn't finished
        if (!*lineptr)
          continue;

        lineptr += 2;
        while (*lineptr && isspace(*lineptr & 255))
          lineptr ++;
      }

      if (*lineptr != '\"')
        continue;

      lineptr ++;
      for (ptr = text; *lineptr && *lineptr != '\"'; lineptr ++)
      {
        if (*lineptr == '\\')
        {
          // Handle C escape
          int ch;			// Quoted character

          lineptr ++;
          if (*lineptr == '\\')
            ch = '\\';
          else if (*lineptr == '\"')
            ch = '\"';
          else if (*lineptr == 'n')
            ch = '\n';
          else if (*lineptr == 'r')
            ch = '\r';
          else if (*lineptr == 't')
            ch = '\t';
          else if (isdigit(*lineptr & 255) && isdigit(lineptr[1] & 255) && isdigit(lineptr[2] & 255))
          {
            ch = ((*lineptr - '0') << 6) | ((lineptr[1] - '0') << 3) | (lineptr[2] - '0');
            lineptr += 2;
          }
          else
            ch = *lineptr;

          if (ptr < (text + sizeof(text) - 1))
            *ptr++ = (char)ch;
        }
        else if (ptr < (text + sizeof(text) - 1))
          *ptr++ = *lineptr;
      }

      *ptr = '\0';

      if (*lineptr != '\"')
        continue;

      // Check whether the pair already exists...
      pair.comment = comment[0] ? comment : NULL;
      pair.key     = text;
      pair.text    = text;

      if (!cupsArrayFind(sf->pairs, &pair))
      {
        // No, add it!
        cupsArrayAdd(sf->pairs, &pair);
        changes ++;
      }
    }

    fclose(fp);
  }

  // Write out any changes as needed...
  if (changes == 0)
  {
    puts(SFSTR("stringsutil: No new strings."));
    return (0);
  }
  else if (changes == 1)
  {
    puts(SFSTR("stringsutil: 1 new string."));
  }
  else
  {
    printf(SFSTR("stringsutil: %d new strings.\n"), changes);
  }

  return (write_strings(sf, sfname) ? 0 : 1);
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


//
// 'write_string()' - Write a quoted C string.
//

static void
write_string(FILE       *fp,		// I - Output file
             const char *s,		// I - String
             bool       code)		// I - Write as embedded code string
{
  const char *escape = code ? "\\\\" : "\\";
					// Escape for string


  if (code)
    fputs("\\\"", fp);
  else
    putc('\"', fp);

  while (*s)
  {
    if (*s == '\\')
      fprintf(fp, "%s%s", escape, escape);
    else if (*s == '\"')
      fprintf(fp, "%s\"", escape);
    else if (*s == '\n')
      fprintf(fp, "%sn", escape);
    else if (*s == '\r')
      fprintf(fp, "%sr", escape);
    else if (*s == '\t')
      fprintf(fp, "%st", escape);
    else if ((*s > 0 && *s < ' ') || *s == 0x7f)
      fprintf(fp, "%s%03o", escape, *s);
    else
      putc(*s, fp);

    s ++;
  }

  if (code)
    fputs("\\\"", fp);
  else
    putc('\"', fp);
}


//
// 'write_strings()' - Write a strings file.
//

static bool				// O - `true` on success, `false` on failure
write_strings(strings_file_t *sf,	// I - Strings
              const char     *sfname)	// I - Strings filename
{
  FILE		*fp;			// File
  _sf_pair_t	*pair;			// Current pair


  if ((fp = fopen(sfname, "w")) == NULL)
  {
    fprintf(stderr, SFSTR(/*Unable to create .strings file*/"stringsutil: Unable to create '%s': %s\n"), sfname, strerror(errno));
    return (false);
  }

  for (pair = (_sf_pair_t *)cupsArrayFirst(sf->pairs); pair; pair = (_sf_pair_t *)cupsArrayNext(sf->pairs))
  {
    if (pair->comment)
      fprintf(fp, "/* %s */\n", pair->comment);
    write_string(fp, pair->key, false);
    fputs(" = ", fp);
    write_string(fp, pair->text, false);
    fputs(";\n", fp);
  }

  fclose(fp);

  return (true);
}
