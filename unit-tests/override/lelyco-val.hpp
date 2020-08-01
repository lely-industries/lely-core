/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2020 N7 Space Sp. z o.o.
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

#ifndef LELY_OVERRIDE_LELY_CO_VAL_H_
#define LELY_OVERRIDE_LELY_CO_VAL_H_

#include "override/lely-defs.hpp"

#ifdef HAVE_LELY_OVERRIDE

/* 0. Declare valid calls counter for the overridden function. */
namespace LelyOverride {
/**
 * Setups number of valid calls to co_val_read().
 */
void co_val_read(int valid_calls);

/**
 * Setups number of valid calls to co_val_write().
 */
void co_val_write(int valid_calls);

/**
 * Setups number of valid calls to co_val_make().
 */
void co_val_make(int valid_calls);

/**
 * Setups number of valid calls to co_val_init_min().
 */
void co_val_init_min(int valid_calls);

/**
 * Setups number of valid calls to co_val_init_max().
 */
void co_val_init_max(int valid_calls);

/**
 * Setups number of valid calls to co_val_init().
 */
void co_val_init(int valid_calls);

}  // namespace LelyOverride

#endif  // HAVE_LELY_OVERRIDE

#endif  // !LELY_OVERRIDE_LELY_CO_VAL_H_
