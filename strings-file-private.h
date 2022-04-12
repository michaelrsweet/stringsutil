//
// Private strings file header file for StringsUtil.
//
// Copyright © 2022 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef STRINGS_FILE_PRIVATE_H
#  define STRINGS_FILE_PRIVATE_H
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <ctype.h>
#  include <errno.h>
#  include "strings-file.h"
#  ifdef __cplusplus
extern "C" {
#  endif // __cplusplus


//
// Macros...
//

#  define SFSTR(s) s


//
// Types...
//

struct _strings_file_s			// Strings file
{
  cups_array_t	*pairs;			// Array of string pairs
  char		error[256];		// Last error message
};

typedef struct _sf_pair_s		// String pair
{
  char		*key,			// Key string
		*text,			// Localized text
		*comment;		// Associated comment, if any
} _sf_pair_t;


//
// Functions...
//

extern int		_sfPairCompare(_sf_pair_t *a, _sf_pair_t *b);
extern _sf_pair_t	*_sfPairCopy(_sf_pair_t *pair);
extern void		_sfPairFree(_sf_pair_t *pair);
extern void		_sfSetError(strings_file_t *sf, const char *message, ...) _SF_FORMAT(2,3);


#  ifdef __cplusplus
}
#  endif // __cplusplus
#endif // !STRINGS_FILE_PRIVATE_H
