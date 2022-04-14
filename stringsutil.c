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
//   stringsutil merge [-c] -f FILENAME-LL.strings FILENAME.strings
//   stringsutil export -f FILENAME.strings FILENAME.{c,cc,cpp,cxx,h,po}
//   stringsutil import [-a] -f FILENAME.strings FILENAME.{po,strings}
//   stringsutil report -f FILENAME.strings FILENAME-LL.strings
//

#include "strings-file-private.h"
#include "es_strings.h"
#include "fr_strings.h"


//
// Local functions...
//

static int	export_strings(strings_file_t *sf, const char *sfname, const char *filename);
static void	import_string(strings_file_t *sf, char *msgid, char *msgstr, char           *comment, bool addnew, int *added, int *ignored, int *modified);
static int	import_strings(strings_file_t *sf, const char *sfname, const char *filename, bool addnew);
static int	merge_strings(strings_file_t *sf, const char *sfname, const char *filename, bool clean);
static int	report_strings(strings_file_t *sf, const char *filename);
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
		*opt;			// Pointer to option
  bool		addnew = false,		// Add new strings on import?
		clean = false;		// Clean old strings?
  const char	*sfname = NULL;		// Strings filename
  strings_file_t *sf = NULL;		// Strings file
  struct stat	sfinfo;			// Strings file info


  // Initialize localizations...
  sfSetLocale();
  sfRegisterString("es", es_strings);
  sfRegisterString("fr", fr_strings);

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
      sfPrintf(stderr, SFSTR("stringsutil: Unknown option '%s'."), argv[i]);
      return (usage(stderr, 1));
    }
    else if (argv[i][0] == '-')
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
        switch (*opt)
        {
          case 'a' : // -a
              addnew = true;
              break;

          case 'c' : // -c
              clean = true;
              break;

          case 'f' : // -f FILENAME.strings
              i ++;
              if (i >= argc)
              {
                sfPuts(stderr, SFSTR("stringsutil: Expected strings filename after '-f'."));
                return (usage(stderr, 1));
              }

              sf     = sfNew();
              sfname = argv[i];

              if (!sfLoadFromFile(sf, argv[i]) && errno != ENOENT)
              {
                sfPrintf(stderr, SFSTR("stringsutil: Unable to load '%s': %s"), argv[i], sfGetError(sf));
                return (1);
              }
              break;

          case 'n' : // -n FUNCTION-NAME
              i ++;
              if (i >= argc)
              {
                sfPuts(stderr, SFSTR("stringsutil: Expected function name after '-n'."));
                return (usage(stderr, 1));
              }
              funcname = argv[i];
              break;

	  default :
	      sfPrintf(stderr, SFSTR("stringsutil: Unknown option '-%c'."), *opt);
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
      sfPuts(stderr, SFSTR("stringsutil: Too many files."));
      return (1);
    }
  }

  // Do the command...
  if (!sf)
  {
    sfPuts(stderr, SFSTR("stringsutil: Expected strings file."));
    return (usage(stderr, 1));
  }

  if (!command)
  {
    sfPuts(stderr, SFSTR("stringsutil: Expected command name."));
    return (usage(stderr, 1));
  }
  else if (!strcmp(command, "scan"))
  {
    if (funcname)
    {
      return (scan_files(sf, sfname, funcname, num_files, files));
    }
    else
    {
      sfPuts(stderr, SFSTR("stringsutil: Expected '-n FUNCTION-NAME' option."));
      return (usage(stderr, 1));
    }
  }
  else if (num_files == 0)
  {
    sfPrintf(stderr, SFSTR("stringsutil: Expected %s filename."), command);
    return (usage(stderr, 1));
  }
  else if (num_files > 1)
  {
    sfPuts(stderr, SFSTR("stringsutil: Too many files."));
    return (1);
  }
  else if (!strcmp(command, "import"))
  {
    return (import_strings(sf, sfname, files[0], addnew));
  }
  else if (!strcmp(command, "merge"))
  {
    return (merge_strings(sf, sfname, files[0], clean));
  }
  else if (stat(sfname, &sfinfo))
  {
    sfPrintf(stderr, SFSTR("stringsutil: Unable to load '%s': %s"), sfname, sfGetError(sf));
    return (1);
  }
  else if (!strcmp(command, "export"))
  {
    return (export_strings(sf, sfname, files[0]));
  }
  else if (!strcmp(command, "report"))
  {
    return (report_strings(sf, files[0]));
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
  size_t	count;			// Number of pairs remaining


  if ((ext = strrchr(filename, '.')) == NULL || (strcmp(ext, ".po") && strcmp(ext, ".h") && strcmp(ext, ".c") && strcmp(ext, ".cc") && strcmp(ext, ".cpp") && strcmp(ext, ".cxx")))
  {
    sfPrintf(stderr, SFSTR("stringsutil: Unknown export format for '%s'."), filename);
    return (1);
  }

  code = !strcmp(ext, ".h") || !strcmp(ext, ".c") || !strcmp(ext, ".cc") || !strcmp(ext, ".cpp") || !strcmp(ext, ".cxx");

  if ((fp = fopen(filename, "w")) == NULL)
  {
    sfPrintf(stderr, SFSTR("stringsutil: Unable to export '%s': %s"), sfname, strerror(errno));
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

  for (count = sf->num_pairs, pair = sf->pairs; count > 0; count --, pair ++)
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

    if (code && count > 1)
      fputs(";\\n\"\n", fp);
    else if (code)
      fputs(";\\n\";\n", fp);
    else
      fputs("\n\n", fp);
  }

  fclose(fp);

  return (0);
}


