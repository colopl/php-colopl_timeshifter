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
# include <semaphore.h>
# include <sys/mman.h>

typedef struct {
  void *data;
  size_t size;
  sem_t semaphore;
} sm_t;

bool sm_init(sm_t *sm, size_t size);
void sm_read(sm_t *sm, void *dest);
bool sm_write(sm_t *sm, void *src);
bool sm_free(sm_t *sm);

#endif  /* SHARED_MEMORY_H */
