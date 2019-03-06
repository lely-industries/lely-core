/**@file
 * This file is part of the asynchronous I/O library; it contains ...
 *
 * @see lely/aio/timer.h
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

#include "aio.h"
#include <lely/aio/future.h>
#include <lely/util/errnum.h>
#define LELY_AIO_TIMER_INLINE extern inline LELY_DLL_EXPORT
#include <lely/aio/timer.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

#if LELY_AIO_WITH_CLOCK

static int aio_clock_realtime_getres(
		const aio_clock_t *clock, struct timespec *res);
static int aio_clock_realtime_gettime(
		const aio_clock_t *clock, struct timespec *tp);
static int aio_clock_realtime_settime(
		aio_clock_t *clock, const struct timespec *tp);

// clang-format off
static const struct aio_clock_vtbl aio_clock_realtime_vtbl = {
	&aio_clock_realtime_getres,
	&aio_clock_realtime_gettime,
	&aio_clock_realtime_settime
};
// clang-format on

const struct aio_clock_realtime aio_clock_realtime = {
	&aio_clock_realtime_vtbl
};

static int aio_clock_monotonic_getres(
		const aio_clock_t *clock, struct timespec *res);
static int aio_clock_monotonic_gettime(
		const aio_clock_t *clock, struct timespec *tp);
static int aio_clock_monotonic_settime(
		aio_clock_t *clock, const struct timespec *tp);

// clang-format off
static const struct aio_clock_vtbl aio_clock_monotonic_vtbl = {
	&aio_clock_monotonic_getres,
	&aio_clock_monotonic_gettime,
	&aio_clock_monotonic_settime
};
// clang-format on

const struct aio_clock_monotonic aio_clock_monotonic = {
	&aio_clock_monotonic_vtbl
};

#endif // LELY_AIO_WITH_CLOCK

struct aio_timer_wait_data {
	aio_promise_t *promise;
	struct aio_task task;
};

static void aio_timer_wait_func(struct aio_task *task);

aio_future_t *
aio_timer_async_wait(
		aio_timer_t *timer, aio_loop_t *loop, struct aio_task **ptask)
{
	aio_exec_t *exec = aio_timer_get_exec(timer);

	struct aio_timer_wait_data *data = malloc(sizeof(*data));
	if (!data)
		return NULL;
	data->promise = aio_promise_create(loop, exec, &free, data);
	if (!data->promise)
		return NULL;
	data->task = (struct aio_task)AIO_TASK_INIT(exec, &aio_timer_wait_func);

	aio_future_t *future = aio_future_create(data->promise);
	assert(future);

	aio_timer_submit_wait(timer, &data->task);

	if (ptask)
		*ptask = &data->task;

	return future;
}

void
aio_timer_run_wait(aio_timer_t *timer, aio_loop_t *loop)
{
	struct aio_task *task = NULL;
	aio_future_t *future = aio_timer_async_wait(timer, loop, &task);
	if (!future)
		return;

	int errc = 0;

	set_errc(0);
	aio_future_run_wait(future);
	switch (aio_future_get_state(future)) {
	case AIO_FUTURE_WAITING:
		errc = get_errc();
		aio_timer_cancel(timer, task);
		break;
	case AIO_FUTURE_CANCELED: errc = errnum2c(ERRNUM_CANCELED); break;
	case AIO_FUTURE_VALUE:
		task = aio_future_get_value(future);
		assert(task);
		errc = task->errc;
		break;
	case AIO_FUTURE_ERROR: errc = aio_future_get_errc(future); break;
	}
	aio_future_destroy(future);

	set_errc(errc);
}

void
aio_timer_run_wait_until(aio_timer_t *timer, aio_loop_t *loop,
		const struct timespec *abs_time)
{
	struct aio_task *task = NULL;
	aio_future_t *future = aio_timer_async_wait(timer, loop, &task);
	if (!future)
		return;

	int errc = 0;

	set_errc(0);
	aio_future_run_wait_until(future, abs_time);
	switch (aio_future_get_state(future)) {
	case AIO_FUTURE_WAITING:
		errc = get_errc();
		aio_timer_cancel(timer, task);
		break;
	case AIO_FUTURE_CANCELED: errc = errnum2c(ERRNUM_CANCELED); break;
	case AIO_FUTURE_VALUE:
		task = aio_future_get_value(future);
		assert(task);
		errc = task->errc;
		break;
	case AIO_FUTURE_ERROR: errc = aio_future_get_errc(future); break;
	}
	aio_future_destroy(future);

	set_errc(errc);
}

#if LELY_AIO_WITH_CLOCK

static int
aio_clock_realtime_getres(const aio_clock_t *clock, struct timespec *res)
{
	(void)clock;

	return clock_getres(CLOCK_REALTIME, res);
}

static int
aio_clock_realtime_gettime(const aio_clock_t *clock, struct timespec *tp)
{
	(void)clock;

	return clock_gettime(CLOCK_REALTIME, tp);
}

static int
aio_clock_realtime_settime(aio_clock_t *clock, const struct timespec *tp)
{
	(void)clock;

	return clock_settime(CLOCK_REALTIME, tp);
}

static int
aio_clock_monotonic_getres(const aio_clock_t *clock, struct timespec *res)
{
	(void)clock;

	return clock_getres(CLOCK_MONOTONIC, res);
}

static int
aio_clock_monotonic_gettime(const aio_clock_t *clock, struct timespec *tp)
{
	(void)clock;

	return clock_gettime(CLOCK_MONOTONIC, tp);
}

static int
aio_clock_monotonic_settime(aio_clock_t *clock, const struct timespec *tp)
{
	(void)clock;

	return clock_settime(CLOCK_MONOTONIC, tp);
}

#endif // LELY_AIO_WITH_CLOCK

static void
aio_timer_wait_func(struct aio_task *task)
{
	assert(task);
	struct aio_timer_wait_data *data =
			structof(task, struct aio_timer_wait_data, task);

	aio_promise_set_value(data->promise, task);
	aio_promise_destroy(data->promise);
}
