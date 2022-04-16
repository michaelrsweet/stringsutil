//
// Private strings file header file for StringsUtil.
//
// Copyright © 2022 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef SF_PRIVATE_H
#  define SF_PRIVATE_H
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <ctype.h>
#  include <errno.h>
#  include <sys/stat.h>
#  if _WIN32
#    define _CRT_SECURE_NO_DEPRECATE
#    define _CRT_SECURE_NO_WARNINGS
#    include <io.h>
#    include <process.h>
typedef SRWLOCK _sf_rwlock_t;
#    define _sf_rwlock_destroy(rw)
#    define _sf_rwlock_init(rw)		InitializeSRWLock(&rw)
#    define _sf_rwlock_rdlock(rw)	AcquireSRWLockShared(&rw)
#    define _sf_rwlock_wrlock(rw)	AcquireSRWLockExclusive(&rw)
#    define _sf_rwlock_unlock(rw)	(rw == (void *)1 ? ReleaseSRWLockExclusive(&rw) : ReleaseSRWLockShared(&rw))
#  else
#    include <unistd.h>
#    include <fcntl.h>
#    include <pthread.h>
typedef pthread_rwlock_t _sf_rwlock_t;
#    define _sf_rwlock_destroy(rw)	pthread_rwlock_destroy(&rw)
#    define _sf_rwlock_init(rw)		pthread_rwlock_init(&rw, NULL)
#    define _sf_rwlock_rdlock(rw)	pthread_rwlock_rdlock(&rw)
#    define _sf_rwlock_wrlock(rw)	pthread_rwlock_wrlock(&rw)
#    define _sf_rwlock_unlock(rw)	pthread_rwlock_unlock(&rw)
#  endif // _WIN32
#  include "sf.h"
#  ifdef __cplusplus
extern "C" {
#  endif // __cplusplus


//
// Types...
//

typedef struct _sf_pair_s		// String pair
{
  char		*key,			// Key string
		*text,			// Localized text
		*comment;		// Associated comment, if any
} _sf_pair_t;

struct _sf_s				// Strings file
{
  _sf_rwlock_t	rwlock;			// Reader/writer lock
  bool		need_sort;		// Do we need to sort?
  size_t	num_pairs,		// Number of pairs
		alloc_pairs;		// Allocated pairs
  _sf_pair_t	*pairs;			// Array of string pairs
  char		error[256];		// Last error message
};


//
// Functions...
//

extern _sf_pair_t	*_sfAddPair(sf_t *sf, const char *key, const char *text, const char *comment);
extern _sf_pair_t	*_sfFindPair(sf_t *sf, const char *key);
extern sf_t		*_sfGetDefault(void);
extern void		_sfRemovePair(sf_t *sf, _sf_pair_t *pair);
extern void		_sfSetError(sf_t *sf, const char *message, ...) _SF_FORMAT(2,3);


#  ifdef __cplusplus
}
#  endif // __cplusplus
#endif // !SF_PRIVATE_H
