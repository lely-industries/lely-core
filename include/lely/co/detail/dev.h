/**@file
 * This is the internal header file of the device description.
 *
 * @see lely/co/dev.h
 *
 * @copyright 2021 Lely Industries N.V.
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

#ifndef LELY_CO_DETAIL_DEV_H_
#define LELY_CO_DETAIL_DEV_H_

#include <lely/co/dev.h>
#include <lely/util/rbtree.h>

/// A CANopen device.
struct __co_dev {
	/// The network-ID.
	co_unsigned8_t netid;
	/// The node-ID.
	co_unsigned8_t id;
	/// The tree containing the object dictionary.
	struct rbtree tree;
#if !LELY_NO_CO_OBJ_NAME
	/// A pointer to the name of the device.
	char *name;
	/// A pointer to the vendor name.
	char *vendor_name;
#endif
	/// The vendor ID.
	co_unsigned32_t vendor_id;
#if !LELY_NO_CO_OBJ_NAME
	/// A pointer to the product name.
	char *product_name;
#endif
	/// The product code.
	co_unsigned32_t product_code;
	/// The revision number.
	co_unsigned32_t revision;
#if !LELY_NO_CO_OBJ_NAME
	/// A pointer to the order code.
	char *order_code;
#endif
	/// The supported bit rates.
	unsigned baud : 10;
	/// The (pending) baudrate (in kbit/s).
	co_unsigned16_t rate;
	/// A flag specifying whether LSS is supported (1) or not (0).
	int lss;
	/// The data types supported for mapping dummy entries in PDOs.
	co_unsigned32_t dummy;
#if !LELY_NO_CO_TPDO
	/// A pointer to the Transmit-PDO event indication function.
	co_dev_tpdo_event_ind_t *tpdo_event_ind;
	/// A pointer to user-specified data for #tpdo_event_ind.
	void *tpdo_event_data;
#if !LELY_NO_CO_MPDO
	/// A pointer to the SAM-MPDO event indication function.
	co_dev_sam_mpdo_event_ind_t *sam_mpdo_event_ind;
	/// A pointer to user-specified data for #sam_mpdo_event_ind.
	void *sam_mpdo_event_data;
#endif
#endif // !LELY_NO_CO_TPDO
};

#endif // LELY_CO_DETAIL_DEV_H_
