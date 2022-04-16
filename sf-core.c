//
// Core strings file functions for StringsUtil.
//
// Copyright © 2022 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "sf-private.h"
#include <stdarg.h>


//
// Local functions...
//

static int	sf_compare_pairs(_sf_pair_t *a, _sf_pair_t *b);
static void	sf_free_pair(_sf_pair_t *pair);
static void	sf_sort(sf_t *sf);


//
// '_sfAddPair()' - Add a pair to a strings file.
//

_sf_pair_t *				// O - New pair or `NULL` on error
_sfAddPair(sf_t       *sf,		// I - Localization strings
           const char *key,		// I - Key string
           const char *text,		// I - Text string
           const char *comment)		// I - Comment or `NULL` for none
{
  _sf_pair_t	*pair;			// New pair


  if (sf->num_pairs >= sf->alloc_pairs)
  {
    if ((pair = realloc(sf->pairs, (sf->alloc_pairs + 32) * sizeof(_sf_pair_t))) == NULL)
    {
      _sfSetError(sf, "Unable to allocate memory for pair.");
      _sf_rwlock_unlock(sf->rwlock);
      return (NULL);
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
    return (NULL);
  }

  sf->num_pairs ++;
  sf->need_sort = sf->num_pairs > 1;

  return (pair);
}


//
// 'sfAddString()' - Add a localization string.
//
// This function adds a localization string to the collection.
//

bool					// O - `true` on success, `false` on error
sfAddString(sf_t       *sf,		// I - Strings
	    const char *key,		// I - Key
	    const char *text,		// I - Text
	    const char *comment)	// I - Comment or `NULL` for none
{
  _sf_pair_t	*pair;			// New pair


  // Range check input...
  if (!sf || !key || !text)
    return (false);

  _sf_rwlock_wrlock(sf->rwlock);

  pair = _sfAddPair(sf, key, text, comment);

  if (sf->need_sort)
    sf_sort(sf);

  _sf_rwlock_unlock(sf->rwlock);

  return (pair != NULL);
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
// '_sfFindPair()' - Find a pair.
//

_sf_pair_t *				// O - Matching pair or `NULL`
_sfFindPair(sf_t       *sf,		// I - Strings
            const char *key)		// I - Key
{
  _sf_pair_t	pair;			// Search key


  pair.key = (char *)key;

  return ((_sf_pair_t *)bsearch(&pair, sf->pairs, sf->num_pairs, sizeof(_sf_pair_t), (int (*)(const void *, const void *))sf_compare_pairs));
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
// This function returns the last error from @link sfLoadFile@ and
// @link sfLoadString@, if any.
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
  const char	*s;			// Matching string


  // Range check input...
  if (!sf)
    sf = _sfGetDefault();

  if (!key || !sf)
    return (key);

  // Look up the key...
  _sf_rwlock_rdlock(sf->rwlock);
  if ((match = _sfFindPair(sf, key)) != NULL)
    s = match->text;
  else
    s = key;
  _sf_rwlock_unlock(sf->rwlock);

  // Return a string to use...
  return (s);
}


//
// 'sfHasString()' - Determine whether a string is localized.
//
// This function looks up a localization string, returning `true` if the string
// exists and `false` otherwise.
//

bool					// O - `true` if the string exists, `false` otherwise
sfHasString(sf_t       *sf,		// I - Localization strings
            const char *key)		// I - Key string
{
  _sf_pair_t	*match;			// Matching pair, if any


  // Range check input...
  if (!sf)
    sf = _sfGetDefault();

  if (!key || !sf)
    return (key);

  // Look up the key...
  _sf_rwlock_rdlock(sf->rwlock);
  match = _sfFindPair(sf, key);
  _sf_rwlock_unlock(sf->rwlock);

  return (match != NULL);
}


//
// 'sfLoadFile()' - Load a ".strings" file.
//
// This function loads a ".strings" file.  The "sf" argument specifies a
// collection of localization strings that was created using the @link sfNew@
// function.
//
// When loading the strings, any existing strings in the collection are left
// unchanged.
//

bool					// O - `true` on success, `false` on failure
sfLoadFile(sf_t       *sf,		// I - Localization strings
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
    fprintf(stderr, "sfLoadFile: Only read %u of %u bytes from '%s'.", (unsigned)bytes, (unsigned)fileinfo.st_size, filename);
  }
#endif // DEBUG

  close(fd);

  data[bytes] = '\0';

  // Load it...
  ret = sfLoadString(sf, data);

  // Free buffer and return...
  free(data);

  return (ret);
}


//
// 'sfLoadString()' - Load a ".strings" file from a compiled-in string.
//
// This function loads a ".strings" file from a compiled-in string.  The "sf"
// argument specifies a collection of localization strings that was created
// using the @link sfNew@ function.
//
// When loading the strings, any existing strings in the collection are left
// unchanged.
//

bool					// O - `true` on success, `false` on failure
sfLoadString(sf_t       *sf,		// I - Localization strings
	     const char *data)		// I - Data to load
{
  const char	*dataptr;		// Pointer into string data
  char		key[1024],		// Key string
		text[1024],		// Localized text string
		comment[1024] = "",	// Comment string, if any
		*ptr;			// Pointer into strings
  int		linenum;		// Line number


  // Range check input...
  if (!sf || !data)
  {
    if (sf)
      _sfSetError(sf, "No data.");

    return (false);
  }

  _sf_rwlock_wrlock(sf->rwlock);

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
      goto error;
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
	  goto error;
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
      goto error;
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
      goto error;
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
      goto error;
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
	  goto error;
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
      goto error;
    }

    dataptr ++;
    *ptr = '\0';

    // Look for terminator, then add the pair...
    if (*dataptr != ';')
    {
      _sfSetError(sf, "sfLoadString: Missing terminator on line %d.", linenum);
      goto error;
    }

    dataptr ++;

    if (!_sfFindPair(sf, key))
    {
      if (!_sfAddPair(sf, key, text, comment))
	goto error;
    }

    comment[0] = '\0';
  }

  sf->error[0] = '\0';

  if (sf->need_sort)
    sf_sort(sf);

  _sf_rwlock_unlock(sf->rwlock);

  return (true);

  // If we get here there was an error..
  error:

  if (sf->need_sort)
    sf_sort(sf);

  _sf_rwlock_unlock(sf->rwlock);

  return (false);
}


