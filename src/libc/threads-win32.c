/**@file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * @see lely/libc/threads.h
 *
 * @copyright 2013-2019 Lely Industries N.V.
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

#if !LELY_NO_THREADS && _WIN32 && !LELY_HAVE_PTHREAD_H

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <process.h>

/// An entry in the list of flags currently tested by call_once().
struct once_info {
	/// A pointer to the next entry in the list.
	struct once_info *next;
	/// The address of a flag object passed to call_once().
	once_flag *flag;
	/// The number of threads currently calling call_once() with #flag.
	size_t cnt;
	/// The mutex protecting *#flag.
	mtx_t mtx;
};

/// The spinlock protecting #once_list.
static volatile LONG once_lock;

/**
 * A pointer to the first entry in the list of flags currently tested by
 * call_once()
 */
static struct once_info *once_list;

/**
 * A static instance of #once_info, used to avoid a calling `malloc()` in case
 * call_once() is not invoked concurrently with different flags.
 */
static struct once_info once_fast;

/**
 * Thread-specific data used to synchronize thrd_create(), thrd_detach(),
 * thrd_exit() and thrd_join().
 */
struct thrd_info {
	/// The function pointer passed to thrd_create().
	thrd_start_t func;
	/// The argument for #func().
	void *arg;
	/// The result of a call to #func().
	int res;
	/// The condition variable signaling a change in #stat.
	cnd_t cond;
	/// The mutex protecting #cond and #stat.
	mtx_t mtx;
	/// The status of the thread.
	enum {
		/// The thread is running.
		THRD_STARTED,
		/**
		 * The thread has stopped and is waiting to be detached or
		 * joined.
		 */
		THRD_STOPPED,
		/// The thread has been detached.
		THRD_DETACHED
	} stat;
};

/**
 * A pointer to the #thrd_info instance for the current thread (NULL for the
 * main thread).
 */
static thread_local struct thrd_info *thrd_self;

/**
 * The function passed to _beginthread(). <b>arglist</b> points to an instance
 * of #thrd_info.
 */
static void thrd_start(void *arglist);

void
call_once(once_flag *flag, void (*func)(void))
{
	assert(flag);
	assert(func);

	// Perform a quick (atomic) check to see if the flag is already set.
	if (InterlockedCompareExchange(flag, 0, 0))
		return;

	struct once_info *info;
	struct once_info **pinfo;

	// Acquire the spinlock for once_list.
	while (InterlockedCompareExchange(&once_lock, 1, 0))
		thrd_yield();

	// Find the flag in the list and increment its use count. If not found,
	// create a new entry and initialize the mutex.
	for (pinfo = &once_list; *pinfo && (*pinfo)->flag != flag;
			pinfo = &(*pinfo)->next)
		;
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

	// Release the spinlock for once_list.
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
			pinfo = &(*pinfo)->next)
		;
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
}

int
cnd_broadcast(cnd_t *cond)
{
	WakeAllConditionVariable(cond);

	return thrd_success;
}

void
cnd_destroy(cnd_t *cond)
{
	(void)cond;
}

int
cnd_init(cnd_t *cond)
{
	InitializeConditionVariable(cond);

	return thrd_success;
}

int
cnd_signal(cnd_t *cond)
{
	WakeConditionVariable(cond);

	return thrd_success;
}

int
cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts)
{
	struct timespec now = { 0, 0 };
	timespec_get(&now, TIME_UTC);
	do {
		// Round up to the nearest number of milliseconds, to make sure
		// we don't wake up too early.
		LONGLONG llMilliseconds =
				(LONGLONG)(ts->tv_sec - now.tv_sec) * 1000
				+ (ts->tv_nsec - now.tv_nsec + 999999l)
						/ 1000000l;
		if (llMilliseconds < 0)
			llMilliseconds = 0;
		DWORD dwMilliseconds = llMilliseconds <= MAX_SLEEP_MS
				? (DWORD)llMilliseconds
				: MAX_SLEEP_MS;
		DWORD dwResult = SleepConditionVariableCS(
				cond, mtx, dwMilliseconds);
		if (!dwResult)
			return thrd_success;
		if (GetLastError() != ERROR_TIMEOUT)
			return thrd_error;
		timespec_get(&now, TIME_UTC);
		// clang-format off
	} while (now.tv_sec < ts->tv_sec || (now.tv_sec == ts->tv_sec
			&& now.tv_nsec < ts->tv_nsec));
	// clang-format on
	return thrd_timedout;
}

