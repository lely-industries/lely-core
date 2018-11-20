/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the external clock and timer functions.
 *
 * @see lely/util/xtime.h
 *
 * @copyright 2018 Lely Industries N.V.
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

#include "util.h"
#ifndef LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/util/errnum.h>
#include <lely/util/pheap.h>
#include <lely/util/time.h>
#include <lely/util/xtime.h>

#include <assert.h>
#include <stdlib.h>

/// An external clock.
struct __xclock {
#ifndef LELY_NO_THREADS
	/// The mutex protecting #cond, #now, #res and #timers.
	mtx_t mtx;
	/// The condition variable used to wake xclock_nanosleep().
	cnd_t cond;
#endif
	/// The current time.
	struct timespec now;
	/**
	 * The clock resolution (i.e., the interval between the previous two
	 * updates).
	 */
	struct timespec res;
	/// The heap containing the timers.
	struct pheap timers;
};

/// An external timer.
struct __xtimer {
	/// The node in the heap of timers.
	struct pnode node;
	/// A pointer to the external clock.
	xclock_t *clock;
	/// The notification type (`SIGEV_NONE` or `SIGEV_THREAD`).
	int notify;
	/// The signal value.
	union sigval value;
	/// The notification function.
	void(__cdecl *notify_function)(union sigval);
	/// The absolute expiration time.
	struct timespec expire;
	/// The period.
	struct timespec period;
	/// A flag indicating whether the timer is armed.
	int armed;
	/// The overrun counter.
	int overrun;
};

LELY_UTIL_EXPORT void *
__xclock_alloc(void)
{
	void *ptr = malloc(sizeof(struct __xclock));
	if (__unlikely(!ptr))
		set_errno(errno);
	return ptr;
}

LELY_UTIL_EXPORT void
__xclock_free(void *ptr)
{
	free(ptr);
}

LELY_UTIL_EXPORT struct __xclock *
__xclock_init(struct __xclock *clock)
{
	assert(clock);

#ifndef LELY_NO_THREADS
	mtx_init(&clock->mtx, mtx_plain);
	cnd_init(&clock->cond);
#endif

	clock->now = (struct timespec){ 0, 0 };
	clock->res = (struct timespec){ 0, 0 };

	pheap_init(&clock->timers, &timespec_cmp);

	return clock;
}

LELY_UTIL_EXPORT void
__xclock_fini(struct __xclock *clock)
{
	assert(clock);

#ifndef LELY_NO_THREADS
	cnd_destroy(&clock->cond);
	mtx_destroy(&clock->mtx);
#endif
}

LELY_UTIL_EXPORT xclock_t *
xclock_create(void)
{
	errc_t errc = 0;

	xclock_t *clock = __xclock_alloc();
	if (__unlikely(!clock)) {
		errc = get_errc();
		goto error_alloc_clock;
	}

	if (__unlikely(!__xclock_init(clock))) {
		errc = get_errc();
		goto error_init_clock;
	}

	return clock;

error_init_clock:
	__xclock_free(clock);
error_alloc_clock:
	set_errc(errc);
	return NULL;
}

LELY_UTIL_EXPORT void
xclock_destroy(xclock_t *clock)
{
	if (clock) {
		__xclock_fini(clock);
		__xclock_free(clock);
	}
}

LELY_UTIL_EXPORT int
xclock_getres(xclock_t *clock, struct timespec *res)
{
	assert(clock);

#ifndef LELY_NO_THREADS
	mtx_lock(&clock->mtx);
#endif
	if (res)
		*res = clock->res;
#ifndef LELY_NO_THREADS
	mtx_unlock(&clock->mtx);
#endif
	return 0;
}

LELY_UTIL_EXPORT int
xclock_gettime(xclock_t *clock, struct timespec *tp)
{
	assert(clock);

#ifndef LELY_NO_THREADS
	mtx_lock(&clock->mtx);
#endif
	if (tp)
		*tp = clock->now;
#ifndef LELY_NO_THREADS
	mtx_unlock(&clock->mtx);
#endif
	return 0;
}

