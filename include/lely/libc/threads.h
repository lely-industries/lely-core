/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<threads.h>`, if it exists, and defines any missing functionality.
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

#ifndef LELY_LIBC_THREADS_H_
#define LELY_LIBC_THREADS_H_

#include <lely/features.h>

// clang-format off
#ifndef LELY_HAVE_THREADS_H
#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define LELY_HAVE_THREADS_H 1
#endif
#endif
// clang-format on

#if LELY_HAVE_THREADS_H
#include <threads.h>
#else // !LELY_HAVE_UNISTD_H

#include <lely/libc/time.h>

#ifndef LELY_HAVE_PTHREAD_H
#if _POSIX_THREADS >= 200112L || defined(__MINGW32__)
#define LELY_HAVE_PTHREAD_H 1
#endif
#endif

#if LELY_HAVE_PTHREAD_H
#include <pthread.h>
#elif _WIN32
#include <windows.h>
#else
#error This file requires POSIX Threads or Windows.
#endif

#ifndef thread_local
#if __cplusplus >= 201103L
// thread_local is a keyword in C++11 and later.
#else
#define thread_local _Thread_local
#endif
#endif

/// The static initializer for an object of type #once_flag.
#if LELY_HAVE_PTHREAD_H
#define ONCE_FLAG_INIT PTHREAD_ONCE_INIT
#elif _WIN32
#define ONCE_FLAG_INIT 0
#endif

/**
 * The maximum number of times that destructors will be called when a thread
 * terminates.
 */
#define TSS_DTOR_ITERATIONS 1

/// A complete object type that holds an identifier for a condition variable.
#if LELY_HAVE_PTHREAD_H
typedef pthread_cond_t cnd_t;
#elif _WIN32
typedef CONDITION_VARIABLE cnd_t;
#endif

/// A complete object type that holds an identifier for a thread.
#if LELY_HAVE_PTHREAD_H
typedef pthread_t thrd_t;
#elif _WIN32
typedef void *thrd_t;
#endif

/**
 * A complete object type that holds an identifier for a thread-specific storage
 * pointer.
 */
#if LELY_HAVE_PTHREAD_H
typedef pthread_key_t tss_t;
#elif _WIN32
typedef DWORD tss_t;
#endif

/// A complete object type that holds an identifier for a mutex.
#if LELY_HAVE_PTHREAD_H
typedef pthread_mutex_t mtx_t;
#elif _WIN32
typedef CRITICAL_SECTION mtx_t;
#endif

enum {
	/// A mutex type that supports neither timeout nor test and return.
	mtx_plain = 0,
	/// A mutex type that supports recursive locking.
	mtx_recursive = 2,
	/**
	 * A mutex type that supports timeout (not available with the native
	 * Windows API).
	 */
	mtx_timed = 1
};

enum {
	/// Indicates that the requested operation succeeded.
	thrd_success,
	/// Indicates that the requested operation failed.
	thrd_error,
	/**
	 * Indicates that the time specified in the call was reached without
	 * acquiring the requested resource.
	 */
	thrd_timedout,
	/**
	 * Indicates that the requested operation failed because a resource
	 * requested by a test and return function is already in use.
	 */
	thrd_busy,
	/**
	 * Indicates that the requested operation failed because it was unable
	 * to allocate memory.
	 */
	thrd_nomem
};

/// A complete object type that holds a flag for use by call_once().
#if LELY_HAVE_PTHREAD_H
typedef pthread_once_t once_flag;
#elif _WIN32
typedef long once_flag;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The function pointer type used for a destructor for a thread-specific storage
 * pointer.
 */
#if LELY_HAVE_PTHREAD_H
typedef void (*tss_dtor_t)(void *);
#elif _WIN32
// clang-format off
typedef void (WINAPI *tss_dtor_t)(void *);
// clang-format on
#endif

/**
 * The function pointer type that is passed to thrd_create() to create a new
 * thread.
 */
typedef int (*thrd_start_t)(void *);

/**
 * Uses the #once_flag at <b>flag</b> to ensure that <b>func</b> is called
 * exactly once, the first time the call_once() functions is called with that
 * value of <b>flag</b>. Completion of an effective call to the call_once()
 * function synchronizes with all subsequent calls to the call_once() function
 * with the same value of <b>flag</b>.
 */
void call_once(once_flag *flag, void (*func)(void));

/**
 * Unblocks all of the threads that are blocked on the condition variable at
 * <b>cond</b> at the time of the call. If no threads are blocked on the
 * condition variable at <b>cond</b> at the time of the call, the function does
 * nothing.
 *
 * @returns #thrd_success on success, or #thrd_error if the request could not be
 * honored.
 */
int cnd_broadcast(cnd_t *cond);

/**
 * Releases all resources used by the condition variable at <b>cond</b>. This
 * function requires that no threads be blocked waiting for the condition
 * variable at <b>cond</b>.
 */
void cnd_destroy(cnd_t *cond);

/**
 * Creates a condition variable. If it succeeds it sets the variable at
 * <b>cond</b> to a value that uniquely identifies the newly created condition
 * variable. A thread that calls cnd_wait() on a newly created condition
 * variable will block.
 *
 * @returns #thrd_success on success, or #thrd_nomem if no memory could be
 * allocated for the newly created condition, or #thrd_error if the request
 * could not be honored.
 */
int cnd_init(cnd_t *cond);

/**
 * Unblocks one of the threads that are blocked on the condition variable at
 * <b>cond</b> at the time of the call. If no threads are blocked on the
 * condition variable at the time of the call, the function does nothing and
 * return success.
 *
 * @returns #thrd_success on success or #thrd_error if the request could not be
 * honored.
 */
int cnd_signal(cnd_t *cond);

/**
 * Atomically unlocks the mutex at <b>mtx</b> and endeavors to block until the
 * condition variable at <b>cond</b> is signaled by a call to cnd_signal() or to
 * cnd_broadcast(), or until after the TIME_UTC-based calendar time at
 * <b>ts</b>. When the calling thread becomes unblocked it locks the variable at
 * <b>mtx</b> before it returns. This function requires that the mutex at
 * <b>mtx</b> be locked by the calling thread.
 *
 * @returns #thrd_success upon success, or #thrd_timedout if the time specified
 * in the call was reached without acquiring the requested resource, or
 * #thrd_error if the request could not be honored.
 */
int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts);

/**
 * Atomically unlocks the mutex at <b>mtx</b> and endeavors to block until the
 * condition variable at <b>cond</b> is signaled by a call to cnd_signal() or to
 * cnd_broadcast(). When the calling thread becomes unblocked it locks the mutex
 * at <b>mtx</b> before it returns. This function requires that the mutex at
 * <b>mtx</b> be locked by the calling thread.
 *
 * @returns #thrd_success on success or #thrd_error if the request could not be
 * honored.
 */
int cnd_wait(cnd_t *cond, mtx_t *mtx);

/**
 * Releases any resources used by the mutex at <b>mtx</b>. No threads can be
 * blocked waiting for the mutex at <b>mtx</b>.
 */
void mtx_destroy(mtx_t *mtx);

/**
 * Creates a mutex object with properties indicated by <b>type</b>, which must
 * have one of the four values:
 * - `#mtx_plain` for a simple non-recursive mutex,
 * - `#mtx_timed` for a non-recursive mutex that supports timeout,
 * - `#mtx_plain | #mtx_recursive` for a simple recursive mutex, or
 * - `#mtx_timed | #mtx_recursive` for a recursive mutex that supports timeout.
 *
 * If this function succeeds, it sets the mutex at <b>mtx</b> to a value that
 * uniquely identifies the newly created mutex.
 *
 * @returns #thrd_success on success, or #thrd_error if the request could not be
 * honored.
 */
