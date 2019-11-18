/**@file
 * This header file is part of the I/O library; it contains I/O polling
 * interface declarations.
 *
 * @copyright 2016-2018 Lely Industries N.V.
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

#ifndef LELY_IO_POLL_H_
#define LELY_IO_POLL_H_

#include <lely/io/io.h>

enum {
	/// An event representing the occurrence of a signal.
	IO_EVENT_SIGNAL = 0,
	/**
	 * An event signaling that an error has occurred for a file descriptor.
	 * Errors will always be reported by io_poll_wait(), even if not
	 * requested by io_poll_watch(). Note that the arrival of high-priority
	 * or out-of-band (OOB) data is considered an error.
	 */
	IO_EVENT_ERROR = 1 << 0,
	/**
	 * An event signaling that a file descriptor is ready for reading
	 * normal-priority (non-OOB) data.
	 */
	IO_EVENT_READ = 1 << 1,
	/**
	 * An event signaling that a file descriptor is ready for writing
	 * normal-priority (non-OOB) data.
	 */
	IO_EVENT_WRITE = 1 << 2
};

/// An I/O event.
struct io_event {
	/**
	 * The events that should be watched or have been triggered (either
	 * #IO_EVENT_SIGNAL, or any combination of #IO_EVENT_ERROR,
	 * #IO_EVENT_READ and #IO_EVENT_WRITE).
	 */
	int events;
	/// Signal attributes depending on the value of #events.
	union {
		/// The signal number (if #events == #IO_EVENT_SIGNAL).
		unsigned char sig;
		/**
		 * A pointer to user-specified data (if #events !=
		 * #IO_EVENT_SIGNAL).
		 */
		void *data;
		/// An I/O device handle (if #events != #IO_EVENT_SIGNAL).
		io_handle_t handle;
	} u;
};

/// The static initializer for struct #io_event.
#define IO_EVENT_INIT \
	{ \
		0, { 0 } \
	}

struct __io_poll;
#ifndef __cplusplus
/// An opaque I/O polling interface type.
typedef struct __io_poll io_poll_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

void *__io_poll_alloc(void);
void __io_poll_free(void *ptr);
struct __io_poll *__io_poll_init(struct __io_poll *poll);
void __io_poll_fini(struct __io_poll *poll);

/// Creates a new I/O polling interface. @see io_poll_destroy()
io_poll_t *io_poll_create(void);

/// Destroys an I/O polling interface. @see io_poll_create()
void io_poll_destroy(io_poll_t *poll);

/**
 * Registers an I/O device with an I/O polling interface and instructs it to
 * watch for certain events. This function is thread-safe and can be run
 * concurrently with io_poll_wait(). However, it is not guaranteed that an
 * ongoing wait will register the new events.
 *
 * @param poll   a pointer to an I/O polling interface.
 * @param handle a valid I/O device handle.
 * @param event  a pointer the events to watch. If <b>events</b> is NULL, the
 *               device is unregistered and removed from the polling interface.
 *               If not, a copy of *<b>event</b> is registered with the polling
 *               interface.
 * @param keep   a flag indicating whether to keep watching the file descriptor
 *               after an event occurs.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_poll_watch(io_poll_t *poll, io_handle_t handle, struct io_event *event,
		int keep);

/**
 * Waits at most <b>timeout</b> milliseconds for at most <b>maxevents</b> I/O
 * events to occur for any of the I/O devices registered with io_poll_watch().
 * Events that occur within <b>timeout</b> milliseconds are stored in
 * *<b>events</b>. This function is thread-safe, but only a single invocation
 * SHOULD run at the same time; otherwise the same event might be reported to
 * multiple threads.
 *
 * @param poll      a pointer to an I/O polling interface.
 * @param maxevents the maximum number of events to return (MUST be larger than
 *                  zero).
 * @param events    an array of at least <b>maxevents</b> #io_event structs. On
 *                  success, *<b>events</b> contains the events that occurred
 *                  within the timeout.
 * @param timeout   the maximum timeout value (in milliseconds). If
 *                  <b>timeout</b> is 0, this function will not wait. If
 *                  <b>timeout</b> is negative, this function will wait
 *                  indefinitely.
 *
 * @returns the number of events in *<b>events</b>, or -1 on error. In the
 * latter case, the error number can be obtained with get_errc(). This
 * function returns 0 if the timeout elapses without an event occurring.
 */
int io_poll_wait(io_poll_t *poll, int maxevents, struct io_event *events,
		int timeout);

/**
 * Generates a signal event. This function can be used to interrupt
 * io_poll_wait(). Note that it is safe to call this function from a signal
 * handler.
 *
 * @param poll a pointer to an I/O polling interface.
 * @param sig  the signal number.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_poll_signal(io_poll_t *poll, unsigned char sig);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_POLL_H_
