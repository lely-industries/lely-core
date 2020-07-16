/**@file
 * This header file is part of the utilities library; it contains the
 * configuration functions.
 *
 * @copyright 2014-2020 Lely Industries N.V.
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

#ifndef LELY_UTIL_CONFIG_H_
#define LELY_UTIL_CONFIG_H_

#include <lely/util/util.h>

#include <stddef.h>

struct __config;
#ifndef __cplusplus
/// An opaque configuration type.
typedef struct __config config_t;
#endif

enum {
	/// Section and key names are case-insensitive.
	CONFIG_CASE = 1 << 0
};

// The file location struct from <lely/util/diag.h>.
struct floc;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a function called by config_foreach() for each key in a
 * configuration struct.
 *
 * @param section a pointer to the name of the section
 * @param key     a pointer to the name of the key.
 * @param value   a pointer to the value of the key.
 * @param data    a pointer to user-specified data.
 */
typedef void config_foreach_func_t(const char *section, const char *key,
		const char *value, void *data);

void *__config_alloc(void);
void __config_free(void *ptr);
struct __config *__config_init(struct __config *config, int flags);
void __config_fini(struct __config *config);

/**
 * Creates a new configuration struct with an unnamed empty root section.
 *
 * @param flags either 0 or #CONFIG_CASE.
 *
 * @returns a pointer to a new configuration struct, or NULL on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see config_destroy()
 */
config_t *config_create(int flags);

/// Destroys a configuration struct. @see config_create()
void config_destroy(config_t *config);

/**
 * Retrieves a list of section names from a configuration struct.
 *
 * @param config   a pointer to a configuration struct.
 * @param n        the maximum number of sections to return.
 * @param sections an array of at least <b>n</b> pointers (can be NULL). On
 *                 success, *<b>sections</b> contains the pointers to the
 *                 section names.
 *
 * @returns the total number of sections in the configuration struct (which may
 * may be different from <b>n</b>).
 *
 * @see config_get_keys()
 */
size_t config_get_sections(
		const config_t *config, size_t n, const char **sections);

/**
 * Retrieves a list of key names from a section in a configuration struct.
 *
 * @param config  a pointer to a configuration struct.
 * @param section a pointer to the name of the section. If <b>section</b> is
 *                NULL or "", the root section is used instead.
 * @param n       the maximum number of sections to return.
 * @param keys    an array of at least <b>n</b> pointers (can be NULL). On
 *                success, *<b>keys</b> contains the pointers to the key names.
 *
 * @returns the total number of keys in the section (which may may be different
 * from <b>n</b>).
 *
 * @see config_get_sections()
 */
size_t config_get_keys(const config_t *config, const char *section, size_t n,
		const char **keys);

/**
 * Retrieves a key from a configuration struct.
 *
 * @param config  a pointer to a configuration struct.
 * @param section a pointer to the name of the section. If <b>section</b> is
 *                NULL or "", the root section is used instead.
 * @param key     a pointer to the name of the key.
 *
 * @returns a pointer to the value of the key, or NULL if not found.
 */
const char *config_get(
		const config_t *config, const char *section, const char *key);

/**
 * Sets a key in or removes a key from a configuration struct.
 *
 * @param config  a pointer to a configuration struct.
 * @param section a pointer to the name of the section. If <b>section</b> is
 *                NULL or "", the root section is used instead.
 * @param key     a pointer to the name of the key. This name is duplicated on
 *                success, so the caller need not guarantee the lifetime of the
 *                string.
 * @param value   a pointer to the new value of the key. If <b>value</b> is
 *                NULL, the key is removed. Like <b>key</b>, the string is
 *                duplicated on success.
 *
 * @returns a pointer to the value (which will differ from <b>value</b> due to
 * duplication), or NULL on error or when the key is deleted. In case of an
 * error, the error number can be obtained with get_errc().
 */
const char *config_set(config_t *config, const char *section, const char *key,
		const char *value);

/**
 * Invokes a function for each key in a configuration struct.
 *
 * @param config a pointer to a configuration struct.
 * @param func   a pointer to the function to be invoked.
 * @param data   a pointer to user-specified data (can be NULL). <b>data</b> is
 *               passed as the last parameter to <b>func</b>.
 */
void config_foreach(const config_t *config, config_foreach_func_t *func,
		void *data);

/**
 * Parses an INI file and adds the keys to a configuration struct.
 *
 * @returns the number of characters read, or 0 on error. I/O and parsing errors
 * are reported with diag_at().
 *
 * @see config_parse_ini_text()
 */
size_t config_parse_ini_file(config_t *config, const char *filename);

/**
 * Parses a string in INI-format and adds the keys to a configuration struct.
 *
 * @param config a pointer to a configuration struct.
 * @param begin  a pointer to the first character in the string.
 * @param end    a pointer to one past the last character in the string (can be
 *               NULL if the string is null-terminated).
 * @param at     an optional pointer to the file location of <b>begin</b> (used
 *               for diagnostic purposes). On exit, if `at != NULL`, *<b>at</b>
 *               points to one past the last character parsed.
 *
 * @returns the number of characters read. Parsing errors are reported with
 * diag() and diag_at(), respectively.
 *
 * @see config_parse_ini_file()
 */
size_t config_parse_ini_text(config_t *config, const char *begin,
		const char *end, struct floc *at);

/**
 * Prints a configuration struct to an INI file.
 *
 * @returns the number of characters written, or 0 on error. I/O errors are
 * reported with diag().
 *
 * @see config_print_ini_text()
 */
size_t config_print_ini_file(const config_t *config, const char *filename);

/**
 * Prints a configuration struct in INI-format to a memory buffer. Note that the
 * output is _not_ null-terminated.
 *
 * @param config a pointer to a configuration struct.
 * @param pbegin the address of a pointer to the start of the buffer. If
 *               <b>pbegin</b> or *<b>pbegin</b> is NULL, nothing is written;
 *               Otherwise, on exit, *<b>pbegin</b> points to one past the last
 *               character written.
 * @param end    a pointer to one past the last character in the buffer. If
 *               <b>end</b> is not NULL, at most `end - *pbegin` characters are
 *               written, and the output may be truncated.
 *
 * @returns the number of characters that would have been written had the buffer
 * been sufficiently large.
 *
 * @see config_print_ini_file()
 */
size_t config_print_ini_text(const config_t *config, char **pbegin, char *end);

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_CONFIG_H_
