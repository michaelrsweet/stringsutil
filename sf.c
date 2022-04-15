//
// Common functions for StringsUtil.
//
// Copyright © 2022 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "sf-private.h"
#include <stdarg.h>
#include <locale.h>


//
// Local globals...
//

static sf_t	*sf_default = NULL;	// Default localization
static char	sf_locale[8] = "";	// Default locale


//
// Local functions...
//

static int	sf_compare_pairs(_sf_pair_t *a, _sf_pair_t *b);
static void	sf_free_pair(_sf_pair_t *pair);


//
// '_sfAdd()' - Add a key/text pair...
//

bool					// O - `true` on success, `false` on error
_sfAdd(sf_t       *sf,			// I - Strings
       const char *key,			// I - Key
       const char *text,		// I - Text
       const char *comment)		// I - Comment, if any
{
  _sf_pair_t	*pair;			// New pair


  _sf_rwlock_wrlock(sf->rwlock);

  if (sf->num_pairs >= sf->alloc_pairs)
  {
    if ((pair = realloc(sf->pairs, (sf->alloc_pairs + 32) * sizeof(_sf_pair_t))) == NULL)
    {
      _sfSetError(sf, "Unable to allocate memory for pair.");
      _sf_rwlock_unlock(sf->rwlock);
      return (false);
    }

    sf->pairs = pair;
    sf->alloc_pairs += 32;
  }

  pair          = sf->pairs + sf->num_pairs;
  pair->key     = strdup(key);
  pair->text    = strdup(text);
  pair->comment = comment && *comment ? strdup(comment) : NULL;

  if (!pair->key || !pair->text || (!pair->comment && comment && *comment))
  {
    _sfSetError(sf, "Unable to copy strings.");
    _sf_rwlock_unlock(sf->rwlock);
    return (false);
  }

  sf->num_pairs ++;
  sf->need_sort = sf->num_pairs > 1;

  _sf_rwlock_unlock(sf->rwlock);

  return (true);
}


//
// 'sfDelete()' - Free a collection of localization strings.
//
// This function frees all memory associated with the localization strings.
//

void
sfDelete(sf_t *sf)			// I - Localization strings
{
  _sf_pair_t	*pair;			// Current pair
  size_t	count;			// Number of pairs


  // Range check input...
  if (!sf)
    return;

  // Free memory...
  _sf_rwlock_destroy(sf->rwlock);

  for (count = sf->num_pairs, pair = sf->pairs; count > 0; count --, pair ++)
    sf_free_pair(pair);

  free(sf->pairs);
  free(sf);
}


//
// '_sfFind()' - Find a pair.
//

_sf_pair_t *				// O - Matching pair or `NULL`
_sfFind(sf_t       *sf,			// I - Strings
        const char *key)		// I - Key
{
  _sf_pair_t	pair,			// Search key
		*match;			// Matching pair


  // Sort as needed...
  if (sf->need_sort)
  {
    _sf_rwlock_wrlock(sf->rwlock);

    if (sf->need_sort)
      qsort(sf->pairs, sf->num_pairs, sizeof(_sf_pair_t), (int (*)(const void *, const void *))sf_compare_pairs);

    sf->need_sort = false;
  }
  else
  {
    _sf_rwlock_rdlock(sf->rwlock);
  }

  pair.key = (char *)key;

  match = (_sf_pair_t *)bsearch(&pair, sf->pairs, sf->num_pairs, sizeof(_sf_pair_t), (int (*)(const void *, const void *))sf_compare_pairs);

  _sf_rwlock_unlock(sf->rwlock);

  return (match);
}


//
// 'sfFormatString()' - Format a localized string.
//
// This function formats a printf-style localized string using the specified
// localization strings.  If no localization exists for the format (key) string,
// the original string is used.  All `snprintf` format specifiers are supported.
//
// The default localization strings ("sf" passed as `NULL`) are initialized
// using the @link sfSetLocale@, @link sfRegisterDirectory@, and
// @link sfRegisterString@ functions.
//

