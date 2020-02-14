/**@file
 * This header file is part of the event library; it contains the standard
 * executor declarations.
 *
 * The standard executor provides an implementation of ev_exec_dispatch(),
 * ev_exec_defer() and ev_exec_run() in terms of ev_exec_post(). This allows
 * event loops to implement a reduced version of the abstract executor
 * interface and still provide a full executor.
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#ifndef LELY_EV_STD_EXEC_H_
#define LELY_EV_STD_EXEC_H_

#include <lely/ev/ev.h>

#include <stddef.h>

typedef const struct ev_std_exec_impl_vtbl *const ev_std_exec_impl_t;

struct ev_std_exec {
	const struct ev_exec_vtbl *exec_vptr;
	ev_std_exec_impl_t *impl;
};

#ifdef __cplusplus
extern "C" {
#endif

struct ev_std_exec_impl_vtbl {
	void (*on_task_init)(ev_std_exec_impl_t *impl);
	void (*on_task_fini)(ev_std_exec_impl_t *impl);
	void (*post)(ev_std_exec_impl_t *impl, struct ev_task *task);
	size_t (*abort)(ev_std_exec_impl_t *impl, struct ev_task *task);
};

void *ev_std_exec_alloc(void);
void ev_std_exec_free(void *ptr);
ev_exec_t *ev_std_exec_init(ev_exec_t *exec, ev_std_exec_impl_t *impl);
void ev_std_exec_fini(ev_exec_t *exec);

ev_exec_t *ev_std_exec_create(ev_std_exec_impl_t *impl);
void ev_std_exec_destroy(ev_exec_t *exec);

#ifdef __cplusplus
}
#endif

#endif // !LELY_EV_STD_EXEC_H_
