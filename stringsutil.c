//
// Strings file utility for StringsUtil.
//
// Copyright © 2022-2024 by Michael R Sweet.
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
//   stringsutil translate -f FILENAME.strings -l LOCALE [-A API-KEY] [-T URL]
//

#include "sf-private.h"
#include "es_strings.h"
#include "fr_strings.h"
#include <cups/cups.h>


//
// The CUPS API is changed in CUPS v3...
//

#if CUPS_VERSION_MAJOR < 3
#  define CUPS_ENCODING_ISO8859_1	CUPS_ISO8859_1
#  define CUPS_ENCODING_JIS_X0213	CUPS_JIS_X0213
#  define cups_len_t int
#  define cups_page_header_t cups_page_header2_t
#  define cupsArrayNew cupsArrayNew3
#  define cupsLangGetName(lang)	lang->language
#  define cupsRasterReadHeader cupsRasterReadHeader2
#  define cupsRasterWriteHeader cupsRasterWriteHeader2
#  define httpAddrConnect httpAddrConnect2
#  define httpConnect httpConnect2
#  define httpGetDateString httpGetDateString2
#  define httpRead httpRead2
#  define httpReconnect httpReconnect2
#  define httpSetEncryption(http,e) (httpEncryption(http,e)>=0)
#  define httpWrite httpWrite2
#  define httpWriteResponse(http,code) (httpWriteResponse(http,code) == 0)
#  define IPP_NUM_CAST (int)
#  if CUPS_VERSION_MINOR < 3
#    define HTTP_STATUS_FOUND (http_status_t)302
#  endif // CUPS_VERSION_MINOR < 3
#  if CUPS_VERSION_MINOR < 5
#    define cupsArrayGetCount cupsArrayCount
#    define cupsArrayGetElement(a,n) cupsArrayIndex(a,(int)n)
#    define cupsArrayGetFirst cupsArrayFirst
#    define cupsArrayGetLast cupsArrayLast
#    define cupsArrayGetNext cupsArrayNext
#    define cupsArrayGetPrev cupsArrayPrev
#    define cupsCreateTempFd(prefix,suffix,buffer,bufsize) cupsTempFd(buffer,bufsize)
#    define cupsGetError cupsLastError
#    define cupsGetErrorString cupsLastErrorString
#    define cupsGetUser cupsUser
#    define cupsRasterGetErrorString cupsRasterErrorString
#    define httpAddrGetFamily httpAddrFamily
#    define httpAddrGetLength httpAddrLength
#    define httpAddrGetString httpAddrString
#    define httpAddrIsLocalhost httpAddrLocalhost
#    define httpDecode64(out,outlen,in,end) httpDecode64_2(out,outlen,in)
#    define httpEncode64(out,outlen,in,inlen,url) httpEncode64_2(out,outlen,in,inlen)
#    define httpGetError httpError
#    define httpStatusString httpStatus
#    define ippGetFirstAttribute ippFirstAttribute
#    define ippGetLength ippLength
#    define ippGetNextAttribute ippNextAttribute
typedef cups_array_func_t cups_array_cb_t;
typedef cups_acopy_func_t cups_acopy_cb_t;
typedef cups_afree_func_t cups_afree_cb_t;
typedef cups_raster_iocb_t cups_raster_cb_t;
typedef ipp_copycb_t ipp_copy_cb_t;
#  else
#    define httpDecode64 httpDecode64_3
#    define httpEncode64 httpEncode64_3
#  endif // CUPS_VERSION_MINOR < 5
#else
#  define cups_len_t size_t
#  define cups_utf8_t char
#  define IPP_NUM_CAST (size_t)
#endif // CUPS_VERSION_MAJOR < 3


//
// Local functions...
//

