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

#include "php.h"
#include "shared_memory.h"

bool sm_init(sm_t *sm, size_t size) {
    sm->size = size;

    if ((sm->data = mmap(NULL, sm->size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
        return false;
    } 

    if (sem_init(&sm->semaphore, 1, 1) != 0) {
        return false;
    }

    return sm;
}

void sm_read(sm_t *sm, void *dest) {
    memcpy(dest, sm->data, sm->size);
}

bool sm_write(sm_t *sm, void *src) {
    if (!sm->data) {
        return false;
    }

    if (sem_wait(&sm->semaphore) != 0) {
        return false;
    }

    memcpy(sm->data, src, sm->size);

    if (sem_post(&sm->semaphore) != 0) {
        return false;
    }

    return true;
}

bool sm_free(sm_t *sm) {
    if (!sm->data) {
        return false;
    }

    if (munmap(sm->data, sm->size) != 0) {
        return false;
    }

    if (sem_destroy(&sm->semaphore) != 0) {
        return false;
    }

    return true;
}
