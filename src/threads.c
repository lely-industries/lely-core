/*!\file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * \see lely/libc/threads.h
 *
 * \copyright 2017 Lely Industries N.V.
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
#if !defined(LELY_HAVE_PTHREAD) && !defined(_WIN32)
#define LELY_NO_THREADS	1
#endif
#endif

#ifndef LELY_NO_THREADS

#include <lely/libc/threads.h>

#include <assert.h>
#include <errno.h>

#ifdef LELY_HAVE_PTHREAD

#include <pthread.h>

#elif defined(_WIN32)

#include <stdlib.h>

#include <process.h>

//! An entry in the list of flags currently tested by call_once().
struct once_info {
	//! A pointer to the next entry in the list.
	struct once_info *next;
	//! The address of a flag object passed to call_once().
	once_flag *flag;
	//! The number of threads currently calling call_once() with #flag.
	int cnt;
	//! The mutex protecting *#flag.
	mtx_t mtx;
};

//! The spinlock protecting #once_list.
static volatile LONG once_lock;

/*!
 * A pointer to the first entry in the list of flags currently tested by
 * call_once()
 */
static struct once_info *once_list;

/*!
 * A static instance of #once_info, used to avoid a calling `malloc()` in case
 * call_once() is not invoked concurrently with different flags.
 */
static struct once_info once_fast;

/*!
 * Thread-specific data used to synchronize thrd_create(), thrd_detach(),
 * thrd_exit() and thrd_join().
 */
struct thrd_info {
	//! The function pointer passed to thrd_create().
	thrd_start_t func;
	//! The argument for #func().
	void *arg;
	//! The result of a call to #func().
	int res;
	//! The condition variable signaling a change in #stat.
	cnd_t cond;
	//! The mutex protecting #cond and #stat.
	mtx_t mtx;
	//! The status of the thread.
	enum {
		//! The thread is running.
		THRD_STARTED,
		/*!
		 * The thread has stopped and is waiting to be detached or
		 * joined.
		 */
		THRD_STOPPED,
		//! The thread has been detached.
		THRD_DETACHED
	} stat;
};

/*!
 * A pointer to the #thrd_info instance for the current thread (NULL for the
 * main thread).
 */
static thread_local struct thrd_info *thrd_self;

/*!
 * The function passed to _beginthread(). \a arglist points to an instance of
 * #thrd_info.
 */
static void __cdecl thrd_start(void *arglist);

#endif // LELY_HAVE_PTHREAD

#undef LELY_HAVE_SCHED
#if _POSIX_C_SOURCE >= 200112L && defined(_POSIX_PRIORITY_SCHEDULING) \
		&& __has_include(<sched.h>)
#define LELY_HAVE_SCHED	1
#include <sched.h>
#endif

LELY_LIBC_EXPORT void __cdecl
call_once(once_flag *flag, void (__cdecl *func)(void))
{
#ifdef LELY_HAVE_PTHREAD
	pthread_once((pthread_once_t *)flag, func);
#else
	// Perform a quick (atomic) check to see if the flag is already set.
	if (__likely(InterlockedCompareExchange(flag, 0, 0)))
		return;

	struct once_info *info;
	struct once_info **pinfo;

	// Acquire the spinlock for #once_list.
	while (InterlockedCompareExchange(&once_lock, 1, 0))
		thrd_yield();

	// Find the flag in the list and increment its use count. If not found,
	// create a new entry and initialize the mutex.
	for (pinfo = &once_list; *pinfo && (*pinfo)->flag != flag;
			pinfo = &(*pinfo)->next);
	if (*pinfo) {
		info = *pinfo;
		info->cnt++;
	} else {
		if (once_fast.flag)
			info = malloc(sizeof(*info));
		else
			info = &once_fast;
		// We cannot signal a malloc() error to the user.
		assert(info);
		info->next = NULL;
		info->flag = flag;
		info->cnt = 0;
		mtx_init(&info->mtx, mtx_plain);
		*pinfo = info;
	}

	// Release the spinlock for #once_list.
	InterlockedExchange(&once_lock, 0);

	// Now that we have a mutex for the flag, lock it and run func() once.
	mtx_lock(&info->mtx);
	if (!InterlockedCompareExchange(info->flag, 0, 0)) {
		func();
		InterlockedExchange(info->flag, 1);
	}
	mtx_unlock(&info->mtx);

	while (InterlockedCompareExchange(&once_lock, 1, 0))
		thrd_yield();

	// Find the flag in the list and decrement its use count. If the count
	// is zero, destroy the mutex and delete the entry.
	for (pinfo = &once_list; (*pinfo)->flag != flag;
			pinfo = &(*pinfo)->next);
	assert(*pinfo);
	info = *pinfo;
	if (!info->cnt--) {
		*pinfo = info->next;
		mtx_destroy(&info->mtx);
		if (info == &once_fast)
			info->flag = NULL;
		else
			free(info);
	}

	InterlockedExchange(&once_lock, 0);
#endif // LELY_HAVE_PTHREAD
}

