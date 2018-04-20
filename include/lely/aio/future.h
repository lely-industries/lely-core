/*!\file
 * This header file is part of the asynchronous I/O library; it contains ...
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

#ifndef LELY_AIO_FUTURE_H_
#define LELY_AIO_FUTURE_H_

#include <lely/libc/time.h>
#include <lely/aio/exec.h>

#include <stddef.h>

struct aio_promise;
typedef struct aio_promise aio_promise_t;

//! The state of a future.
enum aio_future_state {
	//! The future is waiting.
	AIO_FUTURE_WAITING,
	//! The future has been canceled.
	AIO_FUTURE_CANCELED,
	/*!
	 * The future is ready and the value has been set with
	 * aio_future_set_value().
	 */
	AIO_FUTURE_VALUE,
	/*!
	 * The future is ready and the error code has been set with
	 * aio_future_set_errc().
	 */
	AIO_FUTURE_ERROR
};

#ifdef __cplusplus
extern "C" {
#endif

typedef void aio_dtor_t(void *arg);

LELY_AIO_EXTERN aio_promise_t *aio_promise_create(aio_loop_t *loop,
		aio_exec_t *exec, aio_dtor_t *dtor, void *arg);
LELY_AIO_EXTERN void aio_promise_destroy(aio_promise_t *promise);

LELY_AIO_EXTERN void aio_promise_cancel(aio_promise_t *promise);
LELY_AIO_EXTERN void aio_promise_set_value(aio_promise_t *promise, void *value);
LELY_AIO_EXTERN void aio_promise_set_errc(aio_promise_t *promise, int errc);

LELY_AIO_EXTERN aio_future_t *aio_future_create(aio_promise_t *promise);
LELY_AIO_EXTERN void aio_future_destroy(aio_future_t *future);

LELY_AIO_EXTERN aio_loop_t *aio_future_get_loop(const aio_future_t *future);
LELY_AIO_EXTERN aio_exec_t *aio_future_get_exec(const aio_future_t *future);

LELY_AIO_EXTERN enum aio_future_state aio_future_get_state(
		const aio_future_t *future);

LELY_AIO_EXTERN int aio_future_is_ready(const aio_future_t *future);
LELY_AIO_EXTERN int aio_future_is_canceled(const aio_future_t *future);
LELY_AIO_EXTERN int aio_future_has_value(const aio_future_t *future);
LELY_AIO_EXTERN int aio_future_has_errc(const aio_future_t *future);

LELY_AIO_EXTERN void *aio_future_get_value(const aio_future_t *future);
LELY_AIO_EXTERN int aio_future_get_errc(const aio_future_t *future);

LELY_AIO_EXTERN void aio_future_submit_wait(aio_future_t *future,
		struct aio_task *task);
LELY_AIO_EXTERN size_t aio_future_cancel(aio_future_t *future,
		struct aio_task *task);

LELY_AIO_EXTERN void aio_future_run_wait(aio_future_t *future);
LELY_AIO_EXTERN void aio_future_run_wait_until(aio_future_t *future,
		const struct timespec *abs_time);

#ifdef __cplusplus
}
#endif

#endif // LELY_AIO_FUTURE_H_
