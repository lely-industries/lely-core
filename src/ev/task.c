/**@file
 * This file is part of the event library; it contains the implementation of the
 * task functions.
 *
 * @see lely/ev/task.h
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

#include "ev.h"
#include <lely/ev/task.h>
#include <lely/util/util.h>

struct ev_task *
ev_task_from_node(struct slnode *node)
{
	return node ? structof(node, struct ev_task, _node) : NULL;
}
