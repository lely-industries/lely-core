/**@file
 * This file is part of the I/O library; it exposes the abstract timer
 * functions.
 *
 * @see lely/io2/timer.h
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

#include "io2.h"
#define LELY_IO_TIMER_INLINE extern inline
#include <lely/io2/timer.h>
#include <lely/util/util.h>

#include <assert.h>

struct io_timer_async_wait {
	ev_promise_t *promise;
	struct io_timer_wait wait;
};

static void io_timer_async_wait_func(struct ev_task *task);

ev_future_t *
io_timer_async_wait(io_timer_t *timer, ev_exec_t *exec,
		struct io_timer_wait **pwait)
{
	ev_promise_t *promise = ev_promise_create(
			sizeof(struct io_timer_async_wait), NULL);
	if (!promise)
		return NULL;
	ev_future_t *future = ev_promise_get_future(promise);

	struct io_timer_async_wait *async_wait = ev_promise_data(promise);
	async_wait->promise = promise;
	async_wait->wait = (struct io_timer_wait)IO_TIMER_WAIT_INIT(
			exec, &io_timer_async_wait_func);

	io_timer_submit_wait(timer, &async_wait->wait);

	if (pwait)
		*pwait = &async_wait->wait;

	return future;
}

struct io_timer_wait *
io_timer_wait_from_task(struct ev_task *task)
{
	return task ? structof(task, struct io_timer_wait, task) : NULL;
}

static void
io_timer_async_wait_func(struct ev_task *task)
{
	assert(task);
	struct io_timer_wait *wait = io_timer_wait_from_task(task);
	struct io_timer_async_wait *async_wait =
			structof(wait, struct io_timer_async_wait, wait);

	ev_promise_set(async_wait->promise, &wait->r);
	ev_promise_release(async_wait->promise);
}
