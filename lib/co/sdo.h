/**@file
 * This is the internal header file of the Service Data Object (SDO)
 * declarations.
 *
 * @see lely/co/sdo.h
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#ifndef LELY_CO_INTERN_SDO_H_
#define LELY_CO_INTERN_SDO_H_

#include "co.h"
#include <lely/co/sdo.h>

/// The mask to extract the command specifier (CS) from an SDO command byte.
#define CO_SDO_CS_MASK 0xe0

/// The SDO client/server command specifier 'abort transfer' request.
#define CO_SDO_CS_ABORT 0x80

/// The SDO server command specifier 'download initiate' response.
#define CO_SDO_SCS_DN_INI_RES 0x60

/// The SDO server command specifier 'download segment' response.
#define CO_SDO_SCS_DN_SEG_RES 0x20

/// The SDO server command specifier 'upload initiate' response.
#define CO_SDO_SCS_UP_INI_RES 0x40

/// The SDO server command specifier 'upload segment' response.
#define CO_SDO_SCS_UP_SEG_RES 0x00

/// The SDO server command specifier 'block download' response.
#define CO_SDO_SCS_BLK_DN_RES 0xa0

/// The SDO server command specifier 'block upload' response.
#define CO_SDO_SCS_BLK_UP_RES 0xc0

/// The SDO client command specifier 'download initiate' request.
#define CO_SDO_CCS_DN_INI_REQ 0x20

/// The SDO client command specifier 'download segment' request.
#define CO_SDO_CCS_DN_SEG_REQ 0x00

/// The SDO client command specifier 'upload initiate' request.
#define CO_SDO_CCS_UP_INI_REQ 0x40

/// The SDO client command specifier 'upload segment' request.
#define CO_SDO_CCS_UP_SEG_REQ 0x60

/// The SDO client command specifier 'block download' request.
#define CO_SDO_CCS_BLK_DN_REQ 0xc0

/// The SDO client command specifier 'block upload' request.
#define CO_SDO_CCS_BLK_UP_REQ 0xa0

/// The mask to extract the subcommand (SC) from an SDO command byte.
#define CO_SDO_SC_MASK 0x03

/// The SDO server/client subcommand 'initiate download/upload'.
#define CO_SDO_SC_INI_BLK 0x00

/// The SDO server/client subcommand 'end block download/upload'.
#define CO_SDO_SC_END_BLK 0x01

/// The SDO client/client subcommand 'block download/upload' response.
#define CO_SDO_SC_BLK_RES 0x02

/// The SDO client subcommand 'start upload'.
#define CO_SDO_SC_START_UP 0x03

/**
 * The mask to extract the size indicator from an SDO command byte specifying an
 * initiate download/upload request/response.
 */
#define CO_SDO_INI_SIZE_MASK 0x0f

/// The SDO size indicator flag indicating that the data set size is indicated.
#define CO_SDO_INI_SIZE_IND 0x01

/// The SDO size indicator flag indicating expedited transfer.
#define CO_SDO_INI_SIZE_EXP 0x02

/**
 * Retrieves the number of bytes containing expedited data from the SDO size
 * indicator.
 *
 * @see CO_SDO_INI_SIZE_EXP_SET()
 */
#define CO_SDO_INI_SIZE_EXP_GET(cs) (4 - (((cs)&CO_SDO_INI_SIZE_MASK) >> 2))

/**
 * Sets the SDO size indicator, indicating the expedited transfer of <b>n</b>
 * bytes (in the range [1..4]).
 *
 * @see CO_SDO_INI_SIZE_EXP_GET()
 */
#define CO_SDO_INI_SIZE_EXP_SET(n) \
	((((4 - (n)) << 2) | 0x03) & CO_SDO_INI_SIZE_MASK)

/// The mask to get/set the toggle bit from an SDO command byte.
#define CO_SDO_SEG_TOGGLE 0x10

/**
 * The mask to extract the size indicator from an SDO command byte specifying an
 * download/upload segment request/response.
 */
#define CO_SDO_SEG_SIZE_MASK 0x0e

/**
 * Retrieves the number of bytes containing segment data from the SDO size
 * indicator.
 *
 * @see CO_SDO_SEG_SIZE_SET()
 */
#define CO_SDO_SEG_SIZE_GET(cs) (7 - (((cs)&CO_SDO_SEG_SIZE_MASK) >> 1))

/**
 * Sets the SDO size indicator, indicating <b>n</b> bytes contain segment data
 * (in the range [0..7]).
 *
 * @see CO_SDO_SEG_SIZE_GET()
 */
#define CO_SDO_SEG_SIZE_SET(n) (((7 - (n)) << 1) & CO_SDO_SEG_SIZE_MASK)

/// The mask to get/set the last segment bit from an SDO command byte.
#define CO_SDO_SEG_LAST 0x01

/// The SDO size indicator flag indicating that the data set size is indicated.
#define CO_SDO_BLK_SIZE_IND 0x02

/// The SDO CRC support flag.
#define CO_SDO_BLK_CRC 0x04

/// The mask to get/set the last segment bit from an SDO command byte.
#define CO_SDO_SEQ_LAST 0x80

/**
 * The mask to extract the size indicator from an SDO command byte specifying a
 * block download/upload end request.
 */
#define CO_SDO_BLK_SIZE_MASK 0x1c

/**
 * Retrieves the number of bytes containing segment data from the SDO size
 * indicator.
 *
 * @see CO_SDO_BLK_SIZE_SET()
 */
#define CO_SDO_BLK_SIZE_GET(cs) (7 - (((cs)&CO_SDO_BLK_SIZE_MASK) >> 2))

/**
 * Sets the SDO size indicator, indicating <b>n</b> bytes contain segment data
 * (in the range [0..7]).
 *
 * @see CO_SDO_BLK_SIZE_GET()
 */
#define CO_SDO_BLK_SIZE_SET(n) (((7 - (n)) << 2) & CO_SDO_BLK_SIZE_MASK)

/// The maximum sequence number (or segments per block).
#define CO_SDO_MAX_SEQNO 127

#endif // !LELY_CO_INTERN_SDO_H_
