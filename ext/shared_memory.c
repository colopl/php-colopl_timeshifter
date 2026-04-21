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