//
// 'import_string()' - Import a single string.
//

static void
import_string(strings_file_t *sf,	// I  - Strings
              char           *msgid,	// I  - Key string
              char           *msgstr,	// I  - Localized text
              char           *comment,	// I  - Comment, if any
              bool           addnew,	// I  - Add new strings?
              int            *added,	// IO - Number of added strings
              int            *ignored,	// IO - Number of ignored strings
              int            *modified)	// IO - Number of modified strings
{
  _sf_pair_t	*match;			// Matching existing entry
  bool		clear = false;		// Clear incoming strings?


  if (*msgid && *msgstr)
  {
    // See if this is an existing string...
    if ((match = _sfFind(sf, msgid)) != NULL)
    {
      // Found a match...
      if (strcmp(match->text, msgstr))
      {
	// Modify the localization...
	free(match->text);
	match->text = strdup(msgstr);
	(*modified) ++;
	clear = true;
      }
    }
    else if (addnew)
    {
      // Add new string...
      _sfAdd(sf, msgid, msgstr, comment);
      (*added) ++;
      clear = true;
    }
    else
    {
      // Ignore string...
      (*ignored) ++;
      clear = true;
    }
  }

  if (clear)
  {
    // Clear the incoming string...
    *msgid   = '\0';
    *msgstr  = '\0';
    *comment = '\0';
  }
}


//
// 'import_strings()' - Import strings from a PO file.
//