const char *				// O - Formatted localized string
sfFormatString(sf_t       *sf,		// I - Localization strings or `NULL` for default
               char       *buffer,	// I - Output buffer
               size_t     bufsize,	// I - Size of output buffer
               const char *key,		// I - Printf-style format/key string
               ...)			// I - Additional arguments as needed
{
  va_list	ap;			// Argument pointer


  // Range-check input
  if (!buffer || bufsize < 10 || !key)
  {
    if (buffer)
      *buffer = '\0';
    return (NULL);
  }

  // Format string
  va_start(ap, key);
  vsnprintf(buffer, bufsize, sfGetString(sf, key), ap);
  va_end(ap);

  return (buffer);
}


//
// 'sfGetError()' - Get the last error message, if any.
//
// This function returns the last error from @link sfLoadFromFile@ and
// @link sfLoadFromString@, if any.
//

const char *				// O - Last error message or `NULL` for none
sfGetError(sf_t *sf)			// I - Localization strings
{
  if (!sf || !sf->error[0])
    return (NULL);
  else
    return (sf->error);
}


//
// 'sfGetString()' - Lookup a localized string.
//
// This function looks up a localized string for the specified key string.
// If no localization exists, the key string is returned.
//
// The default localization strings ("sf" passed as `NULL`) are initialized
// using the @link sfSetLocale@, @link sfRegisterDirectory@, and
// @link sfRegisterString@ functions.
//

const char *				// O - Localized string
sfGetString(sf_t       *sf,		// I - Localization strings or `NULL` for the default
            const char *key)		// I - Key string
{
  _sf_pair_t	*match;			// Matching pair, if any


  // Range check input...
  if (!key || (!sf && !sf_default))
    return (key);

  // Look up the key...
  match = _sfFind(sf ? sf : sf_default, key);

  // Return a string to use...
  return (match ? match->text : key);
}


//
// 'sfLoadFromFile()' - Load a ".strings" file.
//
// This function loads a ".strings" file.  The "sf" argument specifies a
// collection of localization strings that was created using the @link sfNew@
// function.
//
// When loading the strings, any existing strings in the collection are left
// unchanged.
//

bool					// O - `true` on success, `false` on failure
sfLoadFromFile(sf_t       *sf,		// I - Localization strings
               const char *filename)	// I - File to load
{
  bool		ret;			// Return value
  int		fd;			// File descriptor
  struct stat	fileinfo;		// File information
  ssize_t	bytes;			// Bytes read
  char		*data;			// File contents


  // Range check input...
  if (!sf || !filename)
  {
    errno = EINVAL;
    return (false);
  }

  // Open the file...
  if ((fd = open(filename, O_RDONLY)) < 0)
  {
    _sfSetError(sf, "Unable to open '%s': %s", filename, strerror(errno));
    return (false);
  }

  // Get the file size...
  if (fstat(fd, &fileinfo))
  {
    _sfSetError(sf, "Unable to stat '%s': %s", filename, strerror(errno));
    close(fd);
    return (false);
  }

  // Allocate a buffer for the file...
  if ((data = malloc((size_t)(fileinfo.st_size + 1))) == NULL)
  {
    _sfSetError(sf, "Unable to allocate %u bytes for '%s': %s", (unsigned)(fileinfo.st_size + 1), filename, strerror(errno));
    close(fd);
    return (false);
  }

  // Read the file...
  if ((bytes = read(fd, data, (size_t)fileinfo.st_size)) < 0)
  {
    _sfSetError(sf, "Unable to read '%s': %s", filename, strerror(errno));
    close(fd);
    free(data);
    return (false);
  }
#ifdef DEBUG
  else if (bytes < (ssize_t)fileinfo.st_size)
  {
    // Short read, shouldn't happen but warn if it does...
    fprintf(stderr, "sfLoadFromFile: Only read %u of %u bytes from '%s'.", (unsigned)bytes, (unsigned)fileinfo.st_size, filename);
  }
#endif // DEBUG

  close(fd);

  data[bytes] = '\0';

  // Load it...
  ret = sfLoadFromString(sf, data);

  // Free buffer and return...
  free(data);

  return (ret);
}


