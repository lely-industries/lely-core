/**@file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * @see lely/libc/threads.h
 *
 * @copyright 2013-2020 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "libc.h"
#include <lely/libc/threads.h>

#if !LELY_NO_THREADS && LELY_HAVE_PTHREAD_H

#if LELY_NO_ERRNO
#error This file requires errno.
#endif

#include <errno.h>
#include <stdint.h>

#if _WIN32
#include <processthreadsapi.h>
#endif

#undef LELY_HAVE_SCHED
#if _POSIX_C_SOURCE >= 200112L && defined(_POSIX_PRIORITY_SCHEDULING)
#define LELY_HAVE_SCHED 1
#include <sched.h>
#endif

void
call_once(once_flag *flag, void (*func)(void))
{
	pthread_once(flag, func);
}

int
cnd_broadcast(cnd_t *cond)
{
	int errsv = pthread_cond_broadcast(cond);
	if (errsv) {
		errno = errsv;
		return thrd_error;
	}
	return thrd_success;
}

void
cnd_destroy(cnd_t *cond)
{
	pthread_cond_destroy(cond);
}

int
cnd_init(cnd_t *cond)
{
	int errsv = pthread_cond_init(cond, NULL);
	if (errsv) {
		errno = errsv;
		return errsv == ENOMEM ? thrd_nomem : thrd_error;
	}
	return thrd_success;
}

int
cnd_signal(cnd_t *cond)
{
	int errsv = pthread_cond_signal(cond);
	if (errsv) {
		errno = errsv;
		return thrd_error;
	}
	return thrd_success;
}

int
cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts)
{
	int errsv = pthread_cond_timedwait(cond, mtx, ts);
	if (errsv) {
		errno = errsv;
		return errsv == ETIMEDOUT ? thrd_timedout : thrd_error;
	}
	return thrd_success;
}

int
cnd_wait(cnd_t *cond, mtx_t *mtx)
{
	int errsv = pthread_cond_wait(cond, mtx);
	if (errsv) {
		errno = errsv;
		return thrd_error;
	}
	return thrd_success;
}

void
mtx_destroy(mtx_t *mtx)
{
	pthread_mutex_destroy(mtx);
}

int
mtx_init(mtx_t *mtx, int type)
{
	int errsv;
	pthread_mutexattr_t attr;

	errsv = pthread_mutexattr_init(&attr);
	if (errsv)
		goto error_mutexattr_init;
	// clang-format off
	errsv = pthread_mutexattr_settype(&attr, (type & mtx_recursive)
			? PTHREAD_MUTEX_RECURSIVE
			: PTHREAD_MUTEX_NORMAL);
	// clang-format on
	if (errsv)
		goto error_mutexattr_settype;
	errsv = pthread_mutex_init(mtx, &attr);
	if (errsv)
		goto error_mutex_init;
	pthread_mutexattr_destroy(&attr);

	return thrd_success;

error_mutex_init:
error_mutexattr_settype:
	pthread_mutexattr_destroy(&attr);
error_mutexattr_init:
	errno = errsv;
	return thrd_error;
}

int
mtx_lock(mtx_t *mtx)
{
	int errsv = pthread_mutex_lock(mtx);
	if (errsv) {
		errno = errsv;
		return thrd_error;
	}
	return thrd_success;
}

int
mtx_timedlock(mtx_t *mtx, const struct timespec *ts)
{
	int errsv = pthread_mutex_timedlock(mtx, ts);
	if (errsv) {
		errno = errsv;
		return errsv == ETIMEDOUT ? thrd_timedout : thrd_error;
	}
	return thrd_success;
}

int
mtx_trylock(mtx_t *mtx)
{
	int errsv = pthread_mutex_trylock(mtx);
	if (errsv) {
		errno = errsv;
		return errsv == EBUSY ? thrd_busy : thrd_error;
	}
	return thrd_success;
}

int
mtx_unlock(mtx_t *mtx)
{
	int errsv = pthread_mutex_unlock(mtx);
	if (errsv) {
		errno = errsv;
		return thrd_error;
	}
	return thrd_success;
}

int
thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
#if __GNUC__ >= 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
	int errsv = pthread_create(thr, NULL, (void *(*)(void *))func, arg);
#if __GNUC__ >= 8
#pragma GCC diagnostic pop
#endif
	if (errsv) {
		errno = errsv;
		return errsv == EAGAIN ? thrd_nomem : thrd_error;
	}
	return thrd_success;
}

thrd_t
thrd_current(void)
{
	return pthread_self();
}

int
thrd_detach(thrd_t thr)
{
	int errsv = pthread_detach(thr);
	if (errsv) {
		errno = errsv;
		return thrd_error;
	}
	return thrd_success;
}

int
thrd_equal(thrd_t thr0, thrd_t thr1)
{
	return pthread_equal(thr0, thr1);
}

_Noreturn void
thrd_exit(int res)
{
	pthread_exit((void *)(intptr_t)res);
	// cppcheck-suppress unreachableCode
	for (;;)
		;
}

int
thrd_join(thrd_t thr, int *res)
{
	void *value_ptr = NULL;
	int errsv = pthread_join((pthread_t)thr, &value_ptr);
	if (errsv) {
		errno = errsv;
		return thrd_error;
	}
	if (res)
		*res = (intptr_t)value_ptr;
	return thrd_success;
}

int
thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
	int errsv = errno;
	int res = nanosleep(duration, remaining);
	if (res) {
		res = errno == EINTR ? -1 : -2;
		errno = errsv;
	}
	return res;
}

#if _WIN32
void
thrd_yield(void)
{
	SwitchToThread();
}
#elif LELY_HAVE_SCHED
void
thrd_yield(void)
{
	sched_yield();
}
#endif

int
tss_create(tss_t *key, tss_dtor_t dtor)
{
	int errsv = pthread_key_create(key, dtor);
	if (errsv) {
		errno = errsv;
		return thrd_error;
	}
	return thrd_success;
}

void
tss_delete(tss_t key)
{
	pthread_key_delete(key);
}

void *
tss_get(tss_t key)
{
	return pthread_getspecific(key);
}

int
tss_set(tss_t key, void *val)
{
	int errsv = pthread_setspecific(key, val);
	if (errsv) {
		errno = errsv;
		return thrd_error;
	}
	return thrd_success;
}

#endif // !LELY_NO_THREADS && LELY_HAVE_PTHREAD_H
