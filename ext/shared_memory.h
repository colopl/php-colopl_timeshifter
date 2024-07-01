/*
  +----------------------------------------------------------------------+
  | COLOPL PHP TimeShifter.                                              |
  +----------------------------------------------------------------------+
  | Copyright (c) COLOPL, Inc.                                           |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | info@colopl.co.jp so we can mail you a copy immediately.             |
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