//
// 'sfLoadFromString()' - Load a ".strings" file from a compiled-in string.
//
// This function loads a ".strings" file from a compiled-in string.  The "sf"
// argument specifies a collection of localization strings that was created
// using the @link sfNew@ function.
//
// When loading the strings, any existing strings in the collection are left
// unchanged.
//

bool					// O - `true` on success, `false` on failure
sfLoadFromString(sf_t       *sf,	// I - Localization strings
                 const char *data)	// I - Data to load
{
  const char	*dataptr;		// Pointer into string data
  char		key[1024],		// Key string
		text[1024],		// Localized text string
		comment[1024] = "",	// Comment string, if any
		*ptr;			// Pointer into strings
  int		linenum;		// Line number


  // Scan the in-memory strings data and add key/text pairs...
  //
  // Format of strings files is:
  //
  // / * optional comment * /
  // "key" = "text";
  for (dataptr = data, linenum = 1; *dataptr; dataptr ++)
  {
    // Skip leading whitespace...
    while (*dataptr && isspace(*dataptr & 255))
    {
      if (*dataptr == '\n')
        linenum ++;

      dataptr ++;
    }

    if (!*dataptr)
    {
      // End of string...
      break;
    }
    else if (*dataptr == '/' && dataptr[1] == '*')
    {
      // Start of C-style comment...
      for (dataptr += 2; *dataptr && isspace(*dataptr & 255); dataptr ++)
      {
        // Skip leading whitespace...
        if (*dataptr == '\n')
          linenum ++;
      }

      for (ptr = comment; *dataptr; dataptr ++)
      {
        if (*dataptr == '*' && dataptr[1] == '/')
	{
	  dataptr += 2;
	  break;
	}
	else if (ptr < (comment + sizeof(comment) - 1))
	{
	  *ptr++ = *dataptr;
	}

	if (*dataptr == '\n')
	  linenum ++;
      }

      if (!*dataptr)
        break;

      *ptr = '\0';
      while (ptr > comment && isspace(ptr[-1] & 255))
        *--ptr = '\0';			// Strip trailing whitespace

      continue;
    }
    else if (*dataptr != '\"')
    {
      // Something else we don't recognize...
      _sfSetError(sf, "sfLoadString: Syntax error on line %d.", linenum);
      return (false);
    }

    // Parse key string...
    dataptr ++;
    for (ptr = key; *dataptr && *dataptr != '\"'; dataptr ++)
    {
      if (*dataptr == '\\' && dataptr[1])
      {
        // Escaped character...
        int	ch;			// Character

        dataptr ++;
        if (*dataptr == '\\' || *dataptr == '\'' || *dataptr == '\"')
        {
          ch = *dataptr;
	}
	else if (*dataptr == 'n')
	{
	  ch = '\n';
	}
	else if (*dataptr == 'r')
	{
	  ch = '\r';
	}
	else if (*dataptr == 't')
	{
	  ch = '\t';
	}
	else if (*dataptr >= '0' && *dataptr <= '3' && dataptr[1] >= '0' && dataptr[1] <= '7' && dataptr[2] >= '0' && dataptr[2] <= '7')
	{
	  // Octal escape
	  ch = ((*dataptr - '0') << 6) | ((dataptr[1] - '0') << 3) | (dataptr[2] - '0');
	  dataptr += 2;
	}
	else
	{
          _sfSetError(sf, "sfLoadString: Invalid escape in key string on line %d.", linenum);
	  return (false);
	}

        if (ptr < (key + sizeof(key) - 1))
          *ptr++ = (char)ch;
      }
      else if (ptr < (key + sizeof(key) - 1))
      {
        *ptr++ = *dataptr;
      }
    }

    if (!*dataptr)
    {
      _sfSetError(sf, "sfLoadString: Unterminated key string on line %d.", linenum);
      return (false);
    }

    dataptr ++;
    *ptr = '\0';

    // Parse separator...
    while (*dataptr && isspace(*dataptr & 255))
    {
      if (*dataptr == '\n')
        linenum ++;

      dataptr ++;
    }

    if (*dataptr != '=')
    {
      _sfSetError(sf, "sfLoadString: Missing separator on line %d (saw '%c' at offset %ld).", linenum, *dataptr, dataptr - data);
      return (false);
    }

    dataptr ++;
    while (*dataptr && isspace(*dataptr & 255))
    {
      if (*dataptr == '\n')
        linenum ++;

      dataptr ++;
    }

    if (*dataptr != '\"')
    {
      _sfSetError(sf, "sfLoadString: Missing text string on line %d.", linenum);
      return (false);
    }

    // Parse text string...
    dataptr ++;
    for (ptr = text; *dataptr && *dataptr != '\"'; dataptr ++)
    {
      if (*dataptr == '\\')
      {
        // Escaped character...
        int	ch;			// Character

        dataptr ++;
        if (*dataptr == '\\' || *dataptr == '\'' || *dataptr == '\"')
        {
          ch = *dataptr;
	}
	else if (*dataptr == 'n')
	{
	  ch = '\n';
	}
	else if (*dataptr == 'r')
	{
	  ch = '\r';
	}
	else if (*dataptr == 't')
	{
	  ch = '\t';
	}
	else if (*dataptr >= '0' && *dataptr <= '3' && dataptr[1] >= '0' && dataptr[1] <= '7' && dataptr[2] >= '0' && dataptr[2] <= '7')
	{
	  // Octal escape
	  ch = ((*dataptr - '0') << 6) | ((dataptr[1] - '0') << 3) | (dataptr[2] - '0');
	  dataptr += 2;
	}
	else
	{
          _sfSetError(sf, "sfLoadString: Invalid escape in text string on line %d.", linenum);
	  return (false);
	}

        if (ptr < (text + sizeof(text) - 1))
          *ptr++ = (char)ch;
      }
      else if (ptr < (text + sizeof(text) - 1))
      {
        *ptr++ = *dataptr;
      }
    }

    if (!*dataptr)
    {
      _sfSetError(sf, "sfLoadString: Unterminated text string on line %d.", linenum);
      return (false);
    }

    dataptr ++;
    *ptr = '\0';

    // Look for terminator, then add the pair...
    if (*dataptr != ';')
    {
      _sfSetError(sf, "sfLoadString: Missing terminator on line %d.", linenum);
      return (false);
    }

    dataptr ++;

    if (!_sfFind(sf, key))
    {
      if (!_sfAdd(sf, key, text, comment))
        return (false);
    }

    comment[0] = '\0';
  }

  sf->error[0] = '\0';

  return (true);
}


