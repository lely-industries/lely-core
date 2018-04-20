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

#ifndef LELY_AIO_STRAND_H_
#define LELY_AIO_STRAND_H_

#include <lely/aio/exec.h>

#ifdef __cplusplus
extern "C" {
#endif

LELY_AIO_EXTERN void *aio_strand_alloc(void);
LELY_AIO_EXTERN void aio_strand_free(void *ptr);
LELY_AIO_EXTERN aio_exec_t *aio_strand_init(aio_exec_t *exec,
		aio_exec_t *inner_exec);
LELY_AIO_EXTERN void aio_strand_fini(aio_exec_t *exec);

LELY_AIO_EXTERN aio_exec_t *aio_strand_create(aio_exec_t *inner_exec);
LELY_AIO_EXTERN void aio_strand_destroy(aio_exec_t *exec);

LELY_AIO_EXTERN aio_exec_t *aio_strand_get_inner_exec(const aio_exec_t *exec);

#ifdef __cplusplus
}
#endif

#endif // LELY_AIO_STRAND_H_
