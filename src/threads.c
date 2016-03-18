/*!\file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * \see lely/libc/threads.h
 *
 * \copyright 2016 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#ifndef LELY_NO_THREADS

#include <lely/libc/threads.h>

#undef LELY_HAVE_SCHED
#if _POSIX_C_SOURCE >= 200112L && defined(_POSIX_PRIORITY_SCHEDULING) \
		&& __has_include(<sched.h>)
#define LELY_HAVE_SCHED	1
#include <sched.h>
#endif

#ifdef LELY_HAVE_PTHREAD

#include <errno.h>

#include <pthread.h>

LELY_LIBC_EXPORT void __cdecl
call_once(once_flag *flag, void (*func)(void))
{
	pthread_once((pthread_once_t *)flag, func);
}

LELY_LIBC_EXPORT int __cdecl
cnd_broadcast(cnd_t *cond)
{
	return __unlikely(pthread_cond_broadcast((pthread_cond_t *)cond))
			? thrd_error : thrd_success;
}

LELY_LIBC_EXPORT void __cdecl
cnd_destroy(cnd_t *cond)
{
	pthread_cond_destroy((pthread_cond_t *)cond);
}

LELY_LIBC_EXPORT int __cdecl
cnd_init(cnd_t *cond)
{
	switch (pthread_cond_init((pthread_cond_t *)cond, NULL)) {
	case 0: return thrd_success;
	case ENOMEM: return thrd_nomem;
	default: return thrd_error;
	}
}

LELY_LIBC_EXPORT int __cdecl
cnd_signal(cnd_t *cond)
{
	return __unlikely(pthread_cond_signal((pthread_cond_t *)cond))
			? thrd_error : thrd_success;
}

LELY_LIBC_EXPORT int __cdecl
cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts)
{
	switch (pthread_cond_timedwait((pthread_cond_t *)cond,
			(pthread_mutex_t*)mtx, ts)) {
	case 0: return thrd_success;
	case ETIMEDOUT: return thrd_timedout;
	default: return thrd_error;
	}
}

LELY_LIBC_EXPORT int __cdecl
cnd_wait(cnd_t *cond, mtx_t *mtx)
{
	return __unlikely(pthread_cond_wait((pthread_cond_t *)cond,
			(pthread_mutex_t*)mtx)) ? thrd_error : thrd_success;
}

LELY_LIBC_EXPORT void __cdecl
mtx_destroy(mtx_t *mtx)
{
	pthread_mutex_destroy((pthread_mutex_t *)mtx);
}

LELY_LIBC_EXPORT int __cdecl
mtx_init(mtx_t *mtx, int type)
{
	pthread_mutexattr_t attr;
	if (__unlikely(pthread_mutexattr_init(&attr)))
		goto error_mutexattr_init;
	if (__unlikely(pthread_mutexattr_settype(&attr, (type & mtx_recursive)
			? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL)))
		goto error_mutexattr_settype;
	if (__unlikely(pthread_mutex_init((pthread_mutex_t *)mtx, &attr)))
		goto error_mutex_init;

	pthread_mutexattr_destroy(&attr);
	return thrd_success;

error_mutex_init:
error_mutexattr_settype:
	pthread_mutexattr_destroy(&attr);
error_mutexattr_init:
	return thrd_error;
}

LELY_LIBC_EXPORT int __cdecl
mtx_lock(mtx_t *mtx)
{
	return __unlikely(pthread_mutex_lock((pthread_mutex_t *)mtx))
			? thrd_error : thrd_success;
}

LELY_LIBC_EXPORT int __cdecl
mtx_timedlock(mtx_t *mtx, const struct timespec *ts)
{
#if defined(_POSIX_TIMEOUTS) && _POSIX_TIMEOUTS >= 200112L
	switch (pthread_mutex_timedlock((pthread_mutex_t *)mtx, ts)) {
	case 0: return thrd_success;
	case ETIMEDOUT: return thrd_timedout;
	default: return thrd_error;
	}
#else
	__unused_var(mtx);
	__unused_var(ts);

	return thrd_error;
#endif
}

LELY_LIBC_EXPORT int __cdecl
mtx_trylock(mtx_t *mtx)
{
	switch (pthread_mutex_trylock((pthread_mutex_t *)mtx)) {
	case 0: return thrd_success;
	case EBUSY: return thrd_busy;
	default: return thrd_error;
	}
}

LELY_LIBC_EXPORT int __cdecl
mtx_unlock(mtx_t *mtx)
{
	return __unlikely(pthread_mutex_unlock((pthread_mutex_t *)mtx))
			? thrd_error : thrd_success;
}

LELY_LIBC_EXPORT int __cdecl
thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
	switch (pthread_create((pthread_t *)thr, NULL,
			(void *(__cdecl *)(void *))func, arg)) {
	case 0: return thrd_success;
	case EAGAIN: return thrd_nomem;
	default: return thrd_error;
	}
}

LELY_LIBC_EXPORT thrd_t __cdecl
thrd_current(void)
{
	return (thrd_t)pthread_self();
}

LELY_LIBC_EXPORT int __cdecl
thrd_detach(thrd_t thr)
{
	return __unlikely(pthread_detach((pthread_t)thr))
			? thrd_error : thrd_success;
}

LELY_LIBC_EXPORT int __cdecl
thrd_equal(thrd_t thr0, thrd_t thr1)
{
	return pthread_equal((pthread_t)thr0, (pthread_t)thr1);
}

_Noreturn LELY_LIBC_EXPORT void __cdecl
thrd_exit(int res)
{
	pthread_exit((void *)(intptr_t)res);
	for (;;);
}

LELY_LIBC_EXPORT int __cdecl
thrd_join(thrd_t thr, int *res)
{
	void *value_ptr = NULL;
	if (__unlikely(pthread_join((pthread_t)thr, &value_ptr)))
		return thrd_error;
	if (res)
		*res = (intptr_t)value_ptr;
	return thrd_success;
}

LELY_LIBC_EXPORT int __cdecl
tss_create(tss_t *key, tss_dtor_t dtor)
{
	return __unlikely(pthread_key_create((pthread_key_t *)key, dtor))
			? thrd_error : thrd_success;
}

LELY_LIBC_EXPORT void __cdecl
tss_delete(tss_t key)
{
	pthread_key_delete((pthread_key_t)key);
}

LELY_LIBC_EXPORT void * __cdecl
tss_get(tss_t key)
{
	return pthread_getspecific((pthread_key_t)key);
}

LELY_LIBC_EXPORT int __cdecl
tss_set(tss_t key, void *val)
{
	return __unlikely(pthread_setspecific((pthread_key_t)key, val))
			? thrd_error : thrd_success;
}

#endif // LELY_HAVE_PTHREAD

LELY_LIBC_EXPORT int __cdecl
thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
	int res = nanosleep(duration, remaining);
	if (__unlikely(res)) {
#ifdef _POSIX_C_SOURCE
		res = errno == EINTR ? -1 : -2;
#else
		res = -2;
#endif
	}
	return res;
}

LELY_LIBC_EXPORT void __cdecl
thrd_yield(void)
{
#ifdef _WIN32
	SwitchToThread();
#elif defined(LELY_HAVE_SCHED)
	sched_yield();
#endif
}

#endif // !LELY_NO_THREADS

