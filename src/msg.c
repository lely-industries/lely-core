/*!\file
 * This file is part of the CAN library; it contains the implementation of the
 * CAN frame functions.
 *
 * \see lely/can/msg.h
 *
 * \copyright 2016 Lely Industries N.V.
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

#include "can.h"
#include <lely/libc/stdio.h>
#include <lely/can/msg.h>

#include <errno.h>
// Include inttypes.h after stdio.h to enforce declarations of format specifiers
// in Newlib.
#include <inttypes.h>
#include <stdlib.h>

LELY_CAN_EXPORT int
snprintf_can_msg(char *s, size_t n, const struct can_msg *msg)
{
	if (!s)
		n = 0;

	if (__unlikely(!msg))
		return 0;

	uint8_t len = msg->len;
#ifndef LELY_NO_CANFD
	if (msg->flags & CAN_FLAG_EDL)
		len = MIN(len, CANFD_MAX_LEN);
	else
#endif
		len = MIN(len, CAN_MAX_LEN);

	int r, t = 0;

	if (msg->flags & CAN_FLAG_IDE)
		r = snprintf(s, n, "%08" PRIX32, msg->id & CAN_MASK_EID);
	else
		r = snprintf(s, n, "%03" PRIX32, msg->id & CAN_MASK_BID);
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, "   [%d] ", len);
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	if (msg->flags & CAN_FLAG_RTR) {
		r = snprintf(s, n, " remote request");
		if (__unlikely(r < 0))
			return r;
		t += r;
	} else {
		for (uint8_t i = 0; i < len; i++) {
			int r = snprintf(s, n, " %02X", msg->data[i]);
			if (__unlikely(r < 0))
				return r;
			t += r; r = MIN((size_t)r, n); s += r; n -= r;
		}
	}

	return t;
}

LELY_CAN_EXPORT int
asprintf_can_msg(char **ps, const struct can_msg *msg)
{
	int n = snprintf_can_msg(NULL, 0, msg);
	if (__unlikely(n < 0))
		return n;

	char *s = malloc(n + 1);
	if (__unlikely(!s))
		return -1;

	n = snprintf_can_msg(s, n + 1, msg);
	if (__unlikely(n < 0)) {
		int errsv = errno;
		free(s);
		errno = errsv;
		return n;
	}

	*ps = s;
	return n;
}

