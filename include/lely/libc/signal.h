/*!\file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<signal.h>` and defines any missing functionality.
 *
 * \copyright 2017 Lely Industries N.V.
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

#ifndef LELY_LIBC_SIGNAL_H
#define LELY_LIBC_SIGNAL_H

#include <lely/libc/sys/types.h>

#include <signal.h>

#ifndef _POSIX_REALTIME_SIGNALS

//! No asynchronous notification is delivered when the event of interest occurs.
#define SIGEV_NONE	1

/*!
 * A queued signal, with an application-defined value, is generated when the
 * event of interest occurs (not supported with the native Windows API).
 */
#define SIGEV_SIGNAL	0

//! A notification function is called to perform notification.
#define SIGEV_THREAD	2

//! A signal value.
union sigval {
	//! The integer signal value.
	int sival_int;
	//! The pointer signal value.
	void *sival_ptr;
};

//! A struct specifying how a a signal event should be handles.
struct sigevent {
	/*!
	 * The notification type (one of #SIGEV_NONE, #SIGEV_SIGNAL or
	 * #SIGEV_THREAD).
	 */
	int sigev_notify;
	//! The signal number.
	int sigev_signo;
	//! The signal value.
	union sigval sigev_value;
	//! The notification function.
	void (__cdecl *sigev_notify_function)(union sigval);
	//! The notification attributes (ignored on Windows).
	pthread_attr_t *sigev_notify_attributes;
};

#endif // !_POSIX_REALTIME_SIGNALS

#endif