static bool	compare_formats(const char *s1, const char *s2);
static int	decode_json(const char *data, cups_option_t **vars);
static const char *decode_string(const char *data, char term, char *buffer, size_t bufsize);
static char	*encode_json(int num_vars, cups_option_t *vars);
static char	*encode_string(const char *s, char *bufptr, char *bufend);
static int	export_strings(sf_t *sf, const char *sfname, const char *filename);
static void	import_string(sf_t *sf, char *msgid, char *msgstr, char *comment, bool addnew, int *added, int *ignored, int *modified);
static int	import_strings(sf_t *sf, const char *sfname, const char *filename, bool addnew);
static bool	matching_formats(const char *key, const char *text);
static int	merge_strings(sf_t *sf, const char *sfname, const char *filename, bool clean);
static int	report_strings(sf_t *sf, const char *filename);
static int	scan_files(sf_t *sf, const char *sfname, const char *funcname, int num_files, const char *files[]);
static int	translate_strings(sf_t *sf, const char *sfname, const char *url, const char *apikey, const char *language, const char *filename);
static int	usage(FILE *fp, int status);
static void	write_string(FILE *fp, const char *s, bool code);
static bool	write_strings(sf_t *sf, const char *sfname);


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
		*apikey = getenv("LIBRETRANSLATE_APIKEY"),
					// API key
		*command = NULL,	// Command
		*funcname = "SFSTR",	// Function name
		*language = NULL,	// Language code
		*url = getenv("LIBRETRANSLATE_URL"),
					// URL to LibreTranslate server
		*opt;			// Pointer to option
  bool		addnew = false,		// Add new strings on import?
		clean = false;		// Clean old strings?
  const char	*sfname = NULL;		// Strings filename
  sf_t		*sf = NULL;		// Strings file
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
          case 'A' : // -A APIKEY
              i ++;
              if (i >= argc)
              {
                sfPuts(stderr, SFSTR("stringsutil: Expected LibreTranslate API key after '-A'."));
                return (usage(stderr, 1));
              }

              apikey = argv[i];
              break;

          case 'T' : // -T URL
              i ++;
              if (i >= argc)
              {
                sfPuts(stderr, SFSTR("stringsutil: Expected LibreTranslate URL after '-T'."));
                return (usage(stderr, 1));
              }

              url = argv[i];
              break;

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

              if (!sfLoadFile(sf, argv[i]) && errno != ENOENT)
              {
                sfPrintf(stderr, SFSTR("stringsutil: Unable to load '%s': %s"), argv[i], sfGetError(sf));
                return (1);
              }
              break;

          case 'l' : // -l LOCALE
              i ++;
              if (i >= argc)
              {
                sfPuts(stderr, SFSTR("stringsutil: Expected language code after '-l'."));
                return (usage(stderr, 1));
              }

              language = argv[i];
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
    else if (!strcmp(argv[i], "export") || !strcmp(argv[i], "import") || !strcmp(argv[i], "merge") || !strcmp(argv[i], "report") || !strcmp(argv[i], "scan") || !strcmp(argv[i], "translate"))
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
  else if (!strcmp(command, "translate"))
  {
    return (translate_strings(sf, sfname, url, apikey, language, files[0]));
  }

  return (0);
}


//
// 'compare_formats()' - Compare two format strings.
//

static bool				// O - `true` if equal, `false` otherwise
compare_formats(const char *s1,		// I - First string (key)
                const char *s2)		// I - Second string (localized text)
{
  // Compare the two formats - s1 and s2 point after the % and any positional parameters
  while (*s1 && *s2)
  {
    if (*s1 != *s2)
      return (false);

    if (isalpha(*s1 & 255))
      break;

    s1 ++;
    s2 ++;
  }

  return (true);
}


//
// 'decode_json()' - Decode an application/json object.
//

static int				// O - Number of JSON member variables or 0 on error
decode_json(const char    *data,	// I - JSON data
            cups_option_t **vars)	// O - JSON member variables or @code NULL@ on error
{
  int	num_vars = 0;			// Number of form variables
  char	name[1024],			// Variable name
	value[4096],			// Variable value
	*ptr,				// Pointer into value
	*end;				// End of value


  // Scan the string for "name":"value" pairs, unescaping values as needed.
  *vars = NULL;

  if (!data || *data != '{')
    return (0);

  data ++;

  while (*data)
  {
    // Skip leading whitespace/commas...
    while (*data && (isspace(*data & 255) || *data == ','))
      data ++;

    // Get the member variable name, unless we have the end of the object...
    if (*data == '}')
      break;
    else if (*data != '\"')
      goto decode_error;

    data = decode_string(data + 1, '\"', name, sizeof(name));

    if (*data != '\"')
      goto decode_error;

    data ++;

    if (*data != ':')
      goto decode_error;

    data ++;

    if (*data == '\"')
    {
      // Quoted string value...
      data = decode_string(data + 1, '\"', value, sizeof(value));

      if (*data != '\"')
	goto decode_error;

      data ++;
    }
    else if (*data == '[')
    {
      // Array value...
      ptr    = value;
      end    = value + sizeof(value) - 1;
      *ptr++ = '[';

      for (data ++; *data && *data != ']';)
      {
        if (*data == ',')
        {
          if (ptr < end)
            *ptr++ = *data++;
	  else
	    goto decode_error;
        }
        else if (*data == '\"')
        {
          // Quoted string value...
          do
          {
	    if (*data == '\\')
	    {
	      if (ptr < end)
		*ptr++ = *data++;
	      else
		goto decode_error;

              if (!strchr("\\\"/bfnrtu", *data))
                goto decode_error;

	      if (*data == 'u')
	      {
		if (ptr < (end - 5))
		  *ptr++ = *data++;
		else
		  goto decode_error;

	        if (isxdigit(data[0] & 255) && isxdigit(data[1] & 255) && isxdigit(data[2] & 255) && isxdigit(data[3] & 255))
	        {
		  *ptr++ = *data++;
		  *ptr++ = *data++;
		  *ptr++ = *data++;
		  /* 4th character is copied below */
	        }
	        else
	          goto decode_error;
	      }
	    }

	    if (ptr < end)
	      *ptr++ = *data++;
	    else
	      goto decode_error;
	  }
	  while (*data && *data != '\"');

	  if (*data == '\"')
	  {
	    if (ptr < end)
	      *ptr++ = *data++;
	    else
	      goto decode_error;
	  }
        }
        else if (*data == '{' || *data == '[')
        {
          // Unsupported nested array or object value...
	  goto decode_error;
	}
	else
	{
	  // Number, boolean, etc.
          while (*data && *data != ',' && !isspace(*data & 255))
          {
            if (ptr < end)
              *ptr++ = *data++;
	    else
	      goto decode_error;
	  }
	}
      }

      if (*data != ']' || ptr >= end)
        goto decode_error;

      *ptr++ = *data++;
      *ptr   = '\0';

      data ++;
    }
    else if (*data == '{')
    {
      // Unsupported object value...
      goto decode_error;
    }
    else
    {
      // Number, boolean, etc.
      for (ptr = value; *data && *data != ',' && !isspace(*data & 255); data ++)
        if (ptr < (value + sizeof(value) - 1))
          *ptr++ = *data;

      *ptr = '\0';
    }

    // Add the variable...
    num_vars = cupsAddOption(name, value, num_vars, vars);
  }

  return (num_vars);

  // If we get here there was an error in the form data...
  decode_error:

  cupsFreeOptions(num_vars, *vars);

  *vars = NULL;

  return (0);
}


