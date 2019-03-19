/**@file
 * This header file is part of the I/O library; it contains the I/O polling
 * declarations for POSIX platforms.
 *
 * @copyright 2015-2019 Lely Industries N.V.
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

#ifndef LELY_IO2_POSIX_POLL_H_
#define LELY_IO2_POSIX_POLL_H_

#include <lely/ev/poll.h>
#include <lely/io2/ctx.h>
#include <lely/io2/event.h>
#include <lely/io2/sys/io.h>
#include <lely/util/rbtree.h>

struct io_poll_watch;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of function invoked by an I/O polling instance (through
 * ev_poll_wait()) to report I/O events. Only the first event to occur is
 * reported. To receive subsequent I/O events, the file descriptor must be
 * reregistered with io_poll_watch().
 *
 * @param watch  a pointer to an object representing the file descriptor for
 *               which the I/O events are reported.
 * @param events the reported I/O events (a combination of #IO_EVENT_IN,
 *               #IO_EVENT_PRI, #IO_EVENT_OUT, #IO_EVENT_ERR and #IO_EVENT_HUP).
 */
typedef void io_poll_watch_func_t(struct io_poll_watch *watch, int events);

/**
 * An object representing a file descriptor being monitored for I/O events.
 * Additional data can be associated with an I/O event monitor by embedding it
 * in a struct and using structof() from the #io_poll_watch_func_t callback
 * fuction to obtain a pointer to the struct.
 */
struct io_poll_watch {
	/// A pointer to the function to be invoked when an I/O event occurs.
	io_poll_watch_func_t *func;
	int _fd;
	struct rbnode _node;
	int _events;
};

/// The static initializer for #io_poll_watch.
#define IO_POLL_WATCH_INIT(func) \
	{ \
		(func), -1, RBNODE_INIT, 0 \
	}

void *io_poll_alloc(void);
void io_poll_free(void *ptr);
io_poll_t *io_poll_init(io_poll_t *poll, io_ctx_t *ctx, int signo);
void io_poll_fini(io_poll_t *poll);

/**
 * Creates a new I/O polling instance.
 *
 * @param ctx   a pointer to the I/O context with which the polling instance
 *              should be registered.
 * @param signo the signal number used to wake up threads waiting on
 *              io_poll_watch() with ev_poll_kill(). If <b>signo</b> is 0, the
 *              default value SIGUSR1 is used.
 *
 * @returns a pointer to the new polling instance, or NULL on error. In the
 * latter case, the error number can be obtained from `errno`.
 */
io_poll_t *io_poll_create(io_ctx_t *ctx, int signo);

/// Destroys an I/O polling instance. @see io_poll_create()
void io_poll_destroy(io_poll_t *poll);

/**
 * Returns a pointer to the I/O context with which the I/O polling instance is
 * registered.
 */
io_ctx_t *io_poll_get_ctx(const io_poll_t *poll);

/**
 * Returns a pointer to the #ev_poll_t instance corresponding to the I/O polling
 * instance.
 */
ev_poll_t *io_poll_get_poll(const io_poll_t *poll);

/**
 * Registers a file descriptor with an I/O polling instance and monitors it for
 * I/O events (through ev_poll_wait()).
 *
 * @param poll   a pointer to an I/O polling instance.
 * @param fd     the file descriptor to be monitored.
 * @param events the I/O events to monitor (any combination of #IO_EVENT_IN,
 *               #IO_EVENT_PRI, #IO_EVENT_OUT, #IO_EVENT_ERR and #IO_EVENT_HUP).
 *               If <b>events</b> is non-zero the file descriptor is
 *               (re)registered and monitored for the specified events. Note
 *               that error and disconnect events are monitored regardless of
 *               whether #IO_EVENT_ERR and #IO_EVENT_HUP are specified. If
 *               <b>events</b> is 0, the file descriptor is unregistered.
 * @param watch  a pointer to an I/O event monitor. If <b>fd</b> has already
 *               been registered with a different #io_poll_watch object, an
 * error is returned and `errno` is set to `EEXIST`.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained from `errno`.
 */
int io_poll_watch(io_poll_t *poll, int fd, int events,
		struct io_poll_watch *watch);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_POSIX_POLL_H_
