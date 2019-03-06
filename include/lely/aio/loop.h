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

#ifndef LELY_AIO_LOOP_H_
#define LELY_AIO_LOOP_H_

#include <lely/aio/future.h>
#include <lely/aio/poll.h>

#ifdef __cplusplus
extern "C" {
#endif

void *aio_loop_alloc(void);
void aio_loop_free(void *ptr);
aio_loop_t *aio_loop_init(aio_loop_t *loop, aio_poll_t *poll);
void aio_loop_fini(aio_loop_t *loop);

aio_loop_t *aio_loop_create(aio_poll_t *poll);
void aio_loop_destroy(aio_loop_t *loop);

aio_poll_t *aio_loop_get_poll(const aio_loop_t *loop);

struct aio_task *aio_loop_get(aio_loop_t *loop, aio_future_t *const *futures,
		size_t nfutures);
struct aio_task *aio_loop_get_until(aio_loop_t *loop,
		aio_future_t *const *futures, size_t nfutures,
		const struct timespec *abs_time);

void aio_loop_post(aio_loop_t *loop, struct aio_task *task);
void aio_loop_on_task_started(aio_loop_t *loop);
void aio_loop_on_task_finished(aio_loop_t *loop);

size_t aio_loop_run(aio_loop_t *loop, aio_future_t *const *futures,
		size_t nfutures);
size_t aio_loop_run_until(aio_loop_t *loop, aio_future_t *const *futures,
		size_t nfutures, const struct timespec *abs_time);

void aio_loop_stop(aio_loop_t *loop);
int aio_loop_stopped(const aio_loop_t *loop);
void aio_loop_restart(aio_loop_t *loop);

#ifdef __cplusplus
}
#endif

#endif // LELY_AIO_LOOP_H_