LELY_LIBC_EXPORT int __cdecl
cnd_broadcast(cnd_t *cond)
{
#ifdef LELY_HAVE_PTHREAD
	return __unlikely(pthread_cond_broadcast((pthread_cond_t *)cond))
			? thrd_error : thrd_success;
#elif defined(_WIN32)
	WakeAllConditionVariable(&cond->__cond);

	return thrd_success;
#endif
}

LELY_LIBC_EXPORT void __cdecl
cnd_destroy(cnd_t *cond)
{
#ifdef LELY_HAVE_PTHREAD
	pthread_cond_destroy((pthread_cond_t *)cond);
#elif defined(_WIN32)
	__unused_var(cond);
#endif
}

LELY_LIBC_EXPORT int __cdecl
cnd_init(cnd_t *cond)
{
#ifdef LELY_HAVE_PTHREAD
	switch (pthread_cond_init((pthread_cond_t *)cond, NULL)) {
	case 0: return thrd_success;
	case ENOMEM: return thrd_nomem;
	default: return thrd_error;
	}
#elif defined(_WIN32)
	InitializeConditionVariable(&cond->__cond);

	return thrd_success;
#endif
}

LELY_LIBC_EXPORT int __cdecl
cnd_signal(cnd_t *cond)
{
#ifdef LELY_HAVE_PTHREAD
	return __unlikely(pthread_cond_signal((pthread_cond_t *)cond))
			? thrd_error : thrd_success;
#elif defined(_WIN32)
	WakeConditionVariable(&cond->__cond);

	return thrd_success;
#endif
}

LELY_LIBC_EXPORT int __cdecl
cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts)
{
#ifdef LELY_HAVE_PTHREAD
	switch (pthread_cond_timedwait((pthread_cond_t *)cond,
			(pthread_mutex_t*)mtx, ts)) {
	case 0: return thrd_success;
	case ETIMEDOUT: return thrd_timedout;
	default: return thrd_error;
	}
#elif defined(_WIN32)
	struct timespec now = { 0, 0 };
	timespec_get(&now, TIME_UTC);
	int64_t msec = ((int64_t)now.tv_sec - (int64_t)ts->tv_sec) * 1000
			+ ((int64_t)now.tv_nsec - (int64_t)ts->tv_nsec)
			/ 1000000l;

	return SleepConditionVariableCS(&cond->__cond, &mtx->__mtx,
			(DWORD)(msec > 0 ? msec : 0))
			? thrd_success : GetLastError() == ERROR_TIMEOUT
			? thrd_timedout : thrd_error;
#endif
}

LELY_LIBC_EXPORT int __cdecl
cnd_wait(cnd_t *cond, mtx_t *mtx)
{
#ifdef LELY_HAVE_PTHREAD
	return __unlikely(pthread_cond_wait((pthread_cond_t *)cond,
			(pthread_mutex_t*)mtx)) ? thrd_error : thrd_success;
#elif defined(_WIN32)
	return SleepConditionVariableCS(&cond->__cond, &mtx->__mtx, INFINITE)
			? thrd_success : thrd_error;
#endif
}

LELY_LIBC_EXPORT void __cdecl
mtx_destroy(mtx_t *mtx)
{
#ifdef LELY_HAVE_PTHREAD
	pthread_mutex_destroy((pthread_mutex_t *)mtx);
#elif defined(_WIN32)
	DeleteCriticalSection(&mtx->__mtx);
#endif
}

