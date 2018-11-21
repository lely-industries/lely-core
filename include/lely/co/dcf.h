/**@file
 * This header file is part of the CANopen library; it contains the Electronic
 * Data Sheet (EDS) and Device Configuration File (DCF) function declarations.
 *
 * @copyright 2016-2018 Lely Industries N.V.
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

#ifndef LELY_CO_DCF_H_
#define LELY_CO_DCF_H_

#include <lely/co/dev.h>

// The file location struct from <lely/util/diag.h>.
struct floc;

#ifdef __cplusplus
extern "C" {
#endif

struct __co_dev *__co_dev_init_from_dcf_file(
		struct __co_dev *dev, const char *filename);

/**
 * Creates a CANopen device from an EDS or DCF file.
 *
 * @returns a pointer to a new CANopen device, or NULL on error.
 */
co_dev_t *co_dev_create_from_dcf_file(const char *filename);

struct __co_dev *__co_dev_init_from_dcf_text(struct __co_dev *dev,
		const char *begin, const char *end, struct floc *at);

/**
 * Creates a CANopen device from an EDS or DCF text string.
 *
 * @param begin a pointer to the first character in the string.
 * @param end   a pointer to one past the last character in the string (can be
 *              NULL if the string is null-terminated).
 * @param at    an optional pointer to the file location of <b>begin</b> (used
 *              for diagnostic purposes). On exit, if `at != NULL`, *<b>at</b>
 *              points to one past the last character parsed.
 *
 * @returns a pointer to a new CANopen device, or NULL on error.
 */
co_dev_t *co_dev_create_from_dcf_text(
		const char *begin, const char *end, struct floc *at);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_DCF_H_