static int				// O - Exit status
import_strings(strings_file_t *sf,	// I - Strings
               const char     *sfname,	// I - Strings filename
               const char     *filename,// I - Import filename
               bool           addnew)	// I - Add new strings?
{
  const char	*ext;			// Filename extension...
  int		linenum = 0,		// Current line number
		added = 0,		// Number of added strings
		ignored = 0,		// Number of ignored strings
		modified = 0;		// Number of modified strings


  if ((ext = strrchr(filename, '.')) == NULL || (strcmp(ext, ".po") && strcmp(ext, ".strings")))
  {
    sfPrintf(stderr, SFSTR("stringsutil: Unknown import format for '%s'."), filename);
    return (1);
  }

  if (!strcmp(ext, ".po"))
  {
    // Import a GNU gettext .po file...
    FILE	*fp;			// PO file
    char	line[1024],		// Line buffer
		*lineptr,		// Pointer into line
		comment[1024] = "",	// Comment string
		msgid[1024] = "",	// msgid buffer
		msgstr[1024] = "",	// msgstr buffer
		*end = NULL,		// End of buffer
		*ptr = NULL;		// Position in buffer

    // Open the PO file...
    if ((fp = fopen(filename, "r")) == NULL)
    {
      sfPrintf(stderr, SFSTR("stringsutil: Unable to import '%s': %s"), filename, strerror(errno));
      return (1);
    }

    // Read lines until the end...
    while (fgets(line, sizeof(line), fp))
    {
      linenum ++;

      lineptr = line;

      if (*lineptr == '#')
      {
	// Comment...
	import_string(sf, msgid, msgstr, comment, addnew, &added, &ignored, &modified);

	// Skip whitespace
	lineptr ++;
	while (*lineptr && isspace(*lineptr & 255))
	  lineptr ++;

	// Append comment...
	ptr = comment + strlen(comment);
	end = comment + sizeof(comment) - 1;

	if (ptr > comment && ptr < end)
	  *ptr++ = ' ';			// Add initial space if this isn't the first comment line

	while (*lineptr && *lineptr != '\n' && ptr < end)
	  *ptr++ = *lineptr++;

	*ptr = '\0';
	ptr  = end = NULL;
	continue;
      }
      else if (!strncmp(line, "msgid ", 6))
      {
	// Start of message ID...
	import_string(sf, msgid, msgstr, comment, addnew, &added, &ignored, &modified);

	lineptr += 6;
	while (*lineptr && isspace(*lineptr & 255))
	  lineptr ++;

	ptr = msgid;
	end = msgid + sizeof(msgid) - 1;
      }
      else if (!strncmp(line, "msgstr ", 7))
      {
	// Start of message string...
	lineptr += 7;
	while (*lineptr && isspace(*lineptr & 255))
	  lineptr ++;

	ptr = msgstr;
	end = msgstr + sizeof(msgstr) - 1;
      }
      else if (*lineptr == '\n')
      {
	// Blank line
	import_string(sf, msgid, msgstr, comment, addnew, &added, &ignored, &modified);
	ptr = end = NULL;
	comment[0] = '\0';
	continue;
      }

      if (*lineptr != '\"' || !ptr)
      {
	// Something unexpected...
	sfPrintf(stderr, SFSTR("stringsutil: Syntax error on line %d of '%s'."), linenum, filename);
	fclose(fp);
	return (1);
      }

      // Append string...
      lineptr ++;
      while (*lineptr && *lineptr != '\"')
      {
	if (*lineptr == '\\')
	{
	  // Add escaped character...
	  int	ch;			// Escaped character

	  lineptr ++;
	  if (*lineptr == '\\' || *lineptr == '\"' || *lineptr == '\'')
	  {
	    // Simple escape...
	    ch = *lineptr;
	  }
	  else if (*lineptr == 'n')
	  {
	    // Newline
	    ch = '\n';
	  }
	  else if (*lineptr == 'r')
	  {
	    // Carriage return
	    ch = '\r';
	  }
	  else if (*lineptr == 't')
	  {
	    // Horizontal tab
	    ch = '\t';
	  }
	  else if (*lineptr >= '0' && *lineptr <= '3' && lineptr[1] >= '0' && lineptr[1] <= '7' && lineptr[2] >= '0' && lineptr[2] <= '7')
	  {
	    // Octal escape
	    ch = ((*lineptr - '0') << 6) | ((lineptr[1] - '0') << 3) | (lineptr[2] - '0');
	  }
	  else
	  {
	    sfPrintf(stderr, SFSTR("stringsutil: Syntax error on line %d of '%s'."), linenum, filename);
	    fclose(fp);
	    return (1);
	  }

	  if (ptr < end)
	    *ptr++ = (char)ch;
	}
	else if (ptr < end)
	{
	  // Add literal character...
	  *ptr++ = *lineptr;
	}

	lineptr ++;
      }

      *ptr = '\0';
    }

    // All done, import the last string, if any...
    import_string(sf, msgid, msgstr, comment, addnew, &added, &ignored, &modified);

    fclose(fp);
  }
  else
  {
    // Import a .strings file...
    strings_file_t	*isf;		// Import strings
    _sf_pair_t		*pair,		// Existing pair
			*ipair;		// Imported pair
    size_t		count;		// Number of pairs

    isf = sfNew();
    if (!sfLoadFromFile(isf, filename))
    {
      sfPrintf(stderr, SFSTR("stringsutil: Unable to import '%s': %s"), filename, sfGetError(isf));
      sfDelete(isf);
      return (1);
    }

    for (count = isf->num_pairs, ipair = isf->pairs; count > 0; count --, ipair ++)
    {
      if ((pair = _sfFind(sf, ipair->key)) != NULL)
      {
        // Existing string, check for differences...
        if (strcmp(pair->text, ipair->text))
        {
          // Yes
          free(pair->text);
          pair->text = strdup(ipair->text);
          modified ++;
        }
      }
      else if (addnew)
      {
        // Add new string...
        _sfAdd(sf, ipair->key, ipair->text, ipair->comment);
        added ++;
      }
      else
      {
        // Ignore...
        ignored ++;
      }
    }

    sfDelete(isf);
  }

  // Finish up...
  sfPrintf(stdout, SFSTR("stringsutil: %d added, %d ignored, %d modified."), added, ignored, modified);

  if (added || modified)
    return (write_strings(sf, sfname) ? 0 : 1);
  else
    return (0);
}


