/*!\file
 * This file is part of the asynchronous I/O library; it contains ...
 *
 * \see lely/aio/can_bus.h
 *
 * \copyright 2018 Lely Industries N.V.
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

#include "aio.h"
#include <lely/util/errnum.h>
#define LELY_AIO_CAN_BUS_INLINE	extern inline LELY_DLL_EXPORT
#include <lely/aio/can_bus.h>
#include <lely/aio/future.h>

#include <assert.h>
#include <stdlib.h>

struct aio_can_bus_read_data {
	aio_promise_t *promise;
	struct aio_can_bus_read_op op;
};

static void aio_can_bus_read_func(struct aio_task *task);

struct aio_can_bus_write_data {
	aio_promise_t *promise;
	struct aio_can_bus_write_op op;
};

static void aio_can_bus_write_func(struct aio_task *task);

LELY_AIO_EXPORT aio_future_t *
aio_can_bus_async_read(aio_can_bus_t *bus, aio_loop_t *loop,
		struct can_msg *msg, struct can_msg_info *info,
		struct aio_can_bus_read_op **pop)
{
	int errc = 0;

	aio_exec_t *exec = aio_can_bus_get_exec(bus);

	struct aio_can_bus_read_data *data = malloc(sizeof(*data));
	if (!data) {
		errc = get_errc();
		goto error_malloc_data;
	}
	data->promise = aio_promise_create(loop, exec, &free, data);
	if (!data->promise) {
		errc = get_errc();
		goto error_create_promise;
	}
	data->op.msg = msg;
	data->op.info = info;
	data->op.task = (struct aio_task)AIO_TASK_INIT(exec,
			&aio_can_bus_read_func);

	aio_future_t *future = aio_future_create(data->promise);
	assert(future);

	if (aio_can_bus_submit_read(bus, &data->op) == -1) {
		errc = get_errc();
		goto error_submit_read;
	}

	if (pop)
		*pop = &data->op;

	return future;

error_submit_read:
	aio_future_destroy(future);
	aio_promise_destroy(data->promise);
error_create_promise:
error_malloc_data:
	set_errc(errc);
	return NULL;
}

LELY_AIO_EXPORT int
aio_can_bus_run_read(aio_can_bus_t *bus, aio_loop_t *loop,
		struct can_msg *msg, struct can_msg_info *info)
{
	struct aio_can_bus_read_op *op = NULL;
	aio_future_t *future =
			aio_can_bus_async_read(bus, loop, msg, info, &op);
	if (!future)
		return 0;

	int result = -1;
	int errc = 0;

	set_errc(0);
	aio_future_run_wait(future);
	switch (aio_future_get_state(future)) {
	case AIO_FUTURE_WAITING:
		errc = get_errc();
		aio_can_bus_cancel_read(bus, op);
		break;
	case AIO_FUTURE_CANCELED:
		errc = errnum2c(ERRNUM_CANCELED);
		break;
	case AIO_FUTURE_VALUE:
		op = aio_future_get_value(future);
		assert(op);
		errc = op->task.errc;
		result = op->result;
		break;
	case AIO_FUTURE_ERROR:
		errc = aio_future_get_errc(future);
		break;
	}
	aio_future_destroy(future);

	set_errc(errc);
	return result;
}

LELY_AIO_EXPORT int
aio_can_bus_run_read_until(aio_can_bus_t *bus, aio_loop_t *loop,
		struct can_msg *msg, struct can_msg_info *info,
		const struct timespec *abs_time)
{

	struct aio_can_bus_read_op *op = NULL;
	aio_future_t *future =
			aio_can_bus_async_read(bus, loop, msg, info, &op);
	if (!future)
		return 0;

	int result = -1;
	int errc = 0;

	set_errc(0);
	aio_future_run_wait_until(future, abs_time);
	switch (aio_future_get_state(future)) {
	case AIO_FUTURE_WAITING:
		errc = get_errc();
		aio_can_bus_cancel_read(bus, op);
		break;
	case AIO_FUTURE_CANCELED:
		errc = errnum2c(ERRNUM_CANCELED);
		break;
	case AIO_FUTURE_VALUE:
		op = aio_future_get_value(future);
		assert(op);
		errc = op->task.errc;
		result = op->result;
		break;
	case AIO_FUTURE_ERROR:
		errc = aio_future_get_errc(future);
		break;
	}
	aio_future_destroy(future);

	set_errc(errc);
	return result;
}

LELY_AIO_EXPORT aio_future_t *
aio_can_bus_async_write(aio_can_bus_t *bus, aio_loop_t *loop,
		const struct can_msg *msg, struct aio_can_bus_write_op **pop)
{
	int errc = 0;

	aio_exec_t *exec = aio_can_bus_get_exec(bus);

	struct aio_can_bus_write_data *data = malloc(sizeof(*data));
	if (!data) {
		errc = get_errc();
		goto error_malloc_data;
	}
	data->promise = aio_promise_create(loop, exec, &free, data);
	if (!data->promise) {
		errc = get_errc();
		goto error_create_promise;
	}
	data->op.msg = msg;
	data->op.task = (struct aio_task)AIO_TASK_INIT(exec,
			&aio_can_bus_write_func);

	aio_future_t *future = aio_future_create(data->promise);
	assert(future);

	if (aio_can_bus_submit_write(bus, &data->op) == -1) {
		errc = get_errc();
		goto error_submit_write;
	}

	if (pop)
		*pop = &data->op;

	return future;

error_submit_write:
	aio_future_destroy(future);
	aio_promise_destroy(data->promise);
error_create_promise:
error_malloc_data:
	set_errc(errc);
	return NULL;
}

LELY_AIO_EXPORT int
aio_can_bus_run_write(aio_can_bus_t *bus, aio_loop_t *loop,
		const struct can_msg *msg)
{
	struct aio_can_bus_write_op *op = NULL;
	aio_future_t *future = aio_can_bus_async_write(bus, loop, msg, &op);
	if (!future)
		return 0;

	int result = -1;
	int errc = 0;

	aio_future_run_wait(future);
	set_errc(0);
	switch (aio_future_get_state(future)) {
	case AIO_FUTURE_WAITING:
		errc = get_errc();
		aio_can_bus_cancel_write(bus, op);
		break;
	case AIO_FUTURE_CANCELED:
		errc = errnum2c(ERRNUM_CANCELED);
		break;
	case AIO_FUTURE_VALUE:
		op = aio_future_get_value(future);
		assert(op);
		errc = op->task.errc;
		result = op->result;
		break;
	case AIO_FUTURE_ERROR:
		errc = aio_future_get_errc(future);
		break;
	}
	aio_future_destroy(future);

	set_errc(errc);
	return result;
}

LELY_AIO_EXPORT int
aio_can_bus_run_write_until(aio_can_bus_t *bus, aio_loop_t *loop,
		const struct can_msg *msg, const struct timespec *abs_time)
{
	struct aio_can_bus_write_op *op = NULL;
	aio_future_t *future = aio_can_bus_async_write(bus, loop, msg, &op);
	if (!future)
		return 0;

	int result = -1;
	int errc = 0;

	set_errc(0);
	aio_future_run_wait_until(future, abs_time);
	switch (aio_future_get_state(future)) {
	case AIO_FUTURE_WAITING:
		errc = get_errc();
		aio_can_bus_cancel_write(bus, op);
		break;
	case AIO_FUTURE_CANCELED:
		errc = errnum2c(ERRNUM_CANCELED);
		break;
	case AIO_FUTURE_VALUE:
		op = aio_future_get_value(future);
		assert(op);
		errc = op->task.errc;
		result = op->result;
		break;
	case AIO_FUTURE_ERROR:
		errc = aio_future_get_errc(future);
		break;
	}
	aio_future_destroy(future);

	set_errc(errc);
	return result;
}

static void
aio_can_bus_read_func(struct aio_task *task)
{
	assert(task);
	struct aio_can_bus_read_op *op =
			structof(task, struct aio_can_bus_read_op, task);
	struct aio_can_bus_read_data *data =
			structof(op, struct aio_can_bus_read_data, op);

	aio_promise_set_value(data->promise, op);
	aio_promise_destroy(data->promise);
}

static void
aio_can_bus_write_func(struct aio_task *task)
{
	assert(task);
	struct aio_can_bus_write_op *op =
			structof(task, struct aio_can_bus_write_op, task);
	struct aio_can_bus_write_data *data =
			structof(op, struct aio_can_bus_write_data, op);

	aio_promise_set_value(data->promise, op);
	aio_promise_destroy(data->promise);
}
