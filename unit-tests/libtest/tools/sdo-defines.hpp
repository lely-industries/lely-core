/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2021 N7 Space Sp. z o.o.
 *
 * Unit Test Suite was developed under a programme of,
 * and funded by, the European Space Agency.
 *
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

#ifndef LELY_UNIT_TEST_SDO_DEFINES_HPP_
#define LELY_UNIT_TEST_SDO_DEFINES_HPP_

// values described in CiA 301, section 7.2.4.3 SDO protocols
#define CO_SDO_CCS_DN_SEG_REQ 0x00u
#define CO_SDO_CCS_DN_INI_REQ 0x20u
#define CO_SDO_CCS_UP_INI_REQ 0x40u
#define CO_SDO_CCS_UP_SEG_REQ 0x60u
#define CO_SDO_CCS_BLK_UP_REQ 0xa0u
#define CO_SDO_CCS_BLK_DN_REQ 0xc0u

#define CO_SDO_CS_ABORT 0x80u

#define CO_SDO_SCS_DN_SEG_RES 0x20u
#define CO_SDO_SCS_UP_INI_RES 0x40u
#define CO_SDO_SCS_DN_INI_RES 0x60u
#define CO_SDO_SCS_BLK_DN_RES 0xa0u
#define CO_SDO_SCS_BLK_UP_RES 0xc0u

#define CO_SDO_MAX_SEQNO 127u

#define CO_SDO_SC_END_BLK 0x01
#define CO_SDO_SC_BLK_RES 0x02
#define CO_SDO_SC_START_UP 0x03

#define CO_SDO_SEG_LAST 0x01u
#define CO_SDO_SEG_TOGGLE 0x10u
#define CO_SDO_SEG_SIZE_SET(n) (((7 - (n)) << 1) & CO_SDO_SEG_SIZE_MASK)
#define CO_SDO_SEQ_LAST 0x80u
#define CO_SDO_SEG_SIZE_MASK 0x0eu
#define CO_SDO_SC_MASK 0x03u

#define CO_SDO_BLK_CRC 0x04u
#define CO_SDO_BLK_SIZE_IND 0x02u
#define CO_SDO_BLK_SIZE_MASK 0x1cu
#define CO_SDO_BLK_SIZE_SET(n) (((7 - (n)) << 2) & CO_SDO_BLK_SIZE_MASK)
#define CO_SDO_SC_INI_BLK 0x00u

#define CO_SDO_INI_SIZE_IND 0x01u
#define CO_SDO_INI_SIZE_EXP 0x02u
#define CO_SDO_INI_SIZE_MASK 0x0fu
#define CO_SDO_INI_SIZE_EXP_SET(n) \
  ((((4 - n) << 2) | 0x03u) & CO_SDO_INI_SIZE_MASK)

#define CO_SDO_MAX_SEQNO 127u

#define CO_SDO_MSG_SIZE 8u
#define CO_SDO_INI_DATA_SIZE 4u

#define CHECK_SDO_CAN_MSG_CMD(res, msg) CHECK_EQUAL((res), (msg)[0])
#define CHECK_SDO_CAN_MSG_IDX(idx, msg) CHECK_EQUAL((idx), ldle_u16((msg) + 1u))
#define CHECK_SDO_CAN_MSG_SUBIDX(subidx, msg) CHECK_EQUAL((subidx), (msg)[3u])
#define CHECK_SDO_CAN_MSG_AC(ac, msg) CHECK_EQUAL((ac), ldle_u32((msg) + 4u))
#define CHECK_SDO_CAN_MSG_VAL(val, msg) CHECK_EQUAL((val), ldle_u32((msg) + 4u))

#endif  // LELY_UNIT_TEST_SDO_DEFINES_HPP_
