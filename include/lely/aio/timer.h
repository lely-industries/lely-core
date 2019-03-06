/**@file
 * This header file is part of the asynchronous I/O library; it contains ...
 *
 * @copyright 2018-2019 Lely Industries N.V.
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

#ifndef LELY_AIO_TIMER_H_
#define LELY_AIO_TIMER_H_

#include <lely/aio/exec.h>
#include <lely/libc/time.h>

#ifndef LELY_AIO_TIMER_INLINE
#define LELY_AIO_TIMER_INLINE inline
#endif

struct aio_clock_vtbl;
typedef const struct aio_clock_vtbl *const aio_clock_t;

struct aio_timer_vtbl;
typedef const struct aio_timer_vtbl *const aio_timer_t;

#ifdef __cplusplus
extern "C" {
#endif

struct aio_clock_vtbl {
	int (*getres)(const aio_clock_t *clock, struct timespec *res);
	int (*gettime)(const aio_clock_t *clock, struct timespec *tp);
	int (*settime)(aio_clock_t *clock, const struct timespec *tp);
};

LELY_AIO_TIMER_INLINE int aio_clock_getres(
		const aio_clock_t *clock, struct timespec *res);
LELY_AIO_TIMER_INLINE int aio_clock_gettime(
		const aio_clock_t *clock, struct timespec *tp);
LELY_AIO_TIMER_INLINE int aio_clock_settime(
		aio_clock_t *clock, const struct timespec *tp);

extern const struct aio_clock_realtime {
	const struct aio_clock_vtbl *vptr;
} aio_clock_realtime;

#define AIO_CLOCK_REALTIME (&aio_clock_realtime.vptr)

extern const struct aio_clock_monotonic {
	const struct aio_clock_vtbl *vptr;
} aio_clock_monotonic;

#define AIO_CLOCK_MONOTONIC (&aio_clock_monotonic.vptr)

struct aio_timer_vtbl {
	aio_clock_t *(*get_clock)(const aio_timer_t *timer);
	int (*getoverrun)(const aio_timer_t *timer);
	int (*gettime)(const aio_timer_t *timer, struct itimerspec *value);
	int (*settime)(aio_timer_t *timer, int flags,
			const struct itimerspec *value,
			struct itimerspec *ovalue);
	aio_exec_t *(*get_exec)(const aio_timer_t *timer);
	void (*submit_wait)(aio_timer_t *timer, struct aio_task *task);
	size_t (*cancel)(aio_timer_t *timer, struct aio_task *task);
};

LELY_AIO_TIMER_INLINE aio_clock_t *aio_timer_get_clock(
		const aio_timer_t *timer);

LELY_AIO_TIMER_INLINE int aio_timer_getoverrun(const aio_timer_t *timer);
LELY_AIO_TIMER_INLINE int aio_timer_gettime(
		const aio_timer_t *timer, struct itimerspec *value);
LELY_AIO_TIMER_INLINE int aio_timer_settime(aio_timer_t *timer, int flags,
		const struct itimerspec *value, struct itimerspec *ovalue);

LELY_AIO_TIMER_INLINE aio_exec_t *aio_timer_get_exec(const aio_timer_t *timer);

LELY_AIO_TIMER_INLINE void aio_timer_submit_wait(
		aio_timer_t *timer, struct aio_task *task);
LELY_AIO_TIMER_INLINE size_t aio_timer_cancel(
		aio_timer_t *timer, struct aio_task *task);

aio_future_t *aio_timer_async_wait(
		aio_timer_t *timer, aio_loop_t *loop, struct aio_task **ptask);

void aio_timer_run_wait(aio_timer_t *timer, aio_loop_t *loop);
void aio_timer_run_wait_until(aio_timer_t *timer, aio_loop_t *loop,
		const struct timespec *abs_time);

void *aio_timer_alloc(void);
void aio_timer_free(void *ptr);
aio_timer_t *aio_timer_init(aio_timer_t *timer, clockid_t clockid,
		aio_exec_t *exec, aio_reactor_t *reactor);
void aio_timer_fini(aio_timer_t *timer);

aio_timer_t *aio_timer_create(
		clockid_t clockid, aio_exec_t *exec, aio_reactor_t *reactor);
void aio_timer_destroy(aio_timer_t *timer);

LELY_AIO_TIMER_INLINE int
aio_clock_getres(const aio_clock_t *clock, struct timespec *res)
{
	return (*clock)->getres(clock, res);
}

LELY_AIO_TIMER_INLINE int
aio_clock_gettime(const aio_clock_t *clock, struct timespec *tp)
{
	return (*clock)->gettime(clock, tp);
}

LELY_AIO_TIMER_INLINE int
aio_clock_settime(aio_clock_t *clock, const struct timespec *tp)
{
	return (*clock)->settime(clock, tp);
}

LELY_AIO_TIMER_INLINE aio_clock_t *
aio_timer_get_clock(const aio_timer_t *timer)
{
	return (*timer)->get_clock(timer);
}

LELY_AIO_TIMER_INLINE int
aio_timer_getoverrun(const aio_timer_t *timer)
{
	return (*timer)->getoverrun(timer);
}

LELY_AIO_TIMER_INLINE int
aio_timer_gettime(const aio_timer_t *timer, struct itimerspec *value)
{
	return (*timer)->gettime(timer, value);
}

LELY_AIO_TIMER_INLINE int
aio_timer_settime(aio_timer_t *timer, int flags, const struct itimerspec *value,
		struct itimerspec *ovalue)
{
	return (*timer)->settime(timer, flags, value, ovalue);
}

LELY_AIO_TIMER_INLINE aio_exec_t *
aio_timer_get_exec(const aio_timer_t *timer)
{
	return (*timer)->get_exec(timer);
}

LELY_AIO_TIMER_INLINE void
aio_timer_submit_wait(aio_timer_t *timer, struct aio_task *task)
{
	(*timer)->submit_wait(timer, task);
}

LELY_AIO_TIMER_INLINE size_t
aio_timer_cancel(aio_timer_t *timer, struct aio_task *task)
{
	return (*timer)->cancel(timer, task);
}

#ifdef __cplusplus
}
#endif

#endif // LELY_AIO_TIMER_H_