int mtx_init(mtx_t *mtx, int type);

/**
 * Blocks until it locks the mutex at <b>mtx</b>. If the mutex is non-recursive,
 * it SHALL not be locked by the calling thread. Prior calls to mtx_unlock() on
 * the same mutex shall synchronize with this operation.
 *
 * @returns #thrd_success on success, or #thrd_error if the request could not be
 * honored.
 */
int mtx_lock(mtx_t *mtx);

/**
 * Endeavors to block until it locks the mutex at <b>mtx</b> or until after the
 * TIME_UTC-based calendar time at <b>ts</b>. The specified mutex SHALL support
 * timeout. If the operation succeeds, prior calls to mtx_unlock() on the same
 * mutex shall synchronize with this operation.
 *
 * @returns #thrd_success on success, or #thrd_timedout if the time specified
 * was reached without acquiring the requested resource, or #thrd_error if the
 * request could not be honored.
 */
int mtx_timedlock(mtx_t *mtx, const struct timespec *ts);

/**
 * Endeavors to lock the mutex at <b>mtx</b>. If the mutex is already locked,
 * the function returns without blocking. If the operation succeeds, prior calls
 * to mtx_unlock() on the same mutex shall synchronize with this operation.
 *
 * @returns #thrd_success on success, or #thrd_busy if the resource requested is
 * already in use, or #thrd_error if the request could not be honored.
 */
int mtx_trylock(mtx_t *mtx);

/**
 * Unlocks the mutex at <b>mtx</b>. The mutex at <b>mtx</b> SHALL be locked by
 * the calling thread.
 *
 * @returns #thrd_success on success, or #thrd_error if the request could not be
 * honored.
 */