LELY_LIBC_EXPORT int __cdecl
mtx_init(mtx_t *mtx, int type)
{
#ifdef LELY_HAVE_PTHREAD
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
#elif defined(_WIN32)
	if (__unlikely(type & mtx_timed))
		return thrd_error;

	InitializeCriticalSection(&mtx->__mtx);

	return thrd_success;
#endif
}

LELY_LIBC_EXPORT int __cdecl
mtx_lock(mtx_t *mtx)
{
#ifdef LELY_HAVE_PTHREAD
	return __unlikely(pthread_mutex_lock((pthread_mutex_t *)mtx))
			? thrd_error : thrd_success;
#elif defined(_WIN32)
	EnterCriticalSection(&mtx->__mtx);

	return thrd_success;
#endif
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
#ifdef LELY_HAVE_PTHREAD
	switch (pthread_mutex_trylock((pthread_mutex_t *)mtx)) {
	case 0: return thrd_success;
	case EBUSY: return thrd_busy;
	default: return thrd_error;
	}
#elif defined(_WIN32)
	return TryEnterCriticalSection(&mtx->__mtx) ? thrd_success : thrd_busy;
#endif
}

LELY_LIBC_EXPORT int __cdecl
mtx_unlock(mtx_t *mtx)
{
#ifdef LELY_HAVE_PTHREAD
	return __unlikely(pthread_mutex_unlock((pthread_mutex_t *)mtx))
			? thrd_error : thrd_success;
#elif defined(_WIN32)
	LeaveCriticalSection(&mtx->__mtx);

	return thrd_success;
#endif
}

LELY_LIBC_EXPORT int __cdecl
thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
#ifdef LELY_HAVE_PTHREAD
	switch (pthread_create((pthread_t *)thr, NULL,
			(void *(__cdecl *)(void *))func, arg)) {
	case 0: return thrd_success;
	case EAGAIN: return thrd_nomem;
	default: return thrd_error;
	}
#elif defined(_WIN32)
	struct thrd_info *info = malloc(sizeof(*info));
	if (__unlikely(!info))
		return thrd_nomem;

	info->func = func;
	info->arg = arg;
	info->res = 0;

	cnd_init(&info->cond);
	mtx_init(&info->mtx, mtx_plain);
	info->stat = THRD_STARTED;

	if (__unlikely(_beginthread(&thrd_start, 0, &info) == (uintptr_t)-1)) {
		mtx_destroy(&info->mtx);
		cnd_destroy(&info->cond);
		free(info);
		return thrd_error;
	}

	*thr = (thrd_t)info;

	return thrd_success;
#endif
}

LELY_LIBC_EXPORT thrd_t __cdecl
thrd_current(void)
{
#ifdef LELY_HAVE_PTHREAD
	return (thrd_t)pthread_self();
#elif defined(_WIN32)
	return (thrd_t)thrd_self;
#endif
}

LELY_LIBC_EXPORT int __cdecl
thrd_detach(thrd_t thr)
{
#ifdef LELY_HAVE_PTHREAD
	return __unlikely(pthread_detach((pthread_t)thr))
			? thrd_error : thrd_success;
#elif defined(_WIN32)
	struct thrd_info *info = (struct thrd_info *)thr;

	mtx_lock(&info->mtx);
	if (info->stat == THRD_STOPPED) {
		mtx_unlock(&info->mtx);

		mtx_destroy(&info->mtx);
		cnd_destroy(&info->cond);
		free(info);
	} else {
		assert(info->stat != THRD_DETACHED);
		info->stat = THRD_DETACHED;
		mtx_unlock(&info->mtx);
	}

	return thrd_success;
#endif
}

LELY_LIBC_EXPORT int __cdecl
thrd_equal(thrd_t thr0, thrd_t thr1)
{
#ifdef LELY_HAVE_PTHREAD
	return pthread_equal((pthread_t)thr0, (pthread_t)thr1);
#elif defined(_WIN32)
	return thr0 == thr1;
#endif
}

