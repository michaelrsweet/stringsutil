//
// Public header file for StringsUtil.
//
// Copyright © 2022 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef SF_H
#  define SF_H
#  include <stdbool.h>
#  ifdef __cplusplus
extern "C" {
#  endif // __cplusplus


//
// Macros...
//

#  define SFSTR(s) s
#  if _WIN32
#    define _SF_FORMAT(a,b)
#  elif defined(__has_extension) || defined(__GNUC__)
#    define _SF_FORMAT(a,b)	__attribute__ ((__format__(__printf__, a,b)))
#  else
#    define _SF_FORMAT(a,b)
#  endif // _WIN32


//
// Types...
//

typedef struct _sf_s	sf_t;		// Strings file


//
// Functions...
//

extern void		sfDelete(sf_t *sf);
extern const char	*sfFormatString(sf_t *sf, char *buffer, size_t bufsize, const char *key, ...) _SF_FORMAT(4,5);
extern const char	*sfGetError(sf_t *sf);
extern const char	*sfGetString(sf_t *sf, const char *key);
extern bool		sfLoadFromFile(sf_t *sf, const char *filename);
extern bool		sfLoadFromString(sf_t *sf, const char *data);
extern sf_t		*sfNew(void);
extern void		sfPrintf(FILE *fp, const char *message, ...);
extern void		sfPuts(FILE *fp, const char *message);
extern void		sfRegisterDirectory(const char *directory);
extern void		sfRegisterString(const char *locale, const char *data);
extern void		sfSetLocale(void);


#  ifdef __cplusplus
}
#  endif // __cplusplus
#endif // !SF_H
