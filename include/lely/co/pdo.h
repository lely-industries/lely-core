/*!\file
 * This header file is part of the CANopen library; it contains the Process Data
 * Object (PDO) declarations.
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

#ifndef LELY_CO_PDO_H
#define LELY_CO_PDO_H

#include <lely/co/type.h>

//! The bit in the PDO COB-ID specifying whether the PDO exists and is valid.
#define CO_PDO_COBID_VALID	UINT32_C(0x80000000)

//! The bit in the PDO COB-ID specifying whether RTR is allowed.
#define CO_PDO_COBID_RTR	UINT32_C(0x40000000)

/*!
 * The bit in the PDO COB-ID specifying whether to use an 11-bit (0) or 29-bit
 * (1) CAN-ID.
 */
#define CO_PDO_COBID_FRAME	UINT32_C(0x20000000)

//! The data type (and object index) of a PDO communication parameter record.
#define CO_DEFSTRUCT_PDO_COMM_PAR	0x0020

//! A PDO communication parameter record.
struct co_pdo_comm_par {
	//! Highest sub-index supported.
	co_unsigned8_t n;
	//! COB-ID.
	co_unsigned32_t cobid;
	//! Transmission type.
	co_unsigned8_t trans;
	//! Inhibit time.
	co_unsigned16_t inhibit;
	co_unsigned8_t __reserved;
	//! Event timer.
	co_unsigned16_t event;
	//! SYNC start value.
	co_unsigned8_t sync;
};

//! The static initializer from struct #co_pdo_comm_par.
#define CO_PDO_COMM_PAR_INIT	{ 6, CO_PDO_COBID_VALID, 0, 0, 0, 0, 0 }

//! The data type (and object index) of a PDO mapping parameter record.
#define CO_DEFSTRUCT_PDO_MAP_PAR	0x0021

//! A PDO mapping parameter record.
struct co_pdo_map_par {
	//! Number of mapped objects in PDO.
	co_unsigned8_t n;
	//! An array of objects to be mapped.
	co_unsigned32_t map[0x40];
};

//! The static initializer from struct #co_pdo_map_par.
#define CO_PDO_MAP_PAR_INIT \
	{ \
		0, \
		{ \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0 \
		} \
	}

#endif