#ifndef LELY_NO_THREADS
LELY_UTIL_EXPORT int
xclock_nanosleep(xclock_t *clock, int flags, const struct timespec *rqtp,
		struct timespec *rmtp)
{
	assert(clock);
	assert(rqtp);

	if (__unlikely(rqtp->tv_nsec < 0 || rqtp->tv_nsec >= 1000000000l)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	mtx_lock(&clock->mtx);

	struct timespec tp = *rqtp;
	if (!(flags & TIMER_ABSTIME)) {
		if (tp.tv_sec < 0 || (!tp.tv_sec && !tp.tv_nsec)) {
			mtx_unlock(&clock->mtx);
			return 0;
		}
		timespec_add(&tp, &clock->now);
	}

	while (timespec_cmp(&tp, &clock->now) < 0) {
		// clang-format off
		if (__unlikely(cnd_wait(&clock->cond, &clock->mtx)
				!= thrd_success)) {
			// clang-format on
			if (!(flags & TIMER_ABSTIME) && rmtp) {
				*rmtp = tp;
				timespec_sub(rmtp, &clock->now);
			}
			mtx_unlock(&clock->mtx);
			set_errnum(ERRNUM_INTR);
			return -1;
		}
	}

	mtx_unlock(&clock->mtx);

	return 0;
}
#endif // !LELY_NO_THREADS

LELY_UTIL_EXPORT int
xclock_settime(xclock_t *clock, const struct timespec *tp)
{
	assert(clock);
	assert(tp);

#ifndef LELY_NO_THREADS
	mtx_lock(&clock->mtx);
#endif
	clock->res = *tp;
	timespec_sub(&clock->res, &clock->now);
	clock->now = *tp;

	struct pnode *node;
	while ((node = pheap_first(&clock->timers))) {
		xtimer_t *timer = structof(node, xtimer_t, node);
		if (timespec_cmp(&clock->now, &timer->expire) < 0)
			break;

		// Remove the timer before update the expiration time.
		pheap_remove(&clock->timers, &timer->node);

		if (timer->period.tv_sec || timer->period.tv_nsec) {
			// Compute the timer overrun counter and next expiration
			// time.
			uint64_t expire = timespec_diff_nsec(
					&clock->now, &timer->expire);
			uint64_t period = (uint64_t)timer->period.tv_sec
							* 1000000000l
					+ timer->period.tv_nsec;
			uint64_t overrun = expire / period;
			timespec_add_nsec(
					&timer->expire, (overrun + 1) * period);
			timer->overrun = MIN(overrun, INT_MAX);

			pheap_insert(&clock->timers, &timer->node);
		} else {
			// Disarm a non-periodic timer.
			timer->expire = (struct timespec){ 0, 0 };
			timer->armed = 0;
		}

		if (timer->notify == SIGEV_THREAD) {
			union sigval value = timer->value;
			void(__cdecl * notify_function)(union sigval) =
					timer->notify_function;
#ifndef LELY_NO_THREADS
			mtx_unlock(&clock->mtx);
#endif
			// Invoke the notification function without holding the
			// lock. This allows the function to call clock and
			// timer functions.
			if (notify_function)
				notify_function(value);
#ifndef LELY_NO_THREADS
			mtx_lock(&clock->mtx);
#endif
		}
	}

#ifndef LELY_NO_THREADS
	// Wake all threads waiting on xclock_nanosleep().
	cnd_broadcast(&clock->cond);
	mtx_unlock(&clock->mtx);
#endif

	return 0;
}

LELY_UTIL_EXPORT void *
__xtimer_alloc(void)
{
	void *ptr = malloc(sizeof(struct __xtimer));
	if (__unlikely(!ptr))
		set_errno(errno);
	return ptr;
}

LELY_UTIL_EXPORT void
__xtimer_free(void *ptr)
{
	free(ptr);
}

LELY_UTIL_EXPORT struct __xtimer *
__xtimer_init(struct __xtimer *timer, xclock_t *clock,
		const struct sigevent *evp)
{
	assert(timer);
	assert(clock);

	// clang-format off
	if (!evp || (evp->sigev_notify != SIGEV_NONE
			&& evp->sigev_notify != SIGEV_THREAD)) {
		// clang-format on
		set_errnum(ERRNUM_INVAL);
		return NULL;
	}

	pnode_init(&timer->node, &timer->expire);

	timer->clock = clock;

	timer->notify = evp->sigev_notify;
	timer->value = evp->sigev_value;
	timer->notify_function = evp->sigev_notify_function;

	timer->expire = (struct timespec){ 0, 0 };
	timer->period = (struct timespec){ 0, 0 };
	timer->armed = 0;
	timer->overrun = 0;

	return timer;
}

LELY_UTIL_EXPORT void
__xtimer_fini(struct __xtimer *timer)
{
	assert(timer);

	xtimer_settime(timer, 0, &(struct itimerspec){ { 0, 0 }, { 0, 0 } },
			NULL);
}

LELY_UTIL_EXPORT xtimer_t *
xtimer_create(xclock_t *clock, const struct sigevent *evp)
{
	errc_t errc = 0;

	xtimer_t *timer = __xtimer_alloc();
	if (__unlikely(!timer)) {
		errc = get_errc();
		goto error_alloc_timer;
	}

	if (__unlikely(!__xtimer_init(timer, clock, evp))) {
		errc = get_errc();
		goto error_init_timer;
	}

	return timer;

error_init_timer:
	__xtimer_free(timer);
error_alloc_timer:
	set_errc(errc);
	return NULL;
}

LELY_UTIL_EXPORT void
xtimer_destroy(xtimer_t *timer)
{
	if (timer) {
		__xtimer_fini(timer);
		__xtimer_free(timer);
	}
}

LELY_UTIL_EXPORT int
xtimer_getoverrun(xtimer_t *timer)
{
	assert(timer);
	assert(timer->clock);

#ifndef LELY_NO_THREADS
	mtx_lock(&timer->clock->mtx);
#endif
	int overrun = timer->overrun;
#ifndef LELY_NO_THREADS
	mtx_unlock(&timer->clock->mtx);
#endif
	return overrun;
}

LELY_UTIL_EXPORT int
xtimer_gettime(xtimer_t *timer, struct itimerspec *value)
{
	assert(timer);
	xclock_t *clock = timer->clock;
	assert(clock);

	if (value) {
#ifndef LELY_NO_THREADS
		mtx_lock(&clock->mtx);
#endif
		struct timespec expire = timer->expire;
		if (expire.tv_sec || expire.tv_nsec)
			timespec_sub(&expire, &clock->now);
		struct timespec period = timer->period;
#ifndef LELY_NO_THREADS
		mtx_unlock(&clock->mtx);
#endif
		value->it_interval = period;
		value->it_value = expire;
	}

	return 0;
}

LELY_UTIL_EXPORT int
xtimer_settime(xtimer_t *timer, int flags, const struct itimerspec *value,
		struct itimerspec *ovalue)
{
	assert(timer);
	xclock_t *clock = timer->clock;
	assert(clock);
	assert(value);

