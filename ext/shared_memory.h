/*
  +----------------------------------------------------------------------+
  | COLOPL PHP TimeShifter.                                              |
  +----------------------------------------------------------------------+
  | Copyright (c) COLOPL, Inc.                                           |
  +----------------------------------------------------------------------+
  | This source file is subject to the BSD-3-Clause license that is      |
  | bundled with this package in the file LICENSE.                       |
  +----------------------------------------------------------------------+
  | Author: Go Kudo <g-kudo@colopl.co.jp>                                |
  +----------------------------------------------------------------------+
*/
#ifndef SHARED_MEMORY_H
# define SHARED_MEMORY_H

# include "php.h"

# ifdef PHP_WIN32
#  include <windows.h>
# else
#  include <semaphore.h>
#  include <sys/mman.h>
#  ifdef __APPLE__
#   define COLOPL_TS_SEMAPHORE_NAME_MAX 32
#  endif
# endif

typedef struct {
	void *data;
	size_t size;
# ifdef PHP_WIN32
	HANDLE mapping;
	HANDLE mutex;
# elif defined(__APPLE__)
	sem_t *semaphore;
	char semaphore_name[COLOPL_TS_SEMAPHORE_NAME_MAX];
# else
	sem_t semaphore;
# endif
} sm_t;

bool sm_init(sm_t *sm, size_t size);
void sm_read(sm_t *sm, void *dest);
bool sm_write(sm_t *sm, void *src);
bool sm_free(sm_t *sm);

#endif  /* SHARED_MEMORY_H */
