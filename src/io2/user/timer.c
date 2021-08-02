/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * user-defined timer.
 *
 * @see lely/io2/user/timer.h
 *
 * @copyright 2015-2021 Lely Industries N.V.
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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or useried.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../timer.h"

#if !LELY_NO_MALLOC

#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/io2/ctx.h>
#include <lely/io2/user/timer.h>
#include <lely/util/errnum.h>
#include <lely/util/time.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

static io_ctx_t *io_user_timer_dev_get_ctx(const io_dev_t *dev);
static ev_exec_t *io_user_timer_dev_get_exec(const io_dev_t *dev);
static size_t io_user_timer_dev_cancel(io_dev_t *dev, struct ev_task *task);
static size_t io_user_timer_dev_abort(io_dev_t *dev, struct ev_task *task);

// clang-format off
static const struct io_dev_vtbl io_user_timer_dev_vtbl = {
	&io_user_timer_dev_get_ctx,
	&io_user_timer_dev_get_exec,
	&io_user_timer_dev_cancel,
	&io_user_timer_dev_abort
};
// clang-format on

static io_dev_t *io_user_timer_get_dev(const io_timer_t *timer);
static io_clock_t *io_user_timer_get_clock(const io_timer_t *timer);
static int io_user_timer_getoverrun(const io_timer_t *timer);
static int io_user_timer_gettime(
		const io_timer_t *timer, struct itimerspec *value);
static int io_user_timer_settime(io_timer_t *timer, int flags,
		const struct itimerspec *value, struct itimerspec *ovalue);
static void io_user_timer_submit_wait(
		io_timer_t *timer, struct io_timer_wait *wait);

// clang-format off
static const struct io_timer_vtbl io_user_timer_vtbl = {
	&io_user_timer_get_dev,
	&io_user_timer_get_clock,
	&io_user_timer_getoverrun,
	&io_user_timer_gettime,
	&io_user_timer_settime,
	&io_user_timer_submit_wait
};
// clang-format on

static int io_user_timer_clock_getres(
		const io_clock_t *clock, struct timespec *res);
static int io_user_timer_clock_gettime(
		const io_clock_t *clock, struct timespec *tp);
static int io_user_timer_clock_settime(
		io_clock_t *clock, const struct timespec *tp);

// clang-format off
static const struct io_clock_vtbl io_user_timer_clock_vtbl = {
	&io_user_timer_clock_getres,
	&io_user_timer_clock_gettime,
	&io_user_timer_clock_settime
};
// clang-format on

static void io_user_timer_svc_shutdown(struct io_svc *svc);

// clang-format off
static const struct io_svc_vtbl io_user_timer_svc_vtbl = {
	NULL,
	&io_user_timer_svc_shutdown
};
// clang-format on

struct io_user_timer {
	const struct io_dev_vtbl *dev_vptr;
	const struct io_timer_vtbl *timer_vptr;
	const struct io_clock_vtbl *clock_vptr;
	struct io_svc svc;
	io_ctx_t *ctx;
	ev_exec_t *exec;
	io_user_timer_setnext_t *func;
	void *arg;
#if !LELY_NO_THREADS
	mtx_t mtx;
#endif
	int shutdown;
	struct sllist queue;
	struct timespec now;
	struct timespec prev;
	struct timespec next;
	struct itimerspec value;
	int overrun;
};

static inline struct io_user_timer *io_user_timer_from_dev(const io_dev_t *dev);
static inline struct io_user_timer *io_user_timer_from_timer(
		const io_timer_t *timer);
static inline struct io_user_timer *io_user_timer_from_clock(
		const io_clock_t *clock);
static inline struct io_user_timer *io_user_timer_from_svc(
		const struct io_svc *svc);

static void io_user_timer_pop(struct io_user_timer *user, struct sllist *queue,
		struct ev_task *task);

static void io_user_timer_do_settime(struct io_user_timer *user);

static inline int timespec_valid(const struct timespec *tp);

void *
io_user_timer_alloc(void)
{
	struct io_user_timer *user = malloc(sizeof(*user));
	if (!user) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return NULL;
	}
	// Suppress a GCC maybe-uninitialized warning.
	user->timer_vptr = NULL;
	// cppcheck-suppress memleak symbolName=user
	return &user->timer_vptr;
}

void
io_user_timer_free(void *ptr)
{
	if (ptr)
		free(io_user_timer_from_timer(ptr));
}