//
// 'decode_string()' - Decode a URL-encoded string.
//

static const char *                     // O - New pointer into string
decode_string(const char *data,         // I - Pointer into data string
              char       term,          // I - Terminating character
              char       *buffer,       // I - String buffer
              size_t     bufsize)       // I - Size of string buffer
{
  int	ch;				// Current character
  char	*ptr,				// Pointer info buffer
	*end;				// Pointer to end of buffer


  for (ptr = buffer, end = buffer + bufsize - 1; *data && *data != term; data ++)
  {
    if ((ch = *data) == '\\')
    {
      // "\something" is an escaped character...
      data ++;

      switch (*data)
      {
        case '\\' :
            ch = '\\';
            break;
        case '\"' :
            ch = '\"';
            break;
        case '/' :
            ch = '/';
            break;
        case 'b' :
            ch = 0x08;
            break;
        case 'f' :
            ch = 0x0c;
            break;
        case 'n' :
            ch = 0x0a;
            break;
        case 'r' :
            ch = 0x0d;
            break;
        case 't' :
            ch = 0x09;
            break;
        case 'u' :
            data ++;
            if (isxdigit(data[0] & 255) && isxdigit(data[1] & 255) && isxdigit(data[2] & 255) && isxdigit(data[3] & 255))
            {
              if (isalpha(data[0]))
                ch = (tolower(data[0]) - 'a' + 10) << 12;
	      else
	        ch = (data[0] - '0') << 12;

              if (isalpha(data[1]))
                ch |= (tolower(data[1]) - 'a' + 10) << 8;
	      else
	        ch |= (data[1] - '0') << 8;

              if (isalpha(data[2]))
                ch |= (tolower(data[2]) - 'a' + 10) << 4;
	      else
	        ch |= (data[2] - '0') << 4;

              if (isalpha(data[3]))
                ch |= tolower(data[3]) - 'a' + 10;
	      else
	        ch |= data[3] - '0';

              data += 3;
              break;
            }

            // Fall through to default on error
	default :
	    *buffer = '\0';
	    return (NULL);
      }

      if (ch < 0x80)
      {
        // ASCII
        if (ptr < end)
          *ptr++ = (char)ch;
      }
      else if (ch < 0x400)
      {
        // 2-byte UTF-8
        if (ptr < (end - 1))
        {
          *ptr++ = (char)(0xc0 | (ch >> 6));
          *ptr++ = (char)(0x80 | (ch & 0x3f));
        }
      }
      else if (ch < 0x10000)
      {
        // 3-byte UTF-8
        if (ptr < (end - 2))
        {
          *ptr++ = (char)(0xe0 | (ch >> 12));
          *ptr++ = (char)(0x80 | ((ch >> 6) & 0x3f));
          *ptr++ = (char)(0x80 | (ch & 0x3f));
        }
      }
      else
      {
        // 4-byte UTF-8
        if (ptr < (end - 3))
        {
          *ptr++ = (char)(0xf0 | (ch >> 18));
          *ptr++ = (char)(0x80 | ((ch >> 12) & 0x3f));
          *ptr++ = (char)(0x80 | ((ch >> 6) & 0x3f));
          *ptr++ = (char)(0x80 | (ch & 0x3f));
        }
      }
    }
    else if (ch && ptr < end)
      *ptr++ = (char)ch;
  }

  *ptr = '\0';

  return (data);
}


