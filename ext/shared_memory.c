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

#ifndef PHP_WIN32
# include <errno.h>
# include <fcntl.h>
# include <stdio.h>
# include <unistd.h>
#endif
#include <stdint.h>

#include "php.h"
#include "shared_memory.h"

#ifndef PHP_WIN32
# ifdef MAP_ANONYMOUS
#  define COLOPL_TS_MAP_ANONYMOUS MAP_ANONYMOUS
# elif defined(MAP_ANON)
#  define COLOPL_TS_MAP_ANONYMOUS MAP_ANON
# else
#  error "Neither MAP_ANONYMOUS nor MAP_ANON is available"
# endif

static inline sem_t *sm_semaphore(sm_t *sm)
{
# ifdef __APPLE__
	return sm->semaphore;
# else
	return &sm->semaphore;
# endif
}
#endif

static inline bool sm_lock(sm_t *sm)
{
#ifdef PHP_WIN32
	DWORD wait_result;

	if (!sm->mutex) {
		return false;
	}

	wait_result = WaitForSingleObject(sm->mutex, INFINITE);
	return wait_result == WAIT_OBJECT_0 || wait_result == WAIT_ABANDONED;
#else
	sem_t *semaphore = sm_semaphore(sm);

	if (!semaphore) {
		return false;
	}

	while (sem_wait(semaphore) != 0) {
		if (errno != EINTR) {
			return false;
		}
	}
	return true;
#endif
}

static inline bool sm_unlock(sm_t *sm)
{
#ifdef PHP_WIN32
	return sm->mutex && ReleaseMutex(sm->mutex);
#else
	sem_t *semaphore = sm_semaphore(sm);

	return semaphore && sem_post(semaphore) == 0;
#endif
}

#if defined(__APPLE__) && !defined(PHP_WIN32)
static bool sm_init_semaphore(sm_t *sm)
{
	static unsigned long semaphore_counter;
	unsigned int attempts;

	for (attempts = 0; attempts < 16; attempts++) {
		snprintf(
			sm->semaphore_name,
			sizeof(sm->semaphore_name),
			"/cts%05lx%05lx",
			(unsigned long) getpid() & 0xfffff,
			semaphore_counter++ & 0xfffff
		);

		sm->semaphore = sem_open(sm->semaphore_name, O_CREAT | O_EXCL, 0600, 1);
		if (sm->semaphore != SEM_FAILED) {
			if (sem_unlink(sm->semaphore_name) != 0) {
				sem_close(sm->semaphore);
				sm->semaphore = NULL;
				sm->semaphore_name[0] = '\0';
				return false;
			}
			return true;
		}

		if (errno != EEXIST) {
			break;
		}
	}

	sm->semaphore = NULL;
	sm->semaphore_name[0] = '\0';
	return false;
}
#endif

bool sm_init(sm_t *sm, size_t size)
{
	memset(sm, 0, sizeof(sm_t));
	sm->size = size;

#ifdef PHP_WIN32
	sm->mapping = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		(DWORD) (((uint64_t) sm->size) >> 32),
		(DWORD) (((uint64_t) sm->size) & 0xffffffff),
		NULL
	);
	if (!sm->mapping) {
		return false;
	}

	sm->data = MapViewOfFile(sm->mapping, FILE_MAP_ALL_ACCESS, 0, 0, sm->size);
	if (!sm->data) {
		CloseHandle(sm->mapping);
		sm->mapping = NULL;
		return false;
	}

	sm->mutex = CreateMutex(NULL, FALSE, NULL);
	if (!sm->mutex) {
		UnmapViewOfFile(sm->data);
		CloseHandle(sm->mapping);
		sm->data = NULL;
		sm->mapping = NULL;
		return false;
	}
#else
	sm->data = mmap(NULL, sm->size, PROT_READ | PROT_WRITE, MAP_SHARED | COLOPL_TS_MAP_ANONYMOUS, -1, 0);
	if (sm->data == MAP_FAILED) {
		sm->data = NULL;
		return false;
	}

#ifdef __APPLE__
	if (!sm_init_semaphore(sm)) {
		munmap(sm->data, sm->size);
		sm->data = NULL;
		return false;
	}
#else
	if (sem_init(&sm->semaphore, 1, 1) != 0) {
		munmap(sm->data, sm->size);
		sm->data = NULL;
		return false;
	}
#endif
#endif

	return true;
}

void sm_read(sm_t *sm, void *dest)
{
	if (!sm->data) {
		memset(dest, 0, sm->size);
		return;
	}

	if (!sm_lock(sm)) {
		memset(dest, 0, sm->size);
		return;
	}

	memcpy(dest, sm->data, sm->size);

	sm_unlock(sm);
}

bool sm_write(sm_t *sm, void *src)
{
	if (!sm->data) {
		return false;
	}

	if (!sm_lock(sm)) {
		return false;
	}

	memcpy(sm->data, src, sm->size);

	if (!sm_unlock(sm)) {
		return false;
	}

	return true;
}

bool sm_free(sm_t *sm)
{
	bool result = true;

	if (!sm->data) {
		return false;
	}

#ifdef PHP_WIN32
	if (!UnmapViewOfFile(sm->data)) {
		result = false;
	}
	sm->data = NULL;

	if (sm->mapping && !CloseHandle(sm->mapping)) {
		result = false;
	}
	sm->mapping = NULL;

	if (sm->mutex && !CloseHandle(sm->mutex)) {
		result = false;
	}
	sm->mutex = NULL;
#else
	if (munmap(sm->data, sm->size) != 0) {
		result = false;
	}
	sm->data = NULL;

# ifdef __APPLE__
	if (sm->semaphore && sem_close(sm->semaphore) != 0) {
		result = false;
	}
	sm->semaphore = NULL;
	sm->semaphore_name[0] = '\0';
# else
	if (sem_destroy(&sm->semaphore) != 0) {
		result = false;
	}
# endif
#endif

	return result;
}
