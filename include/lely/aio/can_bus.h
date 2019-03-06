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

#ifndef LELY_AIO_CAN_BUS_H_
#define LELY_AIO_CAN_BUS_H_

#include <lely/aio/exec.h>
#include <lely/can/msg.h>
#include <lely/libc/time.h>

#ifndef LELY_AIO_CAN_BUS_INLINE
#define LELY_AIO_CAN_BUS_INLINE inline
#endif

struct can_msg_info {
	struct timespec ts;
	int state;
	int error;
};

#define CAN_MSG_INFO_INIT \
	{ \
		{ 0, 0 }, 0, 0 \
	}

struct aio_can_bus_vtbl;
typedef const struct aio_can_bus_vtbl *const aio_can_bus_t;

enum aio_can_bus_option { AIO_CAN_BUS_FD_FRAMES, AIO_CAN_BUS_ERROR_FRAMES };

struct aio_can_bus_read_op {
	struct can_msg *msg;
	struct can_msg_info *info;
	struct aio_task task;
	int result;
};

struct aio_can_bus_write_op {
	const struct can_msg *msg;
	struct aio_task task;
	int result;
};

#ifdef __cplusplus
extern "C" {
#endif

struct aio_can_bus_vtbl {
	aio_exec_t *(*get_exec)(const aio_can_bus_t *bus);
	int (*read)(aio_can_bus_t *bus, struct can_msg *msg,
			struct can_msg_info *info);
	int (*submit_read)(aio_can_bus_t *bus, struct aio_can_bus_read_op *op);
	size_t (*cancel_read)(
			aio_can_bus_t *bus, struct aio_can_bus_read_op *op);
	int (*write)(aio_can_bus_t *bus, const struct can_msg *msg);
	int (*submit_write)(
			aio_can_bus_t *bus, struct aio_can_bus_write_op *op);
	size_t (*cancel_write)(
			aio_can_bus_t *bus, struct aio_can_bus_write_op *op);
	size_t (*cancel)(aio_can_bus_t *bus);
};

LELY_AIO_CAN_BUS_INLINE aio_exec_t *aio_can_bus_get_exec(
		const aio_can_bus_t *bus);

LELY_AIO_CAN_BUS_INLINE int aio_can_bus_read(aio_can_bus_t *bus,
		struct can_msg *msg, struct can_msg_info *info);
LELY_AIO_CAN_BUS_INLINE int aio_can_bus_submit_read(
		aio_can_bus_t *bus, struct aio_can_bus_read_op *op);
LELY_AIO_CAN_BUS_INLINE size_t aio_can_bus_cancel_read(
		aio_can_bus_t *bus, struct aio_can_bus_read_op *op);

LELY_AIO_CAN_BUS_INLINE int aio_can_bus_write(
		aio_can_bus_t *bus, const struct can_msg *msg);
LELY_AIO_CAN_BUS_INLINE int aio_can_bus_submit_write(
		aio_can_bus_t *bus, struct aio_can_bus_write_op *op);
LELY_AIO_CAN_BUS_INLINE size_t aio_can_bus_cancel_write(
		aio_can_bus_t *bus, struct aio_can_bus_write_op *op);

LELY_AIO_CAN_BUS_INLINE size_t aio_can_bus_cancel(aio_can_bus_t *bus);

aio_future_t *aio_can_bus_async_read(aio_can_bus_t *bus, aio_loop_t *loop,
		struct can_msg *msg, struct can_msg_info *info,
		struct aio_can_bus_read_op **pop);

int aio_can_bus_run_read(aio_can_bus_t *bus, aio_loop_t *loop,
		struct can_msg *msg, struct can_msg_info *info);
int aio_can_bus_run_read_until(aio_can_bus_t *bus, aio_loop_t *loop,
		struct can_msg *msg, struct can_msg_info *info,
		const struct timespec *abs_time);

aio_future_t *aio_can_bus_async_write(aio_can_bus_t *bus, aio_loop_t *loop,
		const struct can_msg *msg, struct aio_can_bus_write_op **pop);

int aio_can_bus_run_write(aio_can_bus_t *bus, aio_loop_t *loop,
		const struct can_msg *msg);
int aio_can_bus_run_write_until(aio_can_bus_t *bus, aio_loop_t *loop,
		const struct can_msg *msg, const struct timespec *abs_time);

void *aio_can_bus_alloc(void);
void aio_can_bus_free(void *ptr);
aio_can_bus_t *aio_can_bus_init(
		aio_can_bus_t *bus, aio_exec_t *exec, aio_reactor_t *reactor);
void aio_can_bus_fini(aio_can_bus_t *bus);

aio_can_bus_t *aio_can_bus_create(aio_exec_t *exec, aio_reactor_t *reactor);
void aio_can_bus_destroy(aio_can_bus_t *bus);

aio_handle_t aio_can_bus_get_handle(const aio_can_bus_t *bus);

int aio_can_bus_open(aio_can_bus_t *bus, const char *ifname);

int aio_can_bus_assign(aio_can_bus_t *bus, aio_handle_t handle);

aio_handle_t aio_can_bus_release(aio_can_bus_t *bus);

int aio_can_bus_is_open(const aio_can_bus_t *bus);

int aio_can_bus_close(aio_can_bus_t *bus);

int aio_can_bus_get_option(
		const aio_can_bus_t *bus, int name, void *pvalue, size_t *plen);

int aio_can_bus_set_option(
		aio_can_bus_t *bus, int name, const void *pvalue, size_t len);

LELY_AIO_CAN_BUS_INLINE aio_exec_t *
aio_can_bus_get_exec(const aio_can_bus_t *bus)
{
	return (*bus)->get_exec(bus);
}

LELY_AIO_CAN_BUS_INLINE int
aio_can_bus_read(aio_can_bus_t *bus, struct can_msg *msg,
		struct can_msg_info *info)
{
	return (*bus)->read(bus, msg, info);
}

LELY_AIO_CAN_BUS_INLINE int
aio_can_bus_submit_read(aio_can_bus_t *bus, struct aio_can_bus_read_op *op)
{
	return (*bus)->submit_read(bus, op);
}

LELY_AIO_CAN_BUS_INLINE size_t
aio_can_bus_cancel_read(aio_can_bus_t *bus, struct aio_can_bus_read_op *op)
{
	return (*bus)->cancel_read(bus, op);
}

LELY_AIO_CAN_BUS_INLINE int
aio_can_bus_write(aio_can_bus_t *bus, const struct can_msg *msg)
{
	return (*bus)->write(bus, msg);
}

LELY_AIO_CAN_BUS_INLINE int
aio_can_bus_submit_write(aio_can_bus_t *bus, struct aio_can_bus_write_op *op)
{
	return (*bus)->submit_write(bus, op);
}

LELY_AIO_CAN_BUS_INLINE size_t
aio_can_bus_cancel_write(aio_can_bus_t *bus, struct aio_can_bus_write_op *op)
{
	return (*bus)->cancel_write(bus, op);
}

LELY_AIO_CAN_BUS_INLINE size_t
aio_can_bus_cancel(aio_can_bus_t *bus)
{
	return (*bus)->cancel(bus);
}

#ifdef __cplusplus
}
#endif

#endif // LELY_AIO_CAN_BUS_H_