//
// 'encode_json()' - Encode variables as a JSON object.
//
// The caller is responsible for calling @code free@ on the returned string.
//

static char *				// O - Encoded data or @code NULL@ on error
encode_json(
    int           num_vars,		// I - Number of JSON member variables
    cups_option_t *vars)		// I - JSON member variables
{
  char		buffer[65536],		// Temporary buffer
		*bufptr = buffer,	// Current position in buffer
		*bufend = buffer + sizeof(buffer) - 2;
					// End of buffer
  const char	*valptr;		// Pointer into value
  int		is_number;		// Is the value a number?

  *bufptr++ = '{';
  *bufend   = '\0';

  while (num_vars > 0)
  {
    bufptr = encode_string(vars->name, bufptr, bufend);

    if (bufptr >= bufend)
      return (NULL);

    *bufptr++ = ':';

    if (vars->value[0] == '[')
    {
      // Array value, already encoded...
      strncpy(bufptr, vars->value, bufend - bufptr);
      bufptr += strlen(bufptr);
    }
    else
    {
      is_number = 0;

      if (vars->value[0] == '-' || isdigit(vars->value[0] & 255))
      {
	valptr = vars->value + 1;
	while (*valptr && (isdigit(*valptr & 255) || *valptr == '.'))
	  valptr ++;

	if (*valptr == 'e' || *valptr == 'E' || !*valptr)
	  is_number = 1;
      }

      if (is_number)
      {
        // Copy number literal...
	for (valptr = vars->value; *valptr; valptr ++)
	{
	  if (bufptr < bufend)
	    *bufptr++ = *valptr;
	}
      }
      else
      {
        // Copy string value...
	bufptr = encode_string(vars->value, bufptr, bufend);
      }
    }

    num_vars --;
    vars ++;

    if (num_vars > 0)
    {
      if (bufptr >= bufend)
        return (NULL);

      *bufptr++ = ',';
    }
  }

  *bufptr++ = '}';
  *bufptr   = '\0';

  return (strdup(buffer));
}


//
// 'encode_string()' - URL-encode a string.
//
// The new buffer pointer can go past bufend, but we don't write past there...
//

static char *                           // O - New buffer pointer
encode_string(const char *s,            // I - String to encode
              char       *bufptr,       // I - Pointer into buffer
              char       *bufend)       // I - End of buffer
{
  if (bufptr < bufend)
    *bufptr++ = '\"';

  while (*s && bufptr < bufend)
  {
    if (*s == '\b')
    {
      *bufptr++ = '\\';
      if (bufptr < bufend)
        *bufptr++ = 'b';
    }
    else if (*s == '\f')
    {
      *bufptr++ = '\\';
      if (bufptr < bufend)
        *bufptr++ = 'f';
    }
    else if (*s == '\n')
    {
      *bufptr++ = '\\';
      if (bufptr < bufend)
        *bufptr++ = 'n';
    }
    else if (*s == '\r')
    {
      *bufptr++ = '\\';
      if (bufptr < bufend)
        *bufptr++ = 'r';
    }
    else if (*s == '\t')
    {
      *bufptr++ = '\\';
      if (bufptr < bufend)
        *bufptr++ = 't';
    }
    else if (*s == '\\')
    {
      *bufptr++ = '\\';
      if (bufptr < bufend)
        *bufptr++ = '\\';
    }
    else if (*s == '\"')
    {
      *bufptr++ = '\\';
      if (bufptr < bufend)
        *bufptr++ = '\"';
    }
    else if (*s >= ' ')
      *bufptr++ = *s;

    s ++;
  }

  if (bufptr < bufend)
    *bufptr++ = '\"';

  *bufptr = '\0';

  return (bufptr);
}


//
// 'export_strings()' - Export strings to a PO or C header file.
//