//
// 'merge_strings()' - Merge strings from another strings file.
//

static int				// O - Exit status
merge_strings(strings_file_t *sf,	// I - Strings
              const char     *sfname,	// I - Strings filename
              const char     *filename,	// I - Merge filename
              bool           clean)	// I - Clean old strings?
{
  strings_file_t	*msf;		// Strings file to merge
  _sf_pair_t		*pair,		// Current pair
			*mpair;		// Merge pair
  size_t		count;		// Remaining pairs
  int			added = 0,	// Number of added strings
			removed = 0;	// Number of removed strings


  // Open the merge file...
  msf = sfNew();
  if (!sfLoadFromFile(msf, filename))
  {
    sfPrintf(stderr, SFSTR("stringsutil: Unable to merge '%s': %s"), filename, sfGetError(msf));
    sfDelete(msf);
    return (1);
  }

  // Loop through the merge list and add any new strings...
  for (count = msf->num_pairs, mpair = msf->pairs; count > 0; count --, mpair ++)
  {
    if (_sfFind(sf, mpair->key))
      continue;

    added ++;
    _sfAdd(sf, mpair->key, mpair->text, mpair->comment);
  }

  // Then clean old messages (if needed)...
  if (clean)
  {
    for (count = sf->num_pairs, pair = sf->pairs; count > 0; count --, pair ++)
    {
      if (_sfFind(msf, pair->key))
	continue;

      removed ++;

      _sfRemove(sf, pair - sf->pairs);

      pair --;
      count --;

      if (count == 0)
        break;
    }
  }

  sfDelete(msf);

  if (added || removed)
  {
    sfPrintf(stdout, SFSTR("stringsutil: Added %d string(s), removed %d string(s)."), added, removed);
    return (write_strings(sf, sfname) ? 0 : 1);
  }

  return (0);
}


//
// 'report_strings()' - Report how many strings are translated.
//

