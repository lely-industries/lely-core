/**@file
 * This header file is part of the I/O library; it contains the abstract I/O
 * device interface.
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

#ifndef LELY_IO2_DEV_H_
#define LELY_IO2_DEV_H_

#include <lely/ev/ev.h>
#include <lely/io2/io2.h>

#include <stddef.h>

#ifndef LELY_IO_DEV_INLINE
#define LELY_IO_DEV_INLINE static inline
#endif

/// An abstract I/O device.
typedef const struct io_dev_vtbl *const io_dev_t;

#ifdef __cplusplus
extern "C" {
#endif

struct io_dev_vtbl {
	io_ctx_t *(*get_ctx)(const io_dev_t *dev);
	ev_exec_t *(*get_exec)(const io_dev_t *dev);
	size_t (*cancel)(io_dev_t *dev, struct ev_task *task);
	size_t (*abort)(io_dev_t *dev, struct ev_task *task);
};

/**
 * Returns a pointer to the I/O context with which the I/O device is registered.
 */
LELY_IO_DEV_INLINE io_ctx_t *io_dev_get_ctx(const io_dev_t *dev);

/**
 * Returns a pointer to the executor used by the I/O device to execute
 * asynchronous tasks.
 */
LELY_IO_DEV_INLINE ev_exec_t *io_dev_get_exec(const io_dev_t *dev);

/**
 * Cancels the asynchronous operation submitted to <b>*dev</b>, if its task has
 * not yet been submitted to its executor, or all pending operations if
 * <b>task</b> is NULL. All canceled tasks are submitted for execution before
 * this function returns. If and how cancellation is reported to the tasks
 * depends on the type of the I/O device and the asynchronous operation.
 *
 * @returns the number of aborted tasks.
 */
LELY_IO_DEV_INLINE size_t io_dev_cancel(io_dev_t *dev, struct ev_task *task);

/**
 * Aborts the asynchronous operation submitted to <b>*dev</b>, if its task has
 * not yet been submitted to its executor, or all pending operations if
 * <b>task</b> is NULL. Aborted tasks are _not_ submitted for execution.
 *
 * @returns the number of aborted tasks.
 */
LELY_IO_DEV_INLINE size_t io_dev_abort(io_dev_t *dev, struct ev_task *task);

inline io_ctx_t *
io_dev_get_ctx(const io_dev_t *dev)
{
	return (*dev)->get_ctx(dev);
}

inline ev_exec_t *
io_dev_get_exec(const io_dev_t *dev)
{
	return (*dev)->get_exec(dev);
}

inline size_t
io_dev_cancel(io_dev_t *dev, struct ev_task *task)
{
	return (*dev)->cancel(dev, task);
}

inline size_t
io_dev_abort(io_dev_t *dev, struct ev_task *task)
{
	return (*dev)->abort(dev, task);
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_DEV_H_