static int				// O - Exit status
export_strings(sf_t       *sf,		// I - Strings
	       const char *sfname,	// I - Strings filename
               const char *filename)	// I - Export filename
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
    const char *sfbase;			// Base name of strings filename...

    if ((sfbase = strrchr(sfname, '/')) != NULL)
      sfbase ++;
    else
      sfbase = sfname;

    fputs("static const char *", fp);
    while (*sfbase)
    {
      if (isalnum(*sfbase & 255))
        putc(*sfbase, fp);
      else
        putc('_', fp);

      sfbase ++;
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
import_string(sf_t *sf,			// I  - Strings
              char *msgid,		// I  - Key string
              char *msgstr,		// I  - Localized text
              char *comment,		// I  - Comment, if any
              bool addnew,		// I  - Add new strings?
              int  *added,		// IO - Number of added strings
              int  *ignored,		// IO - Number of ignored strings
              int  *modified)		// IO - Number of modified strings
{
  _sf_pair_t	*match;			// Matching existing entry
  bool		clear = false;		// Clear incoming strings?


  if (*msgid && *msgstr)
  {
    // See if this is an existing string...
    if ((match = _sfFindPair(sf, msgid)) != NULL)
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
      sfAddString(sf, msgid, msgstr, comment);
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
import_strings(sf_t       *sf,		// I - Strings
               const char *sfname,	// I - Strings filename
               const char *filename,	// I - Import filename
               bool       addnew)	// I - Add new strings?
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
    sf_t	*isf;			// Import strings
    _sf_pair_t	*pair,			// Existing pair
		*ipair;			// Imported pair
    size_t	count;			// Number of pairs

    isf = sfNew();
    if (!sfLoadFile(isf, filename))
    {
      sfPrintf(stderr, SFSTR("stringsutil: Unable to import '%s': %s"), filename, sfGetError(isf));
      sfDelete(isf);
      return (1);
    }

    for (count = isf->num_pairs, ipair = isf->pairs; count > 0; count --, ipair ++)
    {
      if ((pair = _sfFindPair(sf, ipair->key)) != NULL)
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
        sfAddString(sf, ipair->key, ipair->text, ipair->comment);
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
// 'matching_formats()' - Determine whether the key and localized text strings have matching formats.
//

static bool				// O - `true` if matching, `false` otherwise
matching_formats(const char *key,	// I - Key string
                 const char *text)	// I - Localized text
{
  int		i,			// Looping var
		num_fmts = 0;		// Number of formats
  const char	*keyfmts[100],		// Formats from key string
		*textfmts[100],		// Formats from text string
		*ptr;			// Pointer into string


  // First collect the formats from the key string...
  memset(keyfmts, 0, sizeof(keyfmts));
  for (ptr = strchr(key, '%'), i = 0; ptr && i < 100; ptr = strchr(ptr + 1, '%'))
  {
    ptr ++;
    if (*ptr == '%')
    {
      // Skip %%...
      ptr ++;
      continue;
    }

    if (isdigit(*ptr & 255) && ptr[1] == '$')
    {
      // 1-digit positional parameter
      i   = *ptr - '1';
      ptr += 2;
    }
    else if (isdigit(*ptr & 255) && isdigit(ptr[1] & 255) && ptr[2] == '$')
    {
      // 2-digit positional parameter
      i = (*ptr - '0') * 10 + ptr[1] - '1';
      ptr += 3;
    }

    keyfmts[i ++] = ptr;
    if (i > num_fmts)
      num_fmts = i;
  }

  if (ptr)
    return (false);

  // Then the formats for the text string...
  memset(textfmts, 0, sizeof(textfmts));
  for (ptr = strchr(text, '%'), i = 0; ptr && i < 100; ptr = strchr(ptr + 1, '%'))
  {
    ptr ++;
    if (*ptr == '%')
    {
      // Skip %%...
      ptr ++;
      continue;
    }

    if (isdigit(*ptr & 255) && ptr[1] == '$')
    {
      // 1-digit positional parameter
      i   = *ptr - '1';
      ptr += 2;
    }
    else if (isdigit(*ptr & 255) && isdigit(ptr[1] & 255) && ptr[2] == '$')
    {
      // 2-digit positional parameter
      i = (*ptr - '0') * 10 + ptr[1] - '1';
      ptr += 3;
    }

    if (i < num_fmts)
      textfmts[i ++] = ptr;
    else
      break;
  }

  if (ptr)
    return (false);

  // Then compare them all...
  for (i = 0; i < num_fmts; i ++)
  {
    if (!keyfmts[i] || !textfmts[0] || !compare_formats(keyfmts[i], textfmts[i]))
      return (false);
  }

  return (true);
}


//
// 'merge_strings()' - Merge strings from another strings file.
//

static int				// O - Exit status
merge_strings(sf_t       *sf,		// I - Strings
              const char *sfname,	// I - Strings filename
              const char *filename,	// I - Merge filename
              bool       clean)		// I - Clean old strings?
{
  sf_t		*msf;			// Strings file to merge
  _sf_pair_t	*pair,			// Current pair
		*mpair;			// Merge pair
  size_t	count;			// Remaining pairs
  int		added = 0,		// Number of added strings
		removed = 0;		// Number of removed strings


  // Open the merge file...
  msf = sfNew();
  if (!sfLoadFile(msf, filename))
  {
    sfPrintf(stderr, SFSTR("stringsutil: Unable to merge '%s': %s"), filename, sfGetError(msf));
    sfDelete(msf);
    return (1);
  }

  // Loop through the merge list and add any new strings...
  for (count = msf->num_pairs, mpair = msf->pairs; count > 0; count --, mpair ++)
  {
    if (_sfFindPair(sf, mpair->key))
      continue;

    added ++;
    sfAddString(sf, mpair->key, mpair->text, mpair->comment);
  }

  // Then clean old messages (if needed)...
  if (clean)
  {
    for (count = sf->num_pairs, pair = sf->pairs; count > 0; count --, pair ++)
    {
      if (_sfFindPair(msf, pair->key))
	continue;

      removed ++;

      _sfRemovePair(sf, pair);

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
report_strings(sf_t       *sf,		// I - Strings
               const char *filename)	// I - Comparison filename
{
  sf_t		*rsf;			// Strings file to report
  _sf_pair_t	*pair,			// Current pair
		*rpair;			// Report pair
  size_t	count;			// Number of pairs
  int		total,			// Total messages
		errors = 0,		// Number of errors
		translated = 0,		// Translated messages
		missing = 0,		// Missing messages
		old = 0,		// Old messages
		untranslated = 0;	// Messages not translated


  // Open the report file...
  rsf = sfNew();
  if (!sfLoadFile(rsf, filename))
  {
    sfPrintf(stderr, SFSTR("stringsutil: Unable to report on '%s': %s"), filename, sfGetError(rsf));
    sfDelete(rsf);
    return (1);
  }

  // Loop through the report list and check strings...
  for (count = rsf->num_pairs, rpair = rsf->pairs; count > 0; count --, rpair ++)
  {
    if ((pair = _sfFindPair(sf, rpair->key)) == NULL)
    {
      old ++;
      continue;
    }

    if (!matching_formats(rpair->key, rpair->text))
    {
      sfPrintf(stderr, SFSTR("stringsutil: Translated format string does not match '%s'."), rpair->key);
      errors ++;
    }

    if (strcmp(rpair->text, pair->text))
      translated ++;
    else
      untranslated ++;
  }

  // Then look for new messages that haven't been merged...
  for (count = sf->num_pairs, pair = sf->pairs; count > 0; count --, pair ++)
  {
    if (_sfFindPair(rsf, pair->key))
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

  return (untranslated > (total / 2) || errors > 0 ? 1 : 0);
}


//
// 'scan_files()' - Scan source files for localization strings.
//

static int				// O - Exit status
scan_files(sf_t       *sf,		// I - Strings
           const char *sfname,		// I - Strings filename
           const char *funcname,	// I - Localization function name
           int        num_files,	// I - Number of files
           const char *files[])		// I - Files
{
  size_t	fnlen;			// Length of function name
  int		i;			// Looping var
//  int		linenum;		// Line number in file
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

//    linenum = 0;

    while (fgets(line, sizeof(line), fp))
    {
      // Look for the function invocation...
//      linenum ++;

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
      if (!_sfFindPair(sf, text))
      {
        // No, add it!
        sfAddString(sf, text, text, comment[0] ? comment : NULL);
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
// 'translate_strings()' - Do a machine translation of key strings using a
//                         LibreTranslate service.
//

static int				// O - Exit status
translate_strings(sf_t       *sf,	// I - Strings
                  const char *sfname,	// I - Strings filename
                  const char *url,	// I - Translation service URL
                  const char *apikey,	// I - Translation service API key, if any
                  const char *language,	// I - Language code
                  const char *filename)	// I - Base strings filename
{
  http_t	*http;			// Connection to service
  char		scheme[32],		// URL scheme
		userpass[32],		// Username:password
		host[256],		// Hostname/IP address
		resource[256];		// Resource path
  int		port;			// Port number
  http_encryption_t encryption;		// Type of encryption to use
  int		num_request = 0,	// Number of request values
		num_response;		// Number of response values
  cups_option_t	*request,		// Request values
		*response;		// Response values
  char		*request_json,		// JSON data for request
		response_json[8192],	// JSON data in response
		*response_ptr;		// Pointer into response
  size_t	request_len,		// Request length
		response_len;		// Response length
  ssize_t	response_bytes;		// Size of JSON response
  const char	*value;			// Response value
  http_state_t	state;			// Current HTTP state
  sf_t		*base_sf;		// Base strings
  const char	*base_text;		// Base localized text
  _sf_pair_t	*pair;			// Current pair
  size_t	count;			// Number of pairs remaining
  int		changes = 0;		// Did we change any strings?
  int		num_formats;		// Number of format specifiers
  const char	*formats[100];		// Format specifiers
  char		text[2048],		// Current text string
		*textptr;		// Pointer into text string


  // Validate the translation URL...
  if (!url)
  {
    sfPuts(stderr, SFSTR("stringsutil: You must specify a LibreTranslate server with the '-t' option or the LIBRETRANSLATE_URL environment variable."));
    return (1);
  }

  if (httpSeparateURI(HTTP_URI_CODING_ALL, url, scheme, sizeof(scheme), userpass, sizeof(userpass), host, sizeof(host), &port, resource, sizeof(resource)) < HTTP_URI_STATUS_OK)
  {
    sfPrintf(stderr, SFSTR("stringsutil: Invalid LibreTranslate URL '%s'."), url);
    return (1);
  }

  // Validate the language code...
  if (!language)
  {
    sfPuts(stderr, SFSTR("stringsutil: You must specify a language code with the '-t' option."));
    return (1);
  }

  // Load the base localization...
  base_sf = sfNew();
  if (!sfLoadFile(base_sf, filename))
  {
    sfPrintf(stderr, SFSTR("stringsutil: Unable to translate from '%s': %s"), filename, sfGetError(base_sf));
    sfDelete(base_sf);
    return (1);
  }

  // Setup the JSON request values...
  if (apikey)
    num_request = cupsAddOption("api_key", apikey, num_request, &request);
  num_request = cupsAddOption("format", "text", num_request, &request);
  num_request = cupsAddOption("source", "en", num_request, &request);
  num_request = cupsAddOption("target", language, num_request, &request);

  // Connect to the server...
  if (!strcmp(scheme, "https") || port == 443)
    encryption = HTTP_ENCRYPTION_ALWAYS;
  else
    encryption = HTTP_ENCRYPTION_IF_REQUESTED;

  if ((http = httpConnect(host, port, NULL, AF_UNSPEC, encryption, 1, 30000, NULL)) == NULL)
  {
    sfPrintf(stderr, SFSTR("stringsutil: Unable to connect to '%s': %s"), url, cupsGetErrorString());
    cupsFreeOptions(num_request, request);
    sfDelete(base_sf);
    return (1);
  }

  // Loop through the strings...
  for (count = sf->num_pairs, pair = sf->pairs; count > 0; count --, pair ++)
  {
    // See if this string needs to be localized...
    if ((base_text = sfGetString(base_sf, pair->key)) == NULL)
    {
      sfPrintf(stderr, SFSTR("stringsutil: Ignoring old string '%s'..."), pair->key);
      continue;
    }

    if (strcmp(base_text, pair->text))
      continue;

    // Try translating it...
    sfPrintf(stdout, SFSTR("stringsutil: Translating '%s'..."), pair->key);

    num_formats = 0;

    if ((value = strchr(pair->text, '%')) != NULL)
    {
      // Convert % formats to _F#_ to avoid translation
      for (value = pair->text, textptr = text; *value;)
      {
        // Copy non-format text...
        while (*value && *value != '%' && textptr < (text + sizeof(text) - 1))
	  *textptr++ = *value++;

        if (!*value)
          break;

        // Replace % with _F#_...
	snprintf(textptr, sizeof(text) - (textptr - text), " _F%d ", num_formats);
	textptr += strlen(textptr);

	formats[num_formats ++] = value;
	if (num_formats >= 100)
	  break;

        // Skip format specifier...
	for (value ++; *value; value ++)
	{
	  if (strchr("aAcCdDeEfFgGinoOpsSuUxX%", *value))
	    break;
	}

	if (*value)
	  value ++;
	else
	  break;
      }

      *textptr = '\0';

      // Add reformatted text...
      num_request = cupsAddOption("q", text, num_request, &request);
    }
    else
    {
      // No format strings...
      num_request = cupsAddOption("q", pair->text, num_request, &request);
    }

    request_json = encode_json(num_request, request);
    request_len  = strlen(request_json);

    httpSetField(http, HTTP_FIELD_CONTENT_TYPE, "application/json");
    httpSetLength(http, request_len);
#if CUPS_VERSION_MAJOR > 2
    if (!httpWriteRequest(http, "POST", "/translate"))
#else
    if (httpPost(http, "/translate"))
#endif // CUPS_VERSION_MAJOR > 2
    {
      if (httpReconnect(http, 30000, NULL))
      {
	sfPrintf(stderr, SFSTR("stringutil: Lost connection to translation server: %s"), cupsGetErrorString());
	free(request_json);
	break;
      }
#if CUPS_VERSION_MAJOR > 2
      else if (!httpWriteRequest(http, "POST", "/translate"))
#else
      else if (httpPost(http, "/translate"))
#endif // CUPS_VERSION_MAJOR > 2
      {
	sfPrintf(stderr, SFSTR("stringutil: Unable to send translation request: %s"), cupsGetErrorString());
	free(request_json);
	break;
      }
    }

    if (httpWrite(http, request_json, request_len) < (ssize_t)request_len)
    {
      sfPrintf(stderr, SFSTR("stringutil: Unable to send translation request: %s"), cupsGetErrorString());
      free(request_json);
      break;
    }

    free(request_json);

    // Wait for the response...
    while (httpUpdate(http) == HTTP_STATUS_CONTINUE)
      ;

    state = httpGetState(http);
    if ((response_len = httpGetLength(http)) == 0 || response_len > (sizeof(response_json) - 1))
      response_len = sizeof(response_len) - 1;

    for (response_ptr = response_json; response_ptr < (response_json + sizeof(response_json) - 1); response_ptr += response_bytes, response_len -= (size_t)response_bytes)
    {
      if ((response_bytes = httpRead(http, response_ptr, response_len)) <= 0)
        break;
    }
    *response_ptr = '\0';

    if (httpGetState(http) == state)
      httpFlush(http);			// Flush any remaining data...

    // Decode the response...
    num_response = decode_json(response_json, &response);
    if ((value = cupsGetOption("translatedText", num_response, response)) != NULL && *value)
    {
      // Translated, replace the localized text...
      if (num_formats > 0)
      {
        // Replace format strings...
	long		n;		// Format index
	const char	*fmtptr;	// Pointer into format string

        for (textptr = text; *value && textptr < (text + sizeof(text) - 1);)
        {
          if (!strncmp(value, "_F", 2) && isdigit(value[2] & 255))
          {
            if ((n = strtol(value + 2, (char **)&value, 10)) < num_formats)
            {
              *textptr++ = '%';

              for (fmtptr = formats[n] + 1; *fmtptr && textptr < (text + sizeof(text) - 1); fmtptr ++)
              {
                *textptr++ = *fmtptr;
		if (strchr("aAcCdDeEfFgGinoOpsSuUxX%", *fmtptr))
		  break;
              }
            }

            if (!*value)
              break;
          }
          else
	    *textptr++ = *value++;
        }

        *textptr = '\0';
        value    = text;
      }

      sfPrintf(stdout, SFSTR("stringsutil: Localized as '%s'."), value);
      free(pair->text);
      pair->text = strdup(value);
      changes ++;
    }
    else
    {
      // Not translated, show error...
      value = cupsGetOption("error", num_response, response);
      sfPrintf(stderr, SFSTR("stringsutil: Unable to translate: %s"), value ? value : "???");
    }

    cupsFreeOptions(num_response, response);
  }

  // Cleanup...
  sfPrintf(stdout, SFSTR("stringsutil: Translated %d string(s)."), changes);

  if (changes > 0)
    write_strings(sf, sfname);

  cupsFreeOptions(num_request, request);
  sfDelete(base_sf);
  httpClose(http);

  return (0);
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
  sfPuts(fp, SFSTR("  -A API-KEY           Specify LibreTranslate API key."));
  sfPuts(fp, SFSTR("  -c                   Remove old strings (merge)."));
  sfPuts(fp, SFSTR("  -f FILENAME.strings  Specify strings file."));
  sfPuts(fp, SFSTR("  -l LOCALE            Specify locale/language ID."));
  sfPuts(fp, SFSTR("  -n NAME              Specify function/macro name for localization."));
  sfPuts(fp, SFSTR("  -T URL               Specify LibreTranslate server URL."));
  sfPuts(fp, SFSTR("  --help               Show program help."));
  sfPuts(fp, SFSTR("  --version            Show program version."));
  puts("");
  sfPuts(fp, SFSTR("Commands:"));
  sfPuts(fp, SFSTR("  export               Export strings to GNU gettext .po or C source file."));
  sfPuts(fp, SFSTR("  import               Import strings from GNU gettext .po or .strings file."));
  sfPuts(fp, SFSTR("  merge                Merge strings from another strings file."));
  sfPuts(fp, SFSTR("  report               Report untranslated strings in the specified strings file(s)."));
  sfPuts(fp, SFSTR("  scan                 Scan C/C++ source files for strings."));
  sfPuts(fp, SFSTR("  translate            Translate strings."));

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
      fprintf(fp, "%s%s\"", escape, code ? "\\" : "");
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
write_strings(sf_t       *sf,		// I - Strings
              const char *sfname)	// I - Strings filename
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