io_timer_t *
io_user_timer_init(io_timer_t *timer, io_ctx_t *ctx, ev_exec_t *exec,
		io_user_timer_setnext_t *func, void *arg)
{
	struct io_user_timer *user = io_user_timer_from_timer(timer);
	assert(ctx);
	assert(exec);

	user->dev_vptr = &io_user_timer_dev_vtbl;
	user->timer_vptr = &io_user_timer_vtbl;
	user->clock_vptr = &io_user_timer_clock_vtbl;

	user->svc = (struct io_svc)IO_SVC_INIT(&io_user_timer_svc_vtbl);
	user->ctx = ctx;

	user->exec = exec;

	user->func = func;
	user->arg = arg;

#if !LELY_NO_THREADS
	if (mtx_init(&user->mtx, mtx_plain) != thrd_success)
		return NULL;
#endif

	user->shutdown = 0;

	sllist_init(&user->queue);

	user->now = (struct timespec){ 0, 0 };
	user->prev = (struct timespec){ 0, 0 };
	user->next = (struct timespec){ 0, 0 };
	user->value = (struct itimerspec){ { 0, 0 }, { 0, 0 } };
	user->overrun = 0;

	io_ctx_insert(user->ctx, &user->svc);

	return timer;
}

void
io_user_timer_fini(io_timer_t *timer)
{
	struct io_user_timer *user = io_user_timer_from_timer(timer);

	io_ctx_remove(user->ctx, &user->svc);
	// Cancel all pending tasks.
	io_user_timer_svc_shutdown(&user->svc);

#if !LELY_NO_THREADS
	mtx_destroy(&user->mtx);
#endif
}

io_timer_t *
io_user_timer_create(io_ctx_t *ctx, ev_exec_t *exec,
		io_user_timer_setnext_t *func, void *arg)
{
	int errc = 0;

	io_timer_t *timer = io_user_timer_alloc();
	if (!timer) {
		errc = get_errc();
		goto error_alloc;
	}

	io_timer_t *tmp = io_user_timer_init(timer, ctx, exec, func, arg);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	timer = tmp;

	return timer;

error_init:
	io_user_timer_free((void *)timer);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
io_user_timer_destroy(io_timer_t *timer)
{
	if (timer) {
		io_user_timer_fini(timer);
		io_user_timer_free((void *)timer);
	}
}

static io_ctx_t *
io_user_timer_dev_get_ctx(const io_dev_t *dev)
{
	const struct io_user_timer *user = io_user_timer_from_dev(dev);

	return user->ctx;
}

static ev_exec_t *
io_user_timer_dev_get_exec(const io_dev_t *dev)
{
	const struct io_user_timer *user = io_user_timer_from_dev(dev);

	return user->exec;
}

static size_t
io_user_timer_dev_cancel(io_dev_t *dev, struct ev_task *task)
{
	struct io_user_timer *user = io_user_timer_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

	io_user_timer_pop(user, &queue, task);

	return io_timer_wait_queue_post(&queue, -1, errnum2c(ERRNUM_CANCELED));
}

static size_t
io_user_timer_dev_abort(io_dev_t *dev, struct ev_task *task)
{
	struct io_user_timer *user = io_user_timer_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

	io_user_timer_pop(user, &queue, task);

	return ev_task_queue_abort(&queue);
}

static io_dev_t *
io_user_timer_get_dev(const io_timer_t *timer)
{
	const struct io_user_timer *user = io_user_timer_from_timer(timer);

	return &user->dev_vptr;
}

static io_clock_t *
io_user_timer_get_clock(const io_timer_t *timer)
{
	const struct io_user_timer *user = io_user_timer_from_timer(timer);

	return &user->clock_vptr;
}

static int
io_user_timer_getoverrun(const io_timer_t *timer)
{
	const struct io_user_timer *user = io_user_timer_from_timer(timer);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&user->mtx);
#endif
	int overrun = user->overrun;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&user->mtx);
#endif
	return overrun;
}

static int
io_user_timer_gettime(const io_timer_t *timer, struct itimerspec *value)
{
	const struct io_user_timer *user = io_user_timer_from_timer(timer);

	if (value) {
#if !LELY_NO_THREADS
		mtx_lock((mtx_t *)&user->mtx);
#endif
		*value = user->value;
		if (value->it_value.tv_sec || value->it_value.tv_nsec)
			timespec_sub(&value->it_value, &user->now);
#if !LELY_NO_THREADS
		mtx_unlock((mtx_t *)&user->mtx);
#endif
	}
	return 0;
}

