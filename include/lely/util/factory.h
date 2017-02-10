/*!\file
 * This header file is part of the utilities library; it contains the factory
 * pattern declarations.
 *
 * \copyright 2017 Lely Industries N.V.
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

#ifndef LELY_UTIL_FACTORY_H
#define LELY_UTIL_FACTORY_H

#include <lely/util/util.h>

#ifdef __cplusplus
extern "C" {
#endif

//! The type of a default constructor function.
typedef void *factory_ctor_t(void);

//! THe type of a destructor function.
typedef void factory_dtor_t(void *ptr);

/*!
 * Registers a constructor and destructor function for the specified type name,
 * replacing any previous registration with the same name.
 *
 * \returns the number of bytes read, or -1 on error. In the latter case, the
 * error number can be obtained with `get_errnum()`.
 *
 * \see factory_remove(), factory_find_ctor(), factory_find_dtor().
 */
LELY_UTIL_EXTERN int factory_insert(const char *name, factory_ctor_t *ctor,
		factory_dtor_t *dtor);

/*!
 * Unregisters the constructor and destructor function for the specified type
 * name.
 *
 * \see factory_insert()
 */
LELY_UTIL_EXTERN void factory_remove(const char *name);

/*!
 * Returns a pointer to the constructor function for the specified type name, or
 * NULL if not found.
 *
 * \see factory_insert()
 */
LELY_UTIL_EXTERN factory_ctor_t *factory_find_ctor(const char *name);

/*!
 * Returns a pointer to the destructor function for the specified type name, or
 * NULL if not found.
 *
 * \see factory_insert()
 */
LELY_UTIL_EXTERN factory_dtor_t *factory_find_dtor(const char *name);

#ifdef __cplusplus
}
#endif

#endif

