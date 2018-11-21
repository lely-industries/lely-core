/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the CANopen type functions.
 *
 * @see lely/co/type.h
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

#include "co.h"
#include <lely/co/type.h>

int
co_type_is_basic(co_unsigned16_t type)
{
	switch (type) {
#define LELY_CO_DEFINE_TYPE(a, b, c, d) case CO_DEFTYPE_##a:
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE
		return 1;
	default: return 0;
	}
}

int
co_type_is_array(co_unsigned16_t type)
{
	switch (type) {
	case CO_DEFTYPE_VISIBLE_STRING:
	case CO_DEFTYPE_OCTET_STRING:
	case CO_DEFTYPE_UNICODE_STRING:
	case CO_DEFTYPE_DOMAIN: return 1;
	default: return 0;
	}
}

size_t
co_type_sizeof(co_unsigned16_t type)
{
	switch (type) {
#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	case CO_DEFTYPE_##a: \
		return sizeof(co_##b##_t);
#include <lely/co/def/type.def>
#undef LELY_CO_DEFINE_TYPE
	default: return 0;
	}
}

size_t
co_type_alignof(co_unsigned16_t type)
{
	switch (type) {
#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	case CO_DEFTYPE_##a: \
		return _Alignof(co_##b##_t);
#include <lely/co/def/type.def>
#undef LELY_CO_DEFINE_TYPE
	default: return 1;
	}
}
