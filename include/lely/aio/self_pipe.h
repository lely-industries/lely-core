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

#ifndef LELY_AIO_SELF_PIPE_H_
#define LELY_AIO_SELF_PIPE_H_

#include <lely/libc/sys/types.h>
#include <lely/aio/aio.h>

struct aio_self_pipe {
	aio_handle_t handles[2];
};

#define AIO_SELF_PIPE_INIT	{ { AIO_INVALID_HANDLE, AIO_INVALID_HANDLE } }

#ifdef __cplusplus
extern "C" {
#endif

LELY_AIO_EXTERN int aio_self_pipe_open(struct aio_self_pipe *pipe);
LELY_AIO_EXTERN int aio_self_pipe_is_open(struct aio_self_pipe *pipe);
LELY_AIO_EXTERN int aio_self_pipe_close(struct aio_self_pipe *pipe);

LELY_AIO_EXTERN ssize_t aio_self_pipe_read(struct aio_self_pipe *pipe);
LELY_AIO_EXTERN ssize_t aio_self_pipe_write(struct aio_self_pipe *pipe);

#ifdef __cplusplus
}
#endif

#endif // LELY_AIO_SELF_PIPE_H_
