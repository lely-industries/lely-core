/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * CAN frame functions.
 *
 * @see lely/io2/can/msg.h
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

#include "../io2.h"
#include <lely/io2/can/msg.h>

#include <string.h>

int
can_msg_cmp(const void *p1, const void *p2)
{
	if (p1 == p2)
		return 0;

	if (!p1)
		return -1;
	if (!p2)
		return 1;

	const struct can_msg *m1 = p1;
	const struct can_msg *m2 = p2;

	int cmp;
	if ((cmp = (m2->id < m1->id) - (m1->id < m2->id)))
		return cmp;
	if ((cmp = (m2->flags < m1->flags) - (m1->flags < m2->flags)))
		return cmp;
	if ((cmp = (m2->len < m1->len) - (m1->len < m2->len)))
		return cmp;

	return memcmp(m1->data, m2->data, m1->len);
}