int
cnd_wait(cnd_t *cond, mtx_t *mtx)
{
	// clang-format off
	return SleepConditionVariableCS(cond, mtx, INFINITE)
			? thrd_success
			: thrd_error;
	// clang-format on
}

void
mtx_destroy(mtx_t *mtx)
{
	DeleteCriticalSection(mtx);
}

int
mtx_init(mtx_t *mtx, int type)
{
	if (type & mtx_timed)
		return thrd_error;

	InitializeCriticalSection(mtx);

	return thrd_success;
}

int
mtx_lock(mtx_t *mtx)
{
	EnterCriticalSection(mtx);

	return thrd_success;
}

int
mtx_timedlock(mtx_t *mtx, const struct timespec *ts)
{
	(void)mtx;
	(void)ts;

	return thrd_error;
}

int
mtx_trylock(mtx_t *mtx)
{
	return TryEnterCriticalSection(mtx) ? thrd_success : thrd_busy;
}

int
mtx_unlock(mtx_t *mtx)
{
	LeaveCriticalSection(mtx);

	return thrd_success;
}

int
thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
	struct thrd_info *info = malloc(sizeof(*info));
	if (!info)
		return thrd_nomem;

	info->func = func;
	info->arg = arg;
	info->res = 0;

	cnd_init(&info->cond);
	mtx_init(&info->mtx, mtx_plain);
	info->stat = THRD_STARTED;

	if (_beginthread(&thrd_start, 0, info) == (uintptr_t)-1) {
		mtx_destroy(&info->mtx);
		cnd_destroy(&info->cond);
		free(info);
		return thrd_error;
	}

	*thr = (thrd_t)info;

	return thrd_success;
}

thrd_t
thrd_current(void)
{
	return (thrd_t)thrd_self;
}

int
thrd_detach(thrd_t thr)
{
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
}

int
thrd_equal(thrd_t thr0, thrd_t thr1)
{
	return thr0 == thr1;
}

_Noreturn void
thrd_exit(int res)
{
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
	for (;;)
		;
}

int
thrd_join(thrd_t thr, int *res)
{
	struct thrd_info *info = (struct thrd_info *)thr;

	mtx_lock(&info->mtx);
	while (info->stat == THRD_STARTED) {
		if (cnd_wait(&info->cond, &info->mtx) == thrd_error)
			break;
	}
	if (info->stat != THRD_STOPPED) {
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

void
thrd_yield(void)
{
	SwitchToThread();
}

int
tss_create(tss_t *key, tss_dtor_t dtor)
{
	DWORD dwFlsIndex = FlsAlloc(dtor);
	if (dwFlsIndex == FLS_OUT_OF_INDEXES)
		return thrd_error;

	*key = dwFlsIndex;

	return thrd_success;
}

void
tss_delete(tss_t key)
{
	FlsFree(key);
}

void *
tss_get(tss_t key)
{
	return FlsGetValue(key);
}

int
tss_set(tss_t key, void *val)
{
	return FlsSetValue(key, val) ? thrd_success : thrd_error;
}

static void
thrd_start(void *arglist)
{
	thrd_self = arglist;

	struct thrd_info *info = (struct thrd_info *)thrd_current();

	thrd_exit(info->func(info->arg));
}

#endif // !LELY_NO_THREADS && _WIN32 && !LELY_HAVE_PTHREAD_H
