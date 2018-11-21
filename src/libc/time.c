/**@file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * @see lely/libc/time.h
 *
 * @copyright 2013-2018 Lely Industries N.V.
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

#include "libc.h"

#if !LELY_NO_RT

#include <lely/libc/time.h>

#if !LELY_HAVE_TIMESPEC_GET

int
timespec_get(struct timespec *ts, int base)
{
	if (base != TIME_UTC || clock_gettime(CLOCK_REALTIME, ts) == -1)
		return 0;
	return base;
}

#endif // !LELY_HAVE_TIMESPEC_GET

#endif // !LELY_NO_RT
