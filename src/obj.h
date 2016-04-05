/*!\file
 * This is the internal header file of the object dictionary.
 *
 * \see lely/co/obj.h
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

#ifndef LELY_CO_INTERN_OBJ_H
#define LELY_CO_INTERN_OBJ_H

#include "co.h"
#include <lely/util/rbtree.h>
#include <lely/co/val.h>

//! A CANopen object.
struct __co_obj {
	//! The node of this object in the tree of objects.
	struct rbnode node;
	//! A pointer to the CANopen device containing this object.
	co_dev_t *dev;
	//! The object index.
	co_unsigned16_t idx;
	//! The tree containing all the sub-objects.
	struct rbtree tree;
	//! A pointer to the name of the object.
	char *name;
	//! The object code.
	co_unsigned8_t code;
	//! A pointer to the object value.
	void *val;
	//! The size (in bytes) of the value at #val.
	size_t size;
};

//! A CANopen sub-object.
struct __co_sub {
	//! The node of this sub-object in the tree of sub-objects.
	struct rbnode node;
	//! A pointer to the CANopen object containing this sub-object.
	co_obj_t *obj;
	//! The object sub-index.
	co_unsigned8_t subidx;
	//! A pointer to the name of the sub-object.
	char *name;
	//! The data type.
	co_unsigned16_t type;
	//! The lower limit of the object value.
	union co_val min;
	//! The upper limit of the object value.
	union co_val max;
	//! The default value.
	union co_val def;
	//! A pointer to the sub-object value.
	void *val;
	//! The access type.
	unsigned access:5;
	//! A flag indicating if it is possible to map this object into a PDO.
	unsigned pdo_mapping:1;
	//! The object flags.
	unsigned flags:26;
	//! A pointer to the download indication function.
	co_sub_dn_ind_t *dn_ind;
	//! A pointer to user-specified data for #dn_ind.
	void *dn_data;
	//! A pointer to the upload indication function.
	co_sub_up_ind_t *up_ind;
	//! A pointer to user-specified data for #up_ind.
	void *up_data;
};

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Invokes the download indication function of a CANopen sub-object, registered
 * with co_sub_set_dn_ind(). This is used for writing values to the object
 * dictionary. If the indication function returns an error, or the
 * refuse-write-on-download flag (#CO_OBJ_FLAGS_WRITE) is set, the value of the
 * sub-object is left untouched.
 *
 * \param sub a pointer to a CANopen sub-object.
 * \param req a pointer to a CANopen SDO download request. All members of
 *            *\a req, except \a membuf, MUST be set by the caller. The
 *            \a membuf MUST be initialized before the first invocation and MUST
 *            only be used by the indication function.
 *
 * \returns 0 on success, or an SDO abort code on error.
 */
co_unsigned32_t co_sub_dn_ind(co_sub_t *sub, struct co_sdo_req *req);

/*!
 * Invokes the upload indication function of a CANopen sub-object, registered
 * with co_sub_set_up_ind(). This is used for reading values from the object
 * dictionary.
 *
 * \param sub a pointer to a CANopen sub-object.
 * \param req a pointer to a CANopen SDO upload request. The \a size member of
 *            *\a req MUST be set to 0 on the first invocation. All members MUST
 *            be initialized by the indication function.
 *
 * \returns 0 on success, or an SDO abort code on error.
 */
co_unsigned32_t co_sub_up_ind(const co_sub_t *sub, struct co_sdo_req *req);

#ifdef __cplusplus
}
#endif

#endif