int mtx_unlock(mtx_t *mtx);

/**
 * Creates a new thread executing `func(arg)`. If this function succeeds, it
 * sets the object at <b>thr</b> to the identifier of the newly created thread.
 * (A thread's identifier may be reused for a different thread once the original
 * thread has exited and either been detached or joined to another thread.) The
 * completion of this function synchronizes with the beginning of the execution
 * of the new thread.
 *
 * @returns #thrd_success on success, or #thrd_nomem if no memory could be
 * allocated for the thread requested, or #thrd_error if the request could not
 * be honored.
 */
int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);

/**
 * Identifies the thread that called it.
 *
 * @returns the identifier of the thread that called it.
 */
thrd_t thrd_current(void);

/**
 * Tells the operating system to dispose of any resources allocated to the
 * thread identified by <b>thr</b> when that thread terminates. The thread
 * identified by <b>thr</b> SHALL not have been previously detached or joined
 * with another thread.
 *
 * @returns #thrd_success on success or #thrd_error if the request could not be
 * honored.
 */
int thrd_detach(thrd_t thr);

/**
 * Determines whether the thread identified by <b>thr0</b> refers to the thread
 * identified by <b>thr1</b>.
 *
 * @returns zero if the thread <b>thr0</b> and the thread <b>thr1</b> refer to
 * different threads. Otherwise this function returns a nonzero value.
 */
int thrd_equal(thrd_t thr0, thrd_t thr1);

/**
 * Terminates execution of the calling thread and sets its result code to
 * <b>res</b>.
 *
 * The program shall terminate normally after the last thread has been
 * terminated. The behavior shall be as if the program called the exit()
 * function with the status `EXIT_SUCCESS` at thread termination time.
 */
_Noreturn void thrd_exit(int res);

/**
 * Joins the thread identified by <b>thr</b> with the current thread by blocking
 * until the other thread has terminated. If the parameter <b>res</b> is not a
 * NULL pointer, it stores the thread's result code in the integer at
 * <b>res</b>. The termination of the other thread synchronizes with the
 * completion of this function. The thread identified by <b>thr</b> SHALL not
 * have been previously detached or joined with another thread.
 *
 * @returns #thrd_success on success or #thrd_error if the request could not be
 * honored.
 */
int thrd_join(thrd_t thr, int *res);

/**
 * Suspends execution of the calling thread until either the interval specified
 * by <b>duration</b> has elapsed or a signal which is not being ignored is
 * received. If interrupted by a signal and the <b>remaining</b> argument is not
 * NULL, the amount of time remaining (the requested interval minus the time
 * actually slept) is stored in the interval it points to. The <b>duration</b>
 * and <b>remaining</b> arguments may point to the same object.
 *
 * The suspension time may be longer than requested because the interval is
 * rounded up to an integer multiple of the sleep resolution or because of the
 * scheduling of other activity by the system. But, except for the case of being
 * interrupted by a signal, the suspension time shall not be less than that
 * specified, as measured by the system clock TIME_UTC.
 *
 * @returns zero if the requested time has elapsed, -1 if it has been
 * interrupted by a signal, or a negative value if it fails.
 */
int thrd_sleep(const struct timespec *duration, struct timespec *remaining);

/**
 * Endeavors to permit other threads to run, even if the current thread would
 * ordinarily continue to run.
 */
void thrd_yield(void);

/**
 * Creates a thread-specific storage pointer with destructor <b>dtor</b>, which
 * may be NULL.
 *
 * If this function is successful, it sets the thread-specific storage at
 * <b>key</b> to a value that uniquely identifies the newly created pointer;
 * otherwise, the thread-specific storage at <b>key</b> is set to an undefined
 * value.
 *
 * @returns #thrd_success on success or #thrd_error if the request could not be
 * honored.
 */
int tss_create(tss_t *key, tss_dtor_t dtor);

/**
 * Releases any resources used by the thread-specific storage identified by
 * <b>key</b>.
 */
void tss_delete(tss_t key);

/**
 * Returns the value for the current thread held in the thread-specific storage
 * identified by <b>key</b>.
 *
 * @returns the value for the current thread if successful, or zero if
 * unsuccessful.
 */
void *tss_get(tss_t key);

/**
 * Sets the value for the current thread held in the thread-specific storage
 * identified by <b>key</b> to <b>val</b>.
 *
 * @returns #thrd_success on success or #thrd_error if the request could not be
 * honored.
 */
int tss_set(tss_t key, void *val);

#ifdef __cplusplus
}
#endif

#endif // !LELY_HAVE_THREADS_H

#endif // !LELY_LIBC_THREADS_H_