static int				// O - Exit status
report_strings(strings_file_t *sf,	// I - Strings
               const char     *filename)// I - Comparison filename
{
  strings_file_t	*rsf;		// Strings file to report
  _sf_pair_t		*pair,		// Current pair
			*rpair;		// Report pair
  size_t		count;		// Number of pairs
  int			total,		// Total messages
			translated = 0,	// Translated messages
			missing = 0,	// Missing messages
			old = 0,	// Old messages
			untranslated = 0;
					// Messages not translated


  // Open the report file...
  rsf = sfNew();
  if (!sfLoadFromFile(rsf, filename))
  {
    sfPrintf(stderr, SFSTR("stringsutil: Unable to report on '%s': %s"), filename, sfGetError(rsf));
    sfDelete(rsf);
    return (1);
  }

  // Loop through the report list and check strings...
  for (count = rsf->num_pairs, rpair = rsf->pairs; count > 0; count --, rpair ++)
  {
    if ((pair = _sfFind(sf, rpair->key)) == NULL)
    {
      old ++;
      continue;
    }

    if (strcmp(rpair->text, pair->text))
      translated ++;
    else
      untranslated ++;
  }

  // Then look for new messages that haven't been merged...
  for (count = sf->num_pairs, pair = sf->pairs; count > 0; count --, pair ++)
  {
    if (_sfFind(rsf, pair->key))
      continue;

    missing ++;
  }

  sfDelete(rsf);

  total = translated + missing + untranslated;

  if (missing || old)
    sfPrintf(stdout, SFSTR("stringsutil: File needs to be merged, %d missing and %d old string(s)."), missing, old);

  if (total == 0)
    sfPuts(stdout, SFSTR("stringsutil: No strings."));
  else
    sfPrintf(stdout, SFSTR("stringsutil: %d string(s), %d (%d%%) translated, %d (%d%%) untranslated."), total, translated, 100 * translated / total, untranslated + missing, 100 * (untranslated + missing) / total);

  return (untranslated > (total / 2) ? 1 : 0);
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
  int		changes = 0;		// How many added strings?


  // Scan each file for strings...
  fnlen = strlen(funcname);

  for (i = 0; i < num_files; i ++)
  {
    if ((fp = fopen(files[i], "r")) == NULL)
    {
      sfPrintf(stderr, SFSTR("stringsutil: Unable to open source file '%s': %s"), files[i], strerror(errno));
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
          else if (*lineptr >= '0' && *lineptr <= '3' && lineptr[1] >= '0' && lineptr[1] <= '7' && lineptr[2] >= '0' && lineptr[2] <= '7')
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
      if (!_sfFind(sf, text))
      {
        // No, add it!
        _sfAdd(sf, text, text, comment[0] ? comment : NULL);
        changes ++;
      }
    }

    fclose(fp);
  }

  // Write out any changes as needed...
  if (changes == 0)
  {
    sfPuts(stdout, SFSTR("stringsutil: No new strings."));
    return (0);
  }
  else if (changes == 1)
  {
    sfPuts(stdout, SFSTR("stringsutil: 1 new string."));
  }
  else
  {
    sfPrintf(stdout, SFSTR("stringsutil: %d new strings."), changes);
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
  sfPuts(fp, SFSTR("Usage: stringsutil [OPTIONS] COMMAND FILENAME(S)"));
  puts("");
  sfPuts(fp, SFSTR("Options:"));
  sfPuts(fp, SFSTR("  -a                   Add new strings (import)."));
  sfPuts(fp, SFSTR("  -c                   Remove old strings (merge)."));
  sfPuts(fp, SFSTR("  -f FILENAME.strings  Specify strings file."));
  sfPuts(fp, SFSTR("  -n NAME              Specify function/macro name for localization."));
  sfPuts(fp, SFSTR("  --help               Show program help."));
  sfPuts(fp, SFSTR("  --version            Show program version."));
  puts("");
  sfPuts(fp, SFSTR("Commands:"));
  sfPuts(fp, SFSTR("  export               Export strings to GNU gettext .po or C source file."));
  sfPuts(fp, SFSTR("  import               Import strings from GNU gettext .po or .strings file."));
  sfPuts(fp, SFSTR("  merge                Merge strings from another strings file."));
  sfPuts(fp, SFSTR("  report               Report untranslated strings in the specified strings file(s)."));
  sfPuts(fp, SFSTR("  scan                 Scan C/C++ source files for strings."));

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
  size_t	count;			// Number of pairs


  if ((fp = fopen(sfname, "w")) == NULL)
  {
    sfPrintf(stderr, SFSTR(/*Unable to create .strings file*/"stringsutil: Unable to create '%s': %s\n"), sfname, strerror(errno));
    return (false);
  }

  for (count = sf->num_pairs, pair = sf->pairs; count > 0; count --, pair ++)
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