static int
io_user_timer_settime(io_timer_t *timer, int flags,
		const struct itimerspec *value, struct itimerspec *ovalue)
{
	struct io_user_timer *user = io_user_timer_from_timer(timer);
	assert(value);

	struct timespec expiry = value->it_value;
	if (!timespec_valid(&value->it_interval) || !timespec_valid(&expiry)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

#if !LELY_NO_THREADS
	// NOTE: mtx_unlock() is called by io_user_timer_do_settime().
	mtx_lock(&user->mtx);
#endif

	if ((expiry.tv_sec || expiry.tv_nsec) && !(flags & TIMER_ABSTIME))
		timespec_add(&expiry, &user->now);

	if (ovalue) {
		*ovalue = user->value;
		if (ovalue->it_value.tv_sec || ovalue->it_value.tv_nsec)
			timespec_sub(&ovalue->it_value, &user->now);
	}
	user->value = (struct itimerspec){ value->it_interval, expiry };
	user->overrun = 0;

	io_user_timer_do_settime(user);
	return 0;
}

static void
io_user_timer_submit_wait(io_timer_t *timer, struct io_timer_wait *wait)
{
	struct io_user_timer *user = io_user_timer_from_timer(timer);
	assert(wait);
	struct ev_task *task = &wait->task;

	if (!task->exec)
		task->exec = user->exec;
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	if (user->shutdown) {
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		io_timer_wait_post(wait, -1, errnum2c(ERRNUM_CANCELED));
	} else {
		sllist_push_back(&user->queue, &task->_node);
		// NOTE: mtx_unlock() is called by io_user_timer_do_settime().
		io_user_timer_do_settime(user);
	}
}

static int
io_user_timer_clock_getres(const io_clock_t *clock, struct timespec *res)
{
	const struct io_user_timer *user = io_user_timer_from_clock(clock);

	if (res) {
#if !LELY_NO_THREADS
		mtx_lock((mtx_t *)&user->mtx);
#endif
		*res = user->now;
		timespec_sub(res, &user->prev);
#if !LELY_NO_THREADS
		mtx_unlock((mtx_t *)&user->mtx);
#endif
	}
	return 0;
}

static int
io_user_timer_clock_gettime(const io_clock_t *clock, struct timespec *tp)
{
	const struct io_user_timer *user = io_user_timer_from_clock(clock);

	if (tp) {
#if !LELY_NO_THREADS
		mtx_lock((mtx_t *)&user->mtx);
#endif
		*tp = user->now;
#if !LELY_NO_THREADS
		mtx_unlock((mtx_t *)&user->mtx);
#endif
	}
	return 0;
}

static int
io_user_timer_clock_settime(io_clock_t *clock, const struct timespec *tp)
{
	struct io_user_timer *user = io_user_timer_from_clock(clock);
	assert(tp);

#if !LELY_NO_THREADS
	// NOTE: mtx_unlock() is called by io_user_timer_do_settime().
	mtx_lock(&user->mtx);
#endif

	user->prev = user->now;
	user->now = *tp;

	io_user_timer_do_settime(user);
	return 0;
}

static void
io_user_timer_svc_shutdown(struct io_svc *svc)
{
	struct io_user_timer *user = io_user_timer_from_svc(svc);
	io_dev_t *dev = &user->dev_vptr;

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	int shutdown = !user->shutdown;
	user->shutdown = 1;
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif

	if (shutdown)
		// Cancel all pending tasks.
		io_user_timer_dev_cancel(dev, NULL);
}

static inline struct io_user_timer *
io_user_timer_from_dev(const io_dev_t *dev)
{
	assert(dev);

	return structof(dev, struct io_user_timer, dev_vptr);
}

static inline struct io_user_timer *
io_user_timer_from_timer(const io_timer_t *timer)
{
	assert(timer);

	return structof(timer, struct io_user_timer, timer_vptr);
}

static inline struct io_user_timer *
io_user_timer_from_clock(const io_clock_t *clock)
{
	assert(clock);

	return structof(clock, struct io_user_timer, clock_vptr);
}

static inline struct io_user_timer *
io_user_timer_from_svc(const struct io_svc *svc)
{
	assert(svc);

	return structof(svc, struct io_user_timer, svc);
}

static void
io_user_timer_pop(struct io_user_timer *user, struct sllist *queue,
		struct ev_task *task)
{
	assert(user);
	assert(queue);

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	if (!task)
		sllist_append(queue, &user->queue);
	else if (sllist_remove(&user->queue, &task->_node))
		sllist_push_back(queue, &task->_node);
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif
}

static void
io_user_timer_do_settime(struct io_user_timer *user)
{
	assert(user);

	struct sllist queue;
	sllist_init(&queue);

	struct timespec *expiry = &user->value.it_value;
	assert(timespec_valid(expiry));

	if ((expiry->tv_sec || expiry->tv_nsec)
			&& timespec_cmp(expiry, &user->now) <= 0) {
		const struct timespec *period = &user->value.it_interval;
		assert(timespec_valid(period));

		if (period->tv_sec || period->tv_nsec) {
			user->overrun = -1;
			while (timespec_cmp(expiry, &user->now) < 0) {
				timespec_add(expiry, period);
				user->overrun++;
			}
		} else {
			*expiry = (struct timespec){ 0, 0 };
			user->overrun = 0;
		}

		sllist_append(&queue, &user->queue);
	}

	struct timespec next = { 0, 0 };
	io_user_timer_setnext_t *func = NULL;
	void *arg = NULL;
	if (timespec_cmp(&user->next, expiry)) {
		next = user->next = *expiry;
		func = user->func;
		arg = user->arg;
	}

	int overrun = user->overrun;

#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif

	if (func)
		func(&next, arg);

	io_timer_wait_queue_post(&queue, overrun, 0);
}

static inline int
timespec_valid(const struct timespec *tp)
{
	return tp->tv_sec >= 0 && tp->tv_nsec >= 0 && tp->tv_nsec < 1000000000l;
}

#endif // !LELY_NO_MALLOC