//
// 'sfNew()' - Create a new (empty) set of localization strings.
//
// This function creates a new (empty) set of localization strings.  Use the
// @link sfLoadFromFile@ and/or @link sfLoadFromString@ functions to load
// localization strings.
//

sf_t *					// O - Localization strings
sfNew(void)
{
  sf_t *sf = (sf_t *)calloc(1, sizeof(sf_t));
					// Localization strings


  if (sf)
    _sf_rwlock_init(sf->rwlock);

  return (sf);
}


//
// 'sfPrintf()' - Print a formatted localized message followed by a newline.
//
// This function prints a formatted localized message followed by a newline to
// the specified file, typically `stdout` or `stderr`.  You must call
// @link sfSetLocale@ and @link sfRegisterString@ or @link sfRegisterDirectory@
// to initialize the message catalog that is used.
//

void
sfPrintf(FILE       *fp,		// I - Output file
         const char *message,		// I - Printf-style message
         ...)				// I - Additional arguments as needed
{
  va_list	ap;			// Pointer to arguments


  va_start(ap, message);
  vfprintf(fp, sfGetString(sf_default, message), ap);
  putc('\n', fp);
  va_end(ap);
}


//
// 'sfPuts()' - Print a localized message followed by a newline.
//
// This function prints a localized message followed by a newline to the
// specified file, typically `stdout` or `stderr`.  You must call
// @link sfSetLocale@ and @link sfRegisterString@ or @link sfRegisterDirectory@
// to initialize the message catalog that is used.
//

void
sfPuts(FILE       *fp,			// I - Output file
       const char *message)		// I - Message
{
  fputs(sfGetString(sf_default, message), fp);
  putc('\n', fp);
}


