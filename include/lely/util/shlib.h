/*!\file
 * This header file is part of the utilities library; it contains the shared
 * library declarations.
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

#ifndef LELY_UTIL_SHLIB_H
#define LELY_UTIL_SHLIB_H

#include <lely/util/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
typedef int WINAPI shlib_func_t();
#else
typedef int shlib_func_t();
#endif

/*!
 * Opens a shared library and makes the symbols available with shlib_find_data()
 * and shlib_find_func().
 *
 * \param filename a pointer to the filename of the shared library. If the
 *                 filename is a full path, only that path is searched,
 *                 otherwise a platform-dependent search strategy is employed to
 *                 find the library. If \a filename is NULL or points to an
 *                 empty string, a handle to the currently running process is
 *                 returned.
 *
 * \returns a handle to the shared library, or NULL on error. In the latter
 * case, a diagnostic message will be printed with `diag()`.
 *
 * \see shlib_close()
 */
LELY_UTIL_EXTERN void *shlib_open(const char *filename);

/*!
 * Closes the handle to a shared library. If all handles to the library are
 * closed, its function and data object MAY be removed from the address space.
 *
 * \see shlib_open()
 */
LELY_UTIL_EXTERN void shlib_close(void *handle);

/*!
 * Retrieves the address of a data object in a shared library.
 *
 * \see shlib_find_func()
 */
LELY_UTIL_EXTERN void *shlib_find_data(void *handle, const char *name);

/*!
 * Retrieves the address of a function object in a shared library.
 *
 * \see shlib_find_data()
 */
LELY_UTIL_EXTERN shlib_func_t *shlib_find_func(void *handle, const char *name);

/*!
 * Returns a pointer to the platform-dependent prefix of shared library
 * filenames.
 *
 * \see shlib_suffix()
 */
LELY_UTIL_EXTERN const char *shlib_prefix(void);

/*!
 * Returns a pointer to the platform-dependent suffix (or extension) of shared
 * library filenames.
 *
 * \see shlib_prefix()
 */
LELY_UTIL_EXTERN const char *shlib_suffix(void);

#ifdef __cplusplus
}
#endif

#endif

