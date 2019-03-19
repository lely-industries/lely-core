/**@file
 * This header file is part of the event library; it contains the abstract
 * polling interface.
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

#ifndef LELY_EV_POLL_H_
#define LELY_EV_POLL_H_

#include <lely/ev/ev.h>

#ifndef LELY_EV_POLL_INLINE
#define LELY_EV_POLL_INLINE static inline
#endif

/// The abstract polling interface.
typedef const struct ev_poll_vtbl *const ev_poll_t;

#ifdef __cplusplus
extern "C" {
#endif

struct ev_poll_vtbl {
	void *(*self)(const ev_poll_t *poll);
	int (*wait)(ev_poll_t *poll, int timeout);
	int (*kill)(ev_poll_t *poll, void *thr);
};

/**
 * Returns the identifier of the calling thread. This identifier can be used to
 * interrupt a call to ev_poll_wait() from another thread with ev_poll_kill().
 */
LELY_EV_POLL_INLINE void *ev_poll_self(const ev_poll_t *poll);

/**
 * Waits for at most <b>timeout</b> milliseconds while polling for new events.
 * If <b>timeout</b> is 0, this function will not wait. If <b>timeout</b> is
 * negative, this function will wait indefinitely.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error code can
 * be obtained with get_errc().
 */
LELY_EV_POLL_INLINE int ev_poll_wait(ev_poll_t *poll, int timeout);

/**
 * Interrupts a polling wait on the specified thread.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error code can
 * be obtained with get_errc().
 */
LELY_EV_POLL_INLINE int ev_poll_kill(ev_poll_t *poll, void *thr);

inline void *
ev_poll_self(const ev_poll_t *poll)
{
	return (*poll)->self(poll);
}

inline int
ev_poll_wait(ev_poll_t *poll, int timeout)
{
	return (*poll)->wait(poll, timeout);
}

inline int
ev_poll_kill(ev_poll_t *poll, void *thr)
{
	return (*poll)->kill(poll, thr);
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_EV_POLL_H_