_Noreturn LELY_LIBC_EXPORT void __cdecl
thrd_exit(int res)
{
#ifdef LELY_HAVE_PTHREAD
	pthread_exit((void *)(intptr_t)res);
#elif defined(_WIN32)
	struct thrd_info *info = (struct thrd_info *)thrd_current();

	info->res = res;

	mtx_lock(&info->mtx);
	if (info->stat == THRD_DETACHED) {
		mtx_unlock(&info->mtx);

		mtx_destroy(&info->mtx);
		cnd_destroy(&info->cond);
		free(info);
	} else {
		assert(info->stat != THRD_STOPPED);
		info->stat = THRD_STOPPED;
		cnd_signal(&info->cond);
		mtx_unlock(&info->mtx);
	}

	_endthread();
#endif
	for (;;);
}

LELY_LIBC_EXPORT int __cdecl
thrd_join(thrd_t thr, int *res)
{
#ifdef LELY_HAVE_PTHREAD
	void *value_ptr = NULL;
	if (__unlikely(pthread_join((pthread_t)thr, &value_ptr)))
		return thrd_error;
	if (res)
		*res = (intptr_t)value_ptr;
#elif defined(_WIN32)
	struct thrd_info *info = (struct thrd_info *)thr;

	mtx_lock(&info->mtx);
	while (info->stat == THRD_STARTED) {
		if (__unlikely(cnd_wait(&info->cond, &info->mtx) == thrd_error))
			break;
	}
	if (__unlikely(info->stat != THRD_STOPPED)) {
		mtx_unlock(&info->mtx);
		return thrd_error;
	}
	mtx_unlock(&info->mtx);

	if (res)
		*res = info->res;

	mtx_destroy(&info->mtx);
	cnd_destroy(&info->cond);
	free(info);

	return thrd_success;
#endif
	return thrd_success;
}

#ifndef LELY_NO_RT
LELY_LIBC_EXPORT int __cdecl
thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
	int errsv = errno;
	int res = nanosleep(duration, remaining);
	if (__unlikely(res)) {
#if defined(_WIN32) || defined(_POSIX_C_SOURCE)
		res = errno == EINTR ? -1 : -2;
#else
		res = -2;
#endif
		errno = errsv;
	}
	return res;
}
#endif

LELY_LIBC_EXPORT void __cdecl
thrd_yield(void)
{
#ifdef _WIN32
	SwitchToThread();
#elif defined(LELY_HAVE_SCHED)
	sched_yield();
#endif
}

LELY_LIBC_EXPORT int __cdecl
tss_create(tss_t *key, tss_dtor_t dtor)
{
#ifdef LELY_HAVE_PTHREAD
	return __unlikely(pthread_key_create((pthread_key_t *)key, dtor))
			? thrd_error : thrd_success;
#elif defined(_WIN32)
	DWORD dwFlsIndex = FlsAlloc(dtor);
	if (__unlikely(dwFlsIndex == FLS_OUT_OF_INDEXES))
		return thrd_error;

	*key = dwFlsIndex;

	return thrd_success;
#endif
}

LELY_LIBC_EXPORT void __cdecl
tss_delete(tss_t key)
{
#ifdef LELY_HAVE_PTHREAD
	pthread_key_delete((pthread_key_t)key);
#elif defined(_WIN32)
	FlsFree(key);
#endif
}

LELY_LIBC_EXPORT void * __cdecl
tss_get(tss_t key)
{
#ifdef LELY_HAVE_PTHREAD
	return pthread_getspecific((pthread_key_t)key);
#elif defined(_WIN32)
	return FlsGetValue(key);
#endif
}

LELY_LIBC_EXPORT int __cdecl
tss_set(tss_t key, void *val)
{
#ifdef LELY_HAVE_PTHREAD
	return __unlikely(pthread_setspecific((pthread_key_t)key, val))
			? thrd_error : thrd_success;
#elif defined(_WIN32)
	return FlsSetValue(key, val) ? thrd_success : thrd_error;
#endif
}

#ifdef LELY_HAVE_PTHREAD
#elif defined(_WIN32)
static void __cdecl
thrd_start(void *arglist)
{
	thrd_self = arglist;

	struct thrd_info *info = (struct thrd_info *)thrd_current();

	thrd_exit(info->func(info->arg));
}
#endif

#endif // !LELY_NO_THREADS