//
// 'sfRegisterDirectory()' - Register ".strings" files in a directory.
//
// This function registers ".strings" files in a directory.  You must call
// @link sfSetLocale@ first to initialize the current locale.
//

void
sfRegisterDirectory(
    const char *directory)		// I - Directory of .strings files
{
  char	filename[1024];			// .strings filename


  if (!sf_locale[0])
    return;

  snprintf(filename, sizeof(filename), "%s/%s.strings", directory, sf_locale);
  if (!sfLoadFromFile(sf_default, filename))
  {
    snprintf(filename, sizeof(filename), "%s/%2s.strings", directory, sf_locale);
    sfLoadFromFile(sf_default, filename);
  }
}


//
// 'sfRegisterString()' - Register a ".strings" file from a compiled-in string.
//
// This function registers a ".strings" file from a compiled-in string.  You
// must call @link sfSetLocale@ first to initialize the current locale.
//

void
sfRegisterString(const char *locale,	// I - Locale
                 const char *data)	// I - Strings data
{
  if (!sf_locale[0])
    return;

  if (!strcmp(locale, sf_locale) || (strlen(locale) == 2 && !strncmp(locale, sf_locale, 2)))
    sfLoadFromString(sf_default, data);
}


//
// '_sfRemove()' - Remove a pair from a strings file.
//

void
_sfRemove(sf_t   *sf,			// I - Localization strings
          size_t n)			// I - Pair index
{
  // Free memory and then squeeze array as needed...
  _sf_rwlock_wrlock(sf->rwlock);

  sf_free_pair(sf->pairs + n);

  sf->num_pairs --;

  if (n < sf->num_pairs)
    memmove(sf->pairs + n, sf->pairs + n + 1, (sf->num_pairs - n) * sizeof(_sf_pair_t));

  _sf_rwlock_unlock(sf->rwlock);
}


//
// '_sfSetError()' - Set the current error message.
//

void
_sfSetError(sf_t       *sf,		// I - Localization strings
            const char *message,	// I - Printf-style error string
            ...)			// I - Additional arguments as needed
{
  va_list	ap;			// Pointer to arguments


  va_start(ap, message);
  vsnprintf(sf->error, sizeof(sf->error), message, ap);
  va_end(ap);
}


//
// 'sfSetLocale()' - Set the current locale.
//
// This function calls `setlocale` to initialize the current locale based on
// the current user session, and then creates an empty message catalog that is
// filled by calls to @link sfRegisterDirectory@ and/or @link sfRegisterString@.
//

void
sfSetLocale(void)
{
  const char	*locale,		// Current locale
		*charset;		// Character set in sf_locale
  char		*ptr;			// Pointer into locale name


  // Only initialize once...
  if (sf_default)
    return;

  // Create an empty strings file object...
  sf_default = sfNew();

  // Set the current locale...
  if ((locale = setlocale(LC_ALL, "")) == NULL || !strcmp(locale, "C") || !strncmp(locale, "C/", 2))
    locale = "en";

  // If the locale name has a character set specified and it is not UTF-8, then
  // just output plain English...
  if ((charset = strchr(locale, '.')) != NULL && strcmp(charset, ".UTF-8"))
    locale = "en";

  // Save the locale name minus the character set...
  strncpy(sf_locale, locale, sizeof(sf_locale) - 1);
  sf_locale[sizeof(sf_locale) - 1] = '\0';

  if ((ptr = strchr(sf_locale, '.')) != NULL)
    *ptr = '\0';
}


//
// 'sf_compare_pairs()' - Compare the keys of two key/text pairs.
//

static int				// O - Result of comparison
sf_compare_pairs(_sf_pair_t *a,		// I - First key/text pair
	         _sf_pair_t *b)		// I - Second key/text pair
{
  return (strcmp(a->key, b->key));
}


//
// 'sf_free_pair()' - Free memory used by a key/text pair.
//

static void
sf_free_pair(_sf_pair_t *pair)		// I - Pair
{
  free(pair->key);
  free(pair->text);
  free(pair->comment);
}