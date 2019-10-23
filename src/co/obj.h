/**@file
 * This is the internal header file of the object dictionary.
 *
 * @see lely/co/obj.h
 *
 * @copyright 2019 Lely Industries N.V.
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

#ifndef LELY_CO_INTERN_OBJ_H_
#define LELY_CO_INTERN_OBJ_H_

#include "co.h"
#include <lely/co/obj.h>
#include <lely/co/val.h>
#include <lely/util/rbtree.h>

/// A CANopen object.
struct __co_obj {
	/// The node of this object in the tree of objects.
	struct rbnode node;
	/// A pointer to the CANopen device containing this object.
	co_dev_t *dev;
	/// The object index.
	co_unsigned16_t idx;
	/// The object code.
	co_unsigned8_t code;
#ifndef LELY_NO_CO_OBJ_NAME
	/// A pointer to the name of the object.
	char *name;
#endif
	/// The tree containing all the sub-objects.
	struct rbtree tree;
	/// A pointer to the object value.
	void *val;
	/// The size (in bytes) of the value at #val.
	size_t size;
};

/// A CANopen sub-object.
struct __co_sub {
	/// The node of this sub-object in the tree of sub-objects.
	struct rbnode node;
	/// A pointer to the CANopen object containing this sub-object.
	co_obj_t *obj;
	/// The object sub-index.
	co_unsigned8_t subidx;
	/// The data type.
	co_unsigned16_t type;
#ifndef LELY_NO_CO_OBJ_NAME
	/// A pointer to the name of the sub-object.
	char *name;
#endif
#ifndef LELY_NO_CO_OBJ_LIMITS
	/// The lower limit of the object value.
	union co_val min;
	/// The upper limit of the object value.
	union co_val max;
#endif
#ifndef LELY_NO_CO_OBJ_DEFAULT
	/// The default value.
	union co_val def;
#endif
	/// A pointer to the sub-object value.
	void *val;
	/// The access type.
	unsigned long access : 5;
	/// A flag indicating if it is possible to map this object into a PDO.
	unsigned long pdo_mapping : 1;
	/// The object flags.
	unsigned long flags : 26;
	/// A pointer to the download indication function.
	co_sub_dn_ind_t *dn_ind;
	/// A pointer to user-specified data for #dn_ind.
	void *dn_data;
#ifndef LELY_NO_CO_OBJ_UPLOAD
	/// A pointer to the upload indication function.
	co_sub_up_ind_t *up_ind;
	/// A pointer to user-specified data for #up_ind.
	void *up_data;
#endif
};

#endif // !LELY_CO_INTERN_OBJ_H_