	struct timespec period = value->it_interval;
	struct timespec expire = value->it_value;

	int arm = expire.tv_sec != 0 || expire.tv_nsec != 0;

	// clang-format off
	if (__unlikely(arm && (period.tv_nsec < 0
			|| period.tv_nsec >= 1000000000l))) {
		// clang-format on
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	if (!arm || period.tv_sec < 0)
		period = (struct timespec){ 0, 0 };

#ifndef LELY_NO_THREADS
	mtx_lock(&clock->mtx);
#endif

	if (arm && !(flags & TIMER_ABSTIME))
		timespec_add(&expire, &clock->now);

	if (timer->armed)
		pheap_remove(&clock->timers, &timer->node);

	if (ovalue) {
		if (timer->armed) {
			ovalue->it_interval = timer->period;
			ovalue->it_value = timer->expire;
			timespec_sub(&ovalue->it_value, &clock->now);
		} else {
			*ovalue = (struct itimerspec){ { 0, 0 }, { 0, 0 } };
		}
	}
	timer->expire = expire;
	timer->period = period;
	timer->armed = arm;
	timer->overrun = 0;

	if (timer->armed)
		pheap_insert(&clock->timers, &timer->node);

#ifndef LELY_NO_THREADS
	mtx_unlock(&clock->mtx);
#endif

	return 0;
}
