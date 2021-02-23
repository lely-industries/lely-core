/**@file
 * This file is part of the I/O library; it exposes the abstract CAN bus
 * functions.
 *
 * @see lely/io2/can.h
 *
 * @copyright 2018-2021 Lely Industries N.V.
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
#define LELY_IO_CAN_INLINE extern inline
#include <lely/io2/can.h>
#include <lely/util/util.h>

#include <assert.h>

struct io_can_chan_async_read {
	ev_promise_t *promise;
	struct io_can_chan_read read;
};

static void io_can_chan_async_read_func(struct ev_task *task);

struct io_can_chan_async_write {
	ev_promise_t *promise;
	struct io_can_chan_write write;
};

static void io_can_chan_async_write_func(struct ev_task *task);

ev_future_t *
io_can_chan_async_read(io_can_chan_t *chan, ev_exec_t *exec,
		struct can_msg *msg, struct can_err *err, struct timespec *tp,
		struct io_can_chan_read **pread)
{
	ev_promise_t *promise = ev_promise_create(
			sizeof(struct io_can_chan_async_read), NULL);
	if (!promise)
		return NULL;
	ev_future_t *future = ev_promise_get_future(promise);

	struct io_can_chan_async_read *async_read = ev_promise_data(promise);
	async_read->promise = promise;
	async_read->read = (struct io_can_chan_read)IO_CAN_CHAN_READ_INIT(
			msg, err, tp, exec, &io_can_chan_async_read_func);

	io_can_chan_submit_read(chan, &async_read->read);

	if (pread)
		*pread = &async_read->read;

	return future;
}

ev_future_t *
io_can_chan_async_write(io_can_chan_t *chan, ev_exec_t *exec,
		const struct can_msg *msg, struct io_can_chan_write **pwrite)
{
	ev_promise_t *promise = ev_promise_create(
			sizeof(struct io_can_chan_async_write), NULL);
	if (!promise)
		return NULL;
	ev_future_t *future = ev_promise_get_future(promise);

	struct io_can_chan_async_write *async_write = ev_promise_data(promise);
	async_write->promise = promise;
	async_write->write = (struct io_can_chan_write)IO_CAN_CHAN_WRITE_INIT(
			msg, exec, &io_can_chan_async_write_func);

	io_can_chan_submit_write(chan, &async_write->write);

	if (pwrite)
		*pwrite = &async_write->write;

	return future;
}

struct io_can_chan_read *
io_can_chan_read_from_task(struct ev_task *task)
{
	return task ? structof(task, struct io_can_chan_read, task) : NULL;
}

struct io_can_chan_write *
io_can_chan_write_from_task(struct ev_task *task)
{
	return task ? structof(task, struct io_can_chan_write, task) : NULL;
}

static void
io_can_chan_async_read_func(struct ev_task *task)
{
	assert(task);
	struct io_can_chan_read *read = io_can_chan_read_from_task(task);
	struct io_can_chan_async_read *async_read =
			structof(read, struct io_can_chan_async_read, read);

	ev_promise_set(async_read->promise, &read->r);
	ev_promise_release(async_read->promise);
}

static void
io_can_chan_async_write_func(struct ev_task *task)
{
	assert(task);
	struct io_can_chan_write *write = io_can_chan_write_from_task(task);
	struct io_can_chan_async_write *async_write =
			structof(write, struct io_can_chan_async_write, write);

	ev_promise_set(async_write->promise, &write->errc);
	ev_promise_release(async_write->promise);
}
