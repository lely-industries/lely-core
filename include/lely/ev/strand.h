/**@file
 * This header file is part of the event library; it contains the strand
 * executor declarations.
 *
 * A strand provides guarantees of ordering and non-concurrency. Tasks run in
 * the order in which they are submitted to a strand, and the invocation of a
 * task function by the strand is never concurrent with that of another task
 * function submitted to the same strand.
 *
 * A strand never executes tasks itself; it operates by serializing the tasks
 * submitted to it and forwarding them one-at-a-time to an inner executor. This
 * allows strands to be used as an adaptor for arbitrary executors.
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

#ifndef LELY_EV_STRAND_H_
#define LELY_EV_STRAND_H_

#include <lely/ev/ev.h>

#ifdef __cplusplus
extern "C" {
#endif

void *ev_strand_alloc(void);
void ev_strand_free(void *ptr);
ev_exec_t *ev_strand_init(ev_exec_t *exec, ev_exec_t *inner_exec);
void ev_strand_fini(ev_exec_t *exec);

/**
 * Creates a strand executor.
 *
 * @param inner_exec a pointer to the inner executor used to execute the tasks.
 *
 * @returns a pointer to a new strand executor, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
ev_exec_t *ev_strand_create(ev_exec_t *inner_exec);

/// Destroys a strand executor. @see ev_strand_create()
void ev_strand_destroy(ev_exec_t *exec);

/// Returns a pointer to the inner executor of a strand.
ev_exec_t *ev_strand_get_inner_exec(const ev_exec_t *exec);

#ifdef __cplusplus
}
#endif

#endif // !LELY_EV_STRAND_H_
