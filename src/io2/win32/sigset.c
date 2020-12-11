/**@file
 * This file is part of the I/O library; it contains the system signal handler
 * implementation for Windows.
 *
 * @see lely/io2/sys/sigset.h
 *
 * @copyright 2019 Lely Industries N.V.
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

#include "io.h"

#if !LELY_NO_STDIO && _WIN32

#include "../sigset.h"
#include <lely/io2/ctx.h>
#include <lely/io2/sys/sigset.h>
#include <lely/io2/win32/poll.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

#include <processthreadsapi.h>

#ifndef LELY_NSIG
#ifdef NSIG
#define LELY_NSIG NSIG
#else
#define LELY_NSIG (SIGABRT + 1)
#endif
#endif

struct io_sigset_node {
	struct io_sigset_node *next;
	unsigned signo : 30;
	unsigned watched : 1;
	unsigned pending : 1;
};

#define IO_SIGSET_NODE_INIT(signo) \
	{ \
		NULL, (signo), 0, 0 \
	}

static struct {
	CRITICAL_SECTION CriticalSection;
	struct io_sigset_node *list[LELY_NSIG - 1];
} io_sigset_shared = { .list = { NULL } };

static BOOL WINAPI io_sigset_handler_routine(DWORD dwCtrlType);
static BOOL io_sigset_handler(int signo);

static io_ctx_t *io_sigset_impl_dev_get_ctx(const io_dev_t *dev);
static ev_exec_t *io_sigset_impl_dev_get_exec(const io_dev_t *dev);
static size_t io_sigset_impl_dev_cancel(io_dev_t *dev, struct ev_task *task);
static size_t io_sigset_impl_dev_abort(io_dev_t *dev, struct ev_task *task);

// clang-format off
static const struct io_dev_vtbl io_sigset_impl_dev_vtbl = {
	&io_sigset_impl_dev_get_ctx,
	&io_sigset_impl_dev_get_exec,
	&io_sigset_impl_dev_cancel,
	&io_sigset_impl_dev_abort
};
// clang-format on

static io_dev_t *io_sigset_impl_get_dev(const io_sigset_t *sigset);
static int io_sigset_impl_clear(io_sigset_t *sigset);
static int io_sigset_impl_insert(io_sigset_t *sigset, int signo);
static int io_sigset_impl_remove(io_sigset_t *sigset, int signo);
static void io_sigset_impl_submit_wait(
		io_sigset_t *sigset, struct io_sigset_wait *wait);

// clang-format off
static const struct io_sigset_vtbl io_sigset_impl_vtbl = {
	&io_sigset_impl_get_dev,
	&io_sigset_impl_clear,
	&io_sigset_impl_insert,
	&io_sigset_impl_remove,
	&io_sigset_impl_submit_wait
};
// clang-format on

static void io_sigset_impl_svc_shutdown(struct io_svc *svc);

// clang-format off
static const struct io_svc_vtbl io_sigset_impl_svc_vtbl = {
	NULL,
	&io_sigset_impl_svc_shutdown
};
// clang-format on

struct io_sigset_impl {
	const struct io_dev_vtbl *dev_vptr;
	const struct io_sigset_vtbl *sigset_vptr;
	io_poll_t *poll;
	struct io_svc svc;
	io_ctx_t *ctx;
	ev_exec_t *exec;
	struct io_cp signal_cp;
	struct ev_task wait_task;
#if !LELY_NO_THREADS
	// The critical section protecting shutdown, wait_posted, pending and
	// queue. signal_posted and nodes are protected by the critical section
	// in io_sigset_shared.
	CRITICAL_SECTION CriticalSection;
#endif
	unsigned shutdown : 1;
	unsigned signal_posted : 1;
	unsigned wait_posted : 1;
	unsigned pending : 1;
	struct sllist queue;
	struct io_sigset_node nodes[LELY_NSIG - 1];
};

static void io_sigset_impl_signal_cp_func(
		struct io_cp *cp, size_t nbytes, int errc);
static void io_sigset_impl_wait_task_func(struct ev_task *task);

static inline struct io_sigset_impl *io_sigset_impl_from_dev(
		const io_dev_t *dev);
static inline struct io_sigset_impl *io_sigset_impl_from_sigset(
		const io_sigset_t *sigset);
static inline struct io_sigset_impl *io_sigset_impl_from_svc(
		const struct io_svc *svc);

static void io_sigset_impl_pop(struct io_sigset_impl *impl,
		struct sllist *queue, struct ev_task *task);

static void io_sigset_impl_do_insert(struct io_sigset_impl *impl, int signo);
static void io_sigset_impl_do_remove(struct io_sigset_impl *impl, int signo);

int
io_win32_sigset_init(void)
{
	InitializeCriticalSection(&io_sigset_shared.CriticalSection);
	if (!SetConsoleCtrlHandler(&io_sigset_handler_routine, TRUE)) {
		DeleteCriticalSection(&io_sigset_shared.CriticalSection);
		return -1;
	}
	return 0;
}

void
io_win32_sigset_fini(void)
{
	SetConsoleCtrlHandler(&io_sigset_handler_routine, FALSE);
	DeleteCriticalSection(&io_sigset_shared.CriticalSection);
}

void *
io_sigset_alloc(void)
{
	struct io_sigset_impl *impl = malloc(sizeof(*impl));
	if (!impl) {
		set_errc(errno2c(errno));
		return NULL;
	}
	return &impl->sigset_vptr;
}

void
io_sigset_free(void *ptr)
{
	if (ptr)
		free(io_sigset_impl_from_sigset(ptr));
}

io_sigset_t *
io_sigset_init(io_sigset_t *sigset, io_poll_t *poll, ev_exec_t *exec)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_sigset(sigset);
	assert(poll);
	assert(exec);
	io_ctx_t *ctx = io_poll_get_ctx(poll);
	assert(ctx);

	impl->dev_vptr = &io_sigset_impl_dev_vtbl;
	impl->sigset_vptr = &io_sigset_impl_vtbl;

	impl->svc = (struct io_svc)IO_SVC_INIT(&io_sigset_impl_svc_vtbl);
	impl->ctx = ctx;

	impl->poll = poll;

	impl->exec = exec;

	impl->signal_cp = (struct io_cp)IO_CP_INIT(
			&io_sigset_impl_signal_cp_func);
	impl->wait_task = (struct ev_task)EV_TASK_INIT(
			impl->exec, &io_sigset_impl_wait_task_func);

#if !LELY_NO_THREADS
	InitializeCriticalSection(&impl->CriticalSection);
#endif

	impl->shutdown = 0;
	impl->signal_posted = 0;
	impl->wait_posted = 0;
	impl->pending = 0;

	sllist_init(&impl->queue);

	for (int i = 1; i < LELY_NSIG; i++)
		impl->nodes[i - 1] =
				(struct io_sigset_node)IO_SIGSET_NODE_INIT(i);

	io_ctx_insert(impl->ctx, &impl->svc);

	return sigset;
}

void
io_sigset_fini(io_sigset_t *sigset)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_sigset(sigset);

	io_ctx_remove(impl->ctx, &impl->svc);
	// Cancel all pending tasks.
	io_sigset_impl_svc_shutdown(&impl->svc);

	// TODO: Find a reliable way to wait for io_sigset_impl_signal_cp_func()
	// to complete.

#if !LELY_NO_THREADS
	EnterCriticalSection(&impl->CriticalSection);
	// If necessary, busy-wait until io_sigset_impl_wait_task_func()
	// completes.
	while (impl->wait_posted) {
		// Try to abort io_sigset_impl_wait_task_func().
		if (ev_exec_abort(impl->wait_task.exec, &impl->wait_task))
			break;
		LeaveCriticalSection(&impl->CriticalSection);
		SwitchToThread();
		EnterCriticalSection(&impl->CriticalSection);
	}
	LeaveCriticalSection(&impl->CriticalSection);

	DeleteCriticalSection(&impl->CriticalSection);
#endif
}

io_sigset_t *
io_sigset_create(io_poll_t *poll, ev_exec_t *exec)
{
	DWORD dwErrCode = 0;

	io_sigset_t *sigset = io_sigset_alloc();
	if (!sigset) {
		dwErrCode = GetLastError();
		goto error_alloc;
	}

	io_sigset_t *tmp = io_sigset_init(sigset, poll, exec);
	if (!tmp) {
		dwErrCode = GetLastError();
		goto error_init;
	}
	sigset = tmp;

	return sigset;

error_init:
	io_sigset_free((void *)sigset);
error_alloc:
	SetLastError(dwErrCode);
	return NULL;
}

void
io_sigset_destroy(io_sigset_t *sigset)
{
	if (sigset) {
		io_sigset_fini(sigset);
		io_sigset_free((void *)sigset);
	}
}

static BOOL WINAPI
io_sigset_handler_routine(DWORD dwCtrlType)
{
	switch (dwCtrlType) {
	case CTRL_C_EVENT: return io_sigset_handler(SIGINT);
	case CTRL_BREAK_EVENT: return io_sigset_handler(SIGBREAK);
	case CTRL_CLOSE_EVENT:
		if (io_sigset_handler(SIGHUP)) {
			// Windows will terminate the process after the handler
			// returns, so we wait to allow an event loop to process
			// the signal.
			Sleep(INFINITE);
			return TRUE;
		}
		return FALSE;
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
	default: return FALSE;
	}
}

static BOOL
io_sigset_handler(int signo)
{
	assert(signo > 0);
	assert(signo < LELY_NSIG);

	EnterCriticalSection(&io_sigset_shared.CriticalSection);
	struct io_sigset_node *list = io_sigset_shared.list[signo - 1];
	for (struct io_sigset_node *node = list; node; node = node->next) {
		// cppcheck-suppress nullPointer
		struct io_sigset_impl *impl = structof(
				node, struct io_sigset_impl, nodes[signo - 1]);
		assert(node->watched);
		if (!node->pending) {
			node->pending = 1;
			if (!impl->signal_posted) {
				impl->signal_posted = 1;
				io_poll_post(impl->poll, 0, &impl->signal_cp);
			}
		}
	}
	LeaveCriticalSection(&io_sigset_shared.CriticalSection);
	return list != NULL;
}

static io_ctx_t *
io_sigset_impl_dev_get_ctx(const io_dev_t *dev)
{
	const struct io_sigset_impl *impl = io_sigset_impl_from_dev(dev);

	return impl->ctx;
}

static ev_exec_t *
io_sigset_impl_dev_get_exec(const io_dev_t *dev)
{
	const struct io_sigset_impl *impl = io_sigset_impl_from_dev(dev);

	return impl->exec;
}

static size_t
io_sigset_impl_dev_cancel(io_dev_t *dev, struct ev_task *task)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

	io_sigset_impl_pop(impl, &queue, task);

	return io_sigset_wait_queue_post(&queue, 0);
}

static size_t
io_sigset_impl_dev_abort(io_dev_t *dev, struct ev_task *task)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

	io_sigset_impl_pop(impl, &queue, task);

	return ev_task_queue_abort(&queue);
}

static io_dev_t *
io_sigset_impl_get_dev(const io_sigset_t *sigset)
{
	const struct io_sigset_impl *impl = io_sigset_impl_from_sigset(sigset);

	return &impl->dev_vptr;
}

static int
io_sigset_impl_clear(io_sigset_t *sigset)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_sigset(sigset);

	EnterCriticalSection(&io_sigset_shared.CriticalSection);
	for (int i = 1; i < LELY_NSIG; i++)
		io_sigset_impl_do_remove(impl, i);
	LeaveCriticalSection(&io_sigset_shared.CriticalSection);

	return 0;
}

static int
io_sigset_impl_insert(io_sigset_t *sigset, int signo)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_sigset(sigset);

	if (signo <= 0 || signo >= LELY_NSIG) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return -1;
	}

	EnterCriticalSection(&io_sigset_shared.CriticalSection);
	io_sigset_impl_do_insert(impl, signo);
	LeaveCriticalSection(&io_sigset_shared.CriticalSection);

	return 0;
}

static int
io_sigset_impl_remove(io_sigset_t *sigset, int signo)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_sigset(sigset);

	if (signo <= 0 || signo >= LELY_NSIG) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return -1;
	}

	EnterCriticalSection(&io_sigset_shared.CriticalSection);
	io_sigset_impl_do_remove(impl, signo);
	LeaveCriticalSection(&io_sigset_shared.CriticalSection);

	return 0;
}

static void
io_sigset_impl_submit_wait(io_sigset_t *sigset, struct io_sigset_wait *wait)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_sigset(sigset);
	assert(wait);
	struct ev_task *task = &wait->task;

	if (!task->exec)
		task->exec = impl->exec;
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	EnterCriticalSection(&impl->CriticalSection);
#endif
	if (impl->shutdown) {
#if !LELY_NO_THREADS
		LeaveCriticalSection(&impl->CriticalSection);
#endif
		io_sigset_wait_post(wait, 0);
	} else {
		sllist_push_back(&impl->queue, &task->_node);
		int post_wait = !impl->wait_posted && impl->pending;
		if (post_wait)
			impl->wait_posted = 1;
#if !LELY_NO_THREADS
		LeaveCriticalSection(&impl->CriticalSection);
#endif
		// cppcheck-suppress duplicateCondition
		if (post_wait)
			ev_exec_post(impl->wait_task.exec, &impl->wait_task);
	}
}

static void
io_sigset_impl_svc_shutdown(struct io_svc *svc)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_svc(svc);
	io_dev_t *dev = &impl->dev_vptr;

#if !LELY_NO_THREADS
	EnterCriticalSection(&impl->CriticalSection);
#endif
	int shutdown = !impl->shutdown;
	impl->shutdown = 1;
	// Try to abort io_sigset_impl_wait_task_func().
	// clang-format off
	if (shutdown && impl->wait_posted
			&& ev_exec_abort(impl->wait_task.exec,
					&impl->wait_task))
		// clang-format on
		impl->wait_posted = 0;
#if !LELY_NO_THREADS
	LeaveCriticalSection(&impl->CriticalSection);
#endif

	if (shutdown)
		// Cancel all pending operations.
		io_sigset_impl_dev_cancel(dev, NULL);
}

static void
io_sigset_impl_signal_cp_func(struct io_cp *cp, size_t nbytes, int errc)
{
	assert(cp);
	struct io_sigset_impl *impl =
			structof(cp, struct io_sigset_impl, signal_cp);
	(void)nbytes;
	(void)errc;

#if !LELY_NO_THREADS
	EnterCriticalSection(&impl->CriticalSection);
#endif
	impl->pending = 1;
	int post_wait = !impl->wait_posted && !sllist_empty(&impl->queue)
			&& !impl->shutdown;
	if (post_wait)
		impl->wait_posted = 1;
#if !LELY_NO_THREADS
	LeaveCriticalSection(&impl->CriticalSection);
#endif

	EnterCriticalSection(&io_sigset_shared.CriticalSection);
	impl->signal_posted = 0;
	LeaveCriticalSection(&io_sigset_shared.CriticalSection);

	if (post_wait)
		ev_exec_post(impl->wait_task.exec, &impl->wait_task);
}

static void
io_sigset_impl_wait_task_func(struct ev_task *task)
{
	assert(task);
	struct io_sigset_impl *impl =
			structof(task, struct io_sigset_impl, wait_task);

	struct io_sigset_wait *wait = NULL;
	int signo = LELY_NSIG;

#if !LELY_NO_THREADS
	EnterCriticalSection(&impl->CriticalSection);
#endif
	if (!sllist_empty(&impl->queue)) {
		EnterCriticalSection(&io_sigset_shared.CriticalSection);
		for (signo = 1; signo < LELY_NSIG; signo++) {
			struct io_sigset_node *node = &impl->nodes[signo - 1];
			if (node->pending) {
				node->pending = 0;
				task = ev_task_from_node(
						sllist_pop_front(&impl->queue));
				wait = io_sigset_wait_from_task(task);
				break;
			}
		}
		LeaveCriticalSection(&io_sigset_shared.CriticalSection);
	}

	impl->pending = signo != LELY_NSIG;
	int post_wait = impl->wait_posted = impl->pending
			&& !sllist_empty(&impl->queue) && !impl->shutdown;
#if !LELY_NO_THREADS
	LeaveCriticalSection(&impl->CriticalSection);
#endif

	if (wait)
		io_sigset_wait_post(wait, signo);

	if (post_wait)
		ev_exec_post(impl->wait_task.exec, &impl->wait_task);
}

static inline struct io_sigset_impl *
io_sigset_impl_from_dev(const io_dev_t *dev)
{
	assert(dev);

	return structof(dev, struct io_sigset_impl, dev_vptr);
}

static inline struct io_sigset_impl *
io_sigset_impl_from_sigset(const io_sigset_t *sigset)
{
	assert(sigset);

	return structof(sigset, struct io_sigset_impl, sigset_vptr);
}

static inline struct io_sigset_impl *
io_sigset_impl_from_svc(const struct io_svc *svc)
{
	assert(svc);

	return structof(svc, struct io_sigset_impl, svc);
}

static void
io_sigset_impl_pop(struct io_sigset_impl *impl, struct sllist *queue,
		struct ev_task *task)
{
	assert(impl);
	assert(queue);

#if !LELY_NO_THREADS
	EnterCriticalSection(&impl->CriticalSection);
#endif
	if (!task)
		sllist_append(queue, &impl->queue);
	else if (sllist_remove(&impl->queue, &task->_node))
		sllist_push_back(queue, &task->_node);
#if !LELY_NO_THREADS
	LeaveCriticalSection(&impl->CriticalSection);
#endif
}

static void
io_sigset_impl_do_insert(struct io_sigset_impl *impl, int signo)
{
	assert(impl);
	assert(signo > 0);
	assert(signo < LELY_NSIG);

	struct io_sigset_node *node = &impl->nodes[signo - 1];
	assert(node->signo == signo);

	if (node->watched)
		return;
	node->watched = 1;
	assert(!node->pending);

	node->next = io_sigset_shared.list[signo - 1];
	io_sigset_shared.list[signo - 1] = node;
}

static void
io_sigset_impl_do_remove(struct io_sigset_impl *impl, int signo)
{
	assert(impl);
	assert(signo > 0);
	assert(signo < LELY_NSIG);

	struct io_sigset_node *node = &impl->nodes[signo - 1];
	assert(node->signo == signo);

	if (!node->watched)
		return;
	node->watched = 0;
	node->pending = 0;

	struct io_sigset_node **pnode = &io_sigset_shared.list[signo - 1];
	assert(*pnode);
	while (*pnode != node)
		pnode = &(*pnode)->next;
	assert(*pnode == node);
	*pnode = node->next;
	node->next = NULL;
}

#endif // !LELY_NO_STDIO && _WIN32