//
// 'sfNew()' - Create a new (empty) set of localization strings.
//
// This function creates a new (empty) set of localization strings.  Use the
// @link sfLoadFile@ and/or @link sfLoadString@ functions to load
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
// '_sfRemovePair()' - Remove a pair from a strings file.
//

void
_sfRemovePair(sf_t       *sf,		// I - Localization strings
              _sf_pair_t *pair)		// I - Pair to remove
{
  size_t	n = pair - sf->pairs;	// Pair index


  // Free memory and then squeeze array as needed...
  sf_free_pair(pair);

  sf->num_pairs --;

  if (n < sf->num_pairs)
    memmove(sf->pairs + n, sf->pairs + n + 1, (sf->num_pairs - n) * sizeof(_sf_pair_t));
}


//
// 'sfRemoveString()' - Remove a localization string.
//
// This function removes a localization string from the collection.
//

bool					// O - 'true` on success, `false` on error
sfRemoveString(sf_t       *sf,		// I - Strings
               const char *key)		// I - Key
{
  _sf_pair_t	*pair;			// Matching pair


  _sf_rwlock_wrlock(sf->rwlock);

  if ((pair = _sfFindPair(sf, key)) != NULL)
    _sfRemovePair(sf, pair);

  _sf_rwlock_unlock(sf->rwlock);

  return (pair != NULL);
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


//
// 'sf_sort()' - Sort the strings.
//

static void
sf_sort(sf_t *sf)			// I - Localization strings
{
  qsort(sf->pairs, sf->num_pairs, sizeof(_sf_pair_t), (int (*)(const void *, const void *))sf_compare_pairs);
  sf->need_sort = false;
}

