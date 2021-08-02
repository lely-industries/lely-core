/**@file
 * This file is part of the I/O library; it contains the system signal handler
 * implementation for POSIX platforms.
 *
 * @see lely/io2/sys/sigset.h
 *
 * @copyright 2018-2021 Lely Industries N.V.
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

#if !LELY_NO_STDIO && _POSIX_C_SOURCE >= 200112L

#include "../sigset.h"
#include "fd.h"
#include <lely/io2/ctx.h>
#include <lely/io2/posix/poll.h>
#include <lely/io2/sys/sigset.h>
#include <lely/util/diag.h>
#include <lely/util/util.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#if !LELY_NO_THREADS
#include <pthread.h>
#include <sched.h>
#endif
#include <unistd.h>

#ifndef LELY_NSIG
#ifdef _NSIG
#define LELY_NSIG _NSIG
#else
#define LELY_NSIG 128
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
#if !LELY_NO_THREADS
	pthread_mutex_t mtx;
#endif
	sig_atomic_t pending;
	struct {
		sig_atomic_t pending;
		sig_atomic_t fd;
		struct io_sigset_node *list;
	} sig[LELY_NSIG - 1];
} io_sigset_shared = {
#if !LELY_NO_THREADS
	PTHREAD_MUTEX_INITIALIZER,
#endif
	0, { { 0, 0, NULL } }
};

static struct sigaction io_sigset_action[LELY_NSIG - 1];

static void io_sigset_handler(int signo);
static void io_sigset_kill(int signo);
static void io_sigset_process_all(void);
static void io_sigset_process_sig(int signo, struct sllist *queue);

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

static int io_sigset_impl_svc_notify_fork(
		struct io_svc *svc, enum io_fork_event e);
static void io_sigset_impl_svc_shutdown(struct io_svc *svc);

// clang-format off
static const struct io_svc_vtbl io_sigset_impl_svc_vtbl = {
	&io_sigset_impl_svc_notify_fork,
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
	struct io_poll_watch watch;
	int fds[2];
	struct ev_task read_task;
	struct ev_task wait_task;
#if !LELY_NO_THREADS
	pthread_mutex_t mtx;
#endif
	unsigned shutdown : 1;
	unsigned read_posted : 1;
	unsigned wait_posted : 1;
	unsigned pending : 1;
	struct sllist queue;
	struct io_sigset_node nodes[LELY_NSIG - 1];
};

static void io_sigset_impl_watch_func(struct io_poll_watch *watch, int events);
static void io_sigset_impl_read_task_func(struct ev_task *task);
static void io_sigset_impl_wait_task_func(struct ev_task *task);

static inline struct io_sigset_impl *io_sigset_impl_from_dev(
		const io_dev_t *dev);
static inline struct io_sigset_impl *io_sigset_impl_from_sigset(
		const io_sigset_t *sigset);
static inline struct io_sigset_impl *io_sigset_impl_from_svc(
		const struct io_svc *svc);

static void io_sigset_impl_pop(struct io_sigset_impl *impl,
		struct sllist *queue, struct ev_task *task);

static int io_sigset_impl_open(struct io_sigset_impl *impl);
static int io_sigset_impl_close(struct io_sigset_impl *impl);

static int io_sigset_impl_do_insert(struct io_sigset_impl *impl, int signo);
static int io_sigset_impl_do_remove(struct io_sigset_impl *impl, int signo);

static size_t io_sigset_impl_do_abort_tasks(struct io_sigset_impl *impl);

void *
io_sigset_alloc(void)
{
	struct io_sigset_impl *impl = malloc(sizeof(*impl));
	if (!impl)
		return NULL;
	// Suppress a GCC maybe-uninitialized warning.
	impl->sigset_vptr = NULL;
	// cppcheck-suppress memleak symbolName=impl
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

	int errsv = 0;

	impl->dev_vptr = &io_sigset_impl_dev_vtbl;
	impl->sigset_vptr = &io_sigset_impl_vtbl;

	impl->poll = poll;

	impl->svc = (struct io_svc)IO_SVC_INIT(&io_sigset_impl_svc_vtbl);
	impl->ctx = ctx;

	impl->exec = exec;

	impl->watch = (struct io_poll_watch)IO_POLL_WATCH_INIT(
			&io_sigset_impl_watch_func);
	impl->fds[0] = impl->fds[1] = -1;

	impl->read_task = (struct ev_task)EV_TASK_INIT(
			impl->exec, &io_sigset_impl_read_task_func);
	impl->wait_task = (struct ev_task)EV_TASK_INIT(
			impl->exec, &io_sigset_impl_wait_task_func);

#if !LELY_NO_THREADS
	if ((errsv = pthread_mutex_init(&impl->mtx, NULL)))
		goto error_init_mtx;
#endif

	impl->shutdown = 0;
	impl->read_posted = 0;
	impl->wait_posted = 0;
	impl->pending = 0;

	sllist_init(&impl->queue);

	for (int i = 1; i < LELY_NSIG; i++)
		impl->nodes[i - 1] =
				(struct io_sigset_node)IO_SIGSET_NODE_INIT(i);

	if (io_sigset_impl_open(impl) == -1) {
		errsv = errno;
		goto error_open;
	}

	io_ctx_insert(impl->ctx, &impl->svc);

	return sigset;

	// io_sigset_impl_close(impl);
error_open:
#if !LELY_NO_THREADS
	pthread_mutex_destroy(&impl->mtx);
error_init_mtx:
#endif
	errno = errsv;
	return NULL;
}

void
io_sigset_fini(io_sigset_t *sigset)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_sigset(sigset);

	io_ctx_remove(impl->ctx, &impl->svc);
	// Cancel all pending tasks.
	io_sigset_impl_svc_shutdown(&impl->svc);

	// Stop monitoring signals.
	io_sigset_impl_clear(sigset);

#if !LELY_NO_THREADS
	int warning = 0;
	pthread_mutex_lock(&impl->mtx);
	// If necessary, busy-wait until io_sigset_impl_read_task_func() and
	// io_sigset_impl_wait_task_func() complete.
	while (impl->read_posted || impl->wait_posted) {
		if (io_sigset_impl_do_abort_tasks(impl))
			continue;
		pthread_mutex_unlock(&impl->mtx);
		if (!warning) {
			warning = 1;
			diag(DIAG_WARNING, 0,
					"io_sigset_fini() invoked with pending operations");
		}
		sched_yield();
		pthread_mutex_lock(&impl->mtx);
	}
	pthread_mutex_unlock(&impl->mtx);
#endif

	// Close the self-pipe.
	io_sigset_impl_close(impl);

#if !LELY_NO_THREADS
	pthread_mutex_destroy(&impl->mtx);
#endif
}

io_sigset_t *
io_sigset_create(io_poll_t *poll, ev_exec_t *exec)
{
	int errsv = 0;

	io_sigset_t *sigset = io_sigset_alloc();
	if (!sigset) {
		errsv = errno;
		goto error_alloc;
	}

	io_sigset_t *tmp = io_sigset_init(sigset, poll, exec);
	if (!tmp) {
		errsv = errno;
		goto error_init;
	}
	sigset = tmp;

	return sigset;

error_init:
	io_sigset_free((void *)sigset);
error_alloc:
	errno = errsv;
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

static void
io_sigset_handler(int signo)
{
	io_sigset_shared.sig[signo - 1].pending = 1;
	io_sigset_shared.pending = 1;

	io_sigset_kill(signo);
}

static void
io_sigset_kill(int signo)
{
	int errsv = errno;
	ssize_t result;
	do {
		errno = 0;
		result = write(io_sigset_shared.sig[signo - 1].fd - 1, "", 1);
	} while (result == -1 && errno == EINTR);
	errno = errsv;
}

static void
io_sigset_process_all(void)
{
	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	pthread_mutex_lock(&io_sigset_shared.mtx);
#endif
	while (io_sigset_shared.pending) {
		io_sigset_shared.pending = 0;
		for (int i = 1; i < LELY_NSIG; i++) {
			if (io_sigset_shared.sig[i - 1].pending) {
				io_sigset_shared.sig[i - 1].pending = 0;
				io_sigset_process_sig(i, &queue);
			}
		}
	}
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&io_sigset_shared.mtx);
#endif

	struct slnode *node;
	while ((node = sllist_pop_front(&queue))) {
		struct ev_task *task = ev_task_from_node(node);
		ev_exec_post(task->exec, task);
	}
}

static void
io_sigset_process_sig(int signo, struct sllist *queue)
{
	for (struct io_sigset_node *node = io_sigset_shared.sig[signo - 1].list;
			node; node = node->next) {
		struct io_sigset_impl *impl = structof(
				node, struct io_sigset_impl, nodes[signo - 1]);
#if !LELY_NO_THREADS
		pthread_mutex_lock(&impl->mtx);
#endif
		assert(node->watched);
		if (!node->pending) {
			node->pending = 1;
			impl->pending = 1;
			if (!impl->wait_posted) {
				impl->wait_posted = 1;
				sllist_push_back(queue, &impl->wait_task._node);
			}
		}
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->mtx);
#endif
	}
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

#if !LELY_NO_THREADS
	// Locking this mutex may fail since it is statically initialized.
	int errsv = pthread_mutex_lock(&io_sigset_shared.mtx);
	if (errsv) {
		errno = errsv;
		return -1;
	}
#endif

	int result = 0;
	for (int i = 1; i < LELY_NSIG; i++) {
		if (io_sigset_impl_do_remove(impl, i) == -1)
			result = -1;
	}

#if !LELY_NO_THREADS
	pthread_mutex_unlock(&io_sigset_shared.mtx);
#endif

	return result;
}

static int
io_sigset_impl_insert(io_sigset_t *sigset, int signo)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_sigset(sigset);

	if (signo <= 0 || signo >= LELY_NSIG) {
		errno = EINVAL;
		return -1;
	}

#if !LELY_NO_THREADS
	// Locking this mutex may fail since it is statically initialized.
	int errsv = pthread_mutex_lock(&io_sigset_shared.mtx);
	if (errsv) {
		errno = errsv;
		return -1;
	}
#endif

	int result = io_sigset_impl_do_insert(impl, signo);

#if !LELY_NO_THREADS
	pthread_mutex_unlock(&io_sigset_shared.mtx);
#endif

	return result;
}

static int
io_sigset_impl_remove(io_sigset_t *sigset, int signo)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_sigset(sigset);

	if (signo <= 0 || signo >= LELY_NSIG) {
		errno = EINVAL;
		return -1;
	}

#if !LELY_NO_THREADS
	// Locking this mutex may fail since it is statically initialized.
	int errsv = pthread_mutex_lock(&io_sigset_shared.mtx);
	if (errsv) {
		errno = errsv;
		return -1;
	}
#endif

	int result = io_sigset_impl_do_remove(impl, signo);

#if !LELY_NO_THREADS
	pthread_mutex_unlock(&io_sigset_shared.mtx);
#endif

	return result;
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
	pthread_mutex_lock(&impl->mtx);
#endif
	if (impl->shutdown) {
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->mtx);
#endif
		io_sigset_wait_post(wait, 0);
	} else {
		sllist_push_back(&impl->queue, &task->_node);
		int post_wait = !impl->wait_posted && impl->pending;
		if (post_wait)
			impl->wait_posted = 1;
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->mtx);
#endif
		if (post_wait)
			ev_exec_post(impl->wait_task.exec, &impl->wait_task);
	}
}

static int
io_sigset_impl_svc_notify_fork(struct io_svc *svc, enum io_fork_event e)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_svc(svc);

	if (e != IO_FORK_CHILD || impl->shutdown)
		return 0;

	int result = 0;
	int errsv = errno;

	if (io_sigset_impl_close(impl) == -1 && !result) {
		errsv = errno;
		result = -1;
	}

	if (io_sigset_impl_open(impl) == -1 && !result) {
		errsv = errno;
		result = -1;
	}

	errno = errsv;
	return result;
}

static void
io_sigset_impl_svc_shutdown(struct io_svc *svc)
{
	struct io_sigset_impl *impl = io_sigset_impl_from_svc(svc);
	io_dev_t *dev = &impl->dev_vptr;

#if !LELY_NO_THREADS
	pthread_mutex_lock(&impl->mtx);
#endif
	int shutdown = !impl->shutdown;
	impl->shutdown = 1;
	if (shutdown) {
		// Stop monitoring the self-pipe.
		io_poll_watch(impl->poll, impl->fds[0], 0, &impl->watch);
		// Try to abort io_sigset_impl_read_task_func() and
		// io_sigset_impl_wait_task_func().
		io_sigset_impl_do_abort_tasks(impl);
	}
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->mtx);
#endif

	if (shutdown)
		// Cancel all pending operations.
		io_sigset_impl_dev_cancel(dev, NULL);
}

static void
io_sigset_impl_watch_func(struct io_poll_watch *watch, int events)
{
	assert(watch);
	struct io_sigset_impl *impl =
			structof(watch, struct io_sigset_impl, watch);
	(void)events;

#if !LELY_NO_THREADS
	pthread_mutex_lock(&impl->mtx);
#endif
	int post_read = !impl->read_posted;
	impl->read_posted = 1;
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->mtx);
#endif

	if (post_read)
		ev_exec_post(impl->read_task.exec, &impl->read_task);
}

static void
io_sigset_impl_read_task_func(struct ev_task *task)
{
	assert(task);
	struct io_sigset_impl *impl =
			structof(task, struct io_sigset_impl, read_task);

	int errsv = errno;
	int pending = 0;
	int events = 0;

	ssize_t result;
	do {
		char buf[LELY_VLA_SIZE_MAX];
		errno = 0;
		result = read(impl->fds[0], buf, sizeof(buf));
		if (result > 0)
			pending = 1;
	} while (result > 0 || (result == -1 && errno == EINTR));
	if (result == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		events |= IO_EVENT_IN;

	if (pending)
		io_sigset_process_all();

#if !LELY_NO_THREADS
	pthread_mutex_lock(&impl->mtx);
#endif
	if (events && !impl->shutdown)
		io_poll_watch(impl->poll, impl->fds[0], events, &impl->watch);
	impl->read_posted = 0;
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->mtx);
#endif

	errno = errsv;
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
	pthread_mutex_lock(&impl->mtx);
#endif
	if (!sllist_empty(&impl->queue)) {
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
	}

	impl->pending = signo != LELY_NSIG;
	int post_wait = impl->wait_posted = impl->pending
			&& !sllist_empty(&impl->queue) && !impl->shutdown;
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->mtx);
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
	pthread_mutex_lock(&impl->mtx);
#endif
	if (!task)
		sllist_append(queue, &impl->queue);
	else if (sllist_remove(&impl->queue, &task->_node))
		sllist_push_back(queue, &task->_node);
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->mtx);
#endif
}

static int
io_sigset_impl_open(struct io_sigset_impl *impl)
{
	assert(impl);

	int errsv = 0;

	if (io_sigset_impl_close(impl) == -1) {
		errsv = errno;
		goto error_close;
	}

#ifdef __linux__
	if (pipe2(impl->fds, O_CLOEXEC | O_NONBLOCK) == -1) {
#else
	if (pipe(impl->fds) == -1) {
#endif
		errsv = errno;
		goto error_pipe;
	}

	// We need to be able to store the write end of the pipe in a
	// sig_atomic_t variable.
	if (impl->fds[1] - 1 > SIG_ATOMIC_MAX) {
		errno = EBADF;
		goto error_sig_atomic;
	}

#ifndef __linux__
	if (io_fd_set_cloexec(impl->fds[0]) == -1) {
		errsv = errno;
		goto error_cloexec;
	}

	if (io_fd_set_cloexec(impl->fds[1]) == -1) {
		errsv = errno;
		goto error_cloexec;
	}

	if (io_fd_set_nonblock(impl->fds[0]) == -1) {
		errsv = errno;
		goto error_nonblock;
	}

	if (io_fd_set_nonblock(impl->fds[1]) == -1) {
		errsv = errno;
		goto error_nonblock;
	}
#endif // !__linux__

	// clang-format off
	if (io_poll_watch(impl->poll, impl->fds[0], IO_EVENT_IN, &impl->watch)
			== -1) {
		// clang-format on
		errsv = errno;
		goto error_poll_watch;
	}

	return 0;

error_poll_watch:
#ifndef __linux__
error_nonblock:
error_cloexec:
#endif
error_sig_atomic:
	close(impl->fds[1]);
	close(impl->fds[0]);
	impl->fds[0] = impl->fds[1] = -1;
error_pipe:
error_close:
	errno = errsv;
	return -1;
}

static int
io_sigset_impl_close(struct io_sigset_impl *impl)
{
	assert(impl);

	int fds[2] = { impl->fds[0], impl->fds[1] };
	if (fds[0] == -1)
		return 0;
	impl->fds[0] = impl->fds[1] = -1;

	int result = 0;
	int errsv = errno;

	if (!impl->shutdown
			&& io_poll_watch(impl->poll, fds[0], 0, &impl->watch)
					== -1
			&& !result) {
		errsv = errno;
		result = -1;
	}

	if (close(fds[1]) == -1 && !result) {
		errsv = errno;
		result = -1;
	}

	if (close(fds[0]) == -1 && !result) {
		errsv = errno;
		result = -1;
	}

	errno = errsv;
	return result;
}

static int
io_sigset_impl_do_insert(struct io_sigset_impl *impl, int signo)
{
	assert(impl);
	assert(signo > 0);
	assert(signo < LELY_NSIG);

	struct io_sigset_node *node = &impl->nodes[signo - 1];
	assert(node->signo == signo);

	if (node->watched)
		return 0;

	if (!io_sigset_shared.sig[signo - 1].list) {
		assert(!io_sigset_shared.sig[signo - 1].fd);
		io_sigset_shared.sig[signo - 1].fd = impl->fds[1] + 1;

		struct sigaction act;
		memset(&act, 0, sizeof(act));
		act.sa_handler = &io_sigset_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_RESTART;

		if (sigaction(signo, &act, &io_sigset_action[signo - 1])
				== -1) {
			io_sigset_shared.sig[signo - 1].fd = 0;
			return -1;
		}
	}

	node->next = io_sigset_shared.sig[signo - 1].list;
	io_sigset_shared.sig[signo - 1].list = node;

	node->watched = 1;
	assert(!node->pending);

	return 0;
}

static int
io_sigset_impl_do_remove(struct io_sigset_impl *impl, int signo)
{
	assert(impl);
	assert(signo > 0);
	assert(signo < LELY_NSIG);

	struct io_sigset_node *node = &impl->nodes[signo - 1];
	assert(node->signo == signo);

	if (!node->watched)
		return 0;

	struct io_sigset_node **pnode = &io_sigset_shared.sig[signo - 1].list;
	assert(*pnode);
	while (*pnode != node)
		pnode = &(*pnode)->next;
	assert(*pnode == node);
	*pnode = node->next;
	node->next = NULL;

	node->watched = 0;
	node->pending = 0;

	int result = 0;

	if (!*pnode) {
		assert(io_sigset_shared.sig[signo - 1].fd == impl->fds[1] + 1);
		if (pnode == &io_sigset_shared.sig[signo - 1].list) {
			result = sigaction(signo, &io_sigset_action[signo - 1],
					NULL);

			io_sigset_shared.sig[signo - 1].fd = 0;
		} else {
			node = structof(pnode, struct io_sigset_node, next);
			impl = structof(node, struct io_sigset_impl,
					nodes[signo - 1]);
			io_sigset_shared.sig[signo - 1].fd = impl->fds[1] + 1;
			io_sigset_kill(signo);
		}
	}

	return result;
}

static size_t
io_sigset_impl_do_abort_tasks(struct io_sigset_impl *impl)
{
	assert(impl);

	size_t n = 0;

	// Try to abort io_sigset_impl_read_task_func().
	// clang-format off
	if (impl->read_posted && ev_exec_abort(impl->read_task.exec,
			&impl->read_task)) {
		// clang-format on
		impl->read_posted = 0;
		n++;
	}

	// Try to abort io_sigset_impl_wait_task_func().
	// clang-format off
	if (impl->wait_posted && ev_exec_abort(impl->wait_task.exec,
			&impl->wait_task)) {
		// clang-format on
		impl->wait_posted = 0;
		n++;
	}

	return n;
}

#endif // !LELY_NO_STDIO && _POSIX_C_SOURCE >= 200112L
