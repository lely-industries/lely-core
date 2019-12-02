/**@file
 * This header file is part of the CANopen library; it contains the Cyclic
 * Redundancy Check (CRC) declarations.
 *
 * @copyright 2016-2019 Lely Industries N.V.
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

#ifndef LELY_CO_CRC_H_
#define LELY_CO_CRC_H_

#include <lely/co/co.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Computes a CRC-16 checksum. This is an implementation of the CRC16-CCITT
 * specification based on the 0x1021 generator polynomial. It uses a table with
 * precomputed values for efficiency.
 *
 * As per section 7.2.4.3.16 in CiA 301 version 4.2.0, the CRC of "123456789"
 * (with an initial value of 0x0000) is 0x31c3.
 *
 * @param crc the initial value.
 * @param bp  a pointer to the bytes to be hashed.
 * @param n   the number of bytes to hash.
 *
 * @returns the updated CRC.
 */
uint_least16_t co_crc(uint_least16_t crc, const uint_least8_t *bp, size_t n);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_CRC_H_
