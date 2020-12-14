/**@file
 * This file is part of the utilities library; it exposes the doubly-linked list
 * functions.
 *
 * @see lely/util/dllist.h
 *
 * @copyright 2013-2020 Lely Industries N.V.
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

#include "util.h"
#define LELY_UTIL_DLLIST_INLINE extern inline
#include <lely/util/dllist.h>

#include <assert.h>

int
dllist_contains(const struct dllist *list, const struct dlnode *node)
{
	assert(list);

	if (!node)
		return 0;

	dllist_foreach (list, node_) {
		if (node_ == node)
			return 1;
	}

	return 0;
}
