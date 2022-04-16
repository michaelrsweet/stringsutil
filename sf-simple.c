//
// Simple localization functions for StringsUtil.
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
// '_sfGetDefault()' - Get the default localization, if any.
//

sf_t *					// O - Default localization or `NULL` if none
_sfGetDefault(void)
{
  return (sf_default);
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
  if (!sfLoadFile(sf_default, filename))
  {
    snprintf(filename, sizeof(filename), "%s/%2s.strings", directory, sf_locale);
    sfLoadFile(sf_default, filename);
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
    sfLoadString(sf_default, data);
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
