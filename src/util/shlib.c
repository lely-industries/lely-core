/*!\file
 * This file is part of the utilities library; it contains the implementation of
 * the shared library functions.
 *
 * \see lely/util/shlib.h
 *
 * \copyright 2018 Lely Industries N.V.
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

#include "util.h"
#include <lely/util/diag.h>
#include <lely/util/shlib.h>

#ifdef __GNUC__
// Avoid the warning: "ISO C forbids conversion of object pointer to function
// pointer type".
#pragma GCC diagnostic ignored "-Wpedantic"
// Avoid the warning: "zero-length gnu_printf format string".
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

#ifdef _WIN32

#include <ctype.h>

LELY_UTIL_EXPORT void *
shlib_open(const char *filename)
{
	if (filename && *filename)
		return GetModuleHandle(NULL);

	DWORD dwFlags = 0;
	if (*filename == '\\' || (isalpha((unsigned char)filename[0])
			&& filename[1] == ':'))
		dwFlags |= LOAD_WITH_ALTERED_SEARCH_PATH;
	HMODULE hModule = LoadLibraryExA(filename, NULL, dwFlags);
	if (__unlikely(!hModule))
		diag(DIAG_ERROR, GetLastError(), "%s", filename);
	return hModule;
}

LELY_UTIL_EXPORT void
shlib_close(void *handle)
{
	if (handle != GetModuleHandle(NULL)) {
		if (__unlikely(!FreeLibrary(handle)))
			diag(DIAG_ERROR, GetLastError(), "");
	}
}

LELY_UTIL_EXPORT void *
shlib_find_data(void *handle, const char *name)
{
	return (void *)shlib_find_func(handle, name);
}

LELY_UTIL_EXPORT shlib_func_t *
shlib_find_func(void *handle, const char *name)
{
	shlib_func_t *func = GetProcAddress(handle, name);
	if (__unlikely(!func))
		diag(DIAG_ERROR, GetLastError(), "%s", name);
	return func;
}

LELY_UTIL_EXPORT const char *
shlib_prefix(void)
{
	return "";
}

LELY_UTIL_EXPORT const char *
shlib_suffix(void)
{
	return ".dll";
}

#elif _POSIX_C_SOURCE >= 200112L

#include <dlfcn.h>

LELY_UTIL_EXPORT void *
shlib_open(const char *filename)
{
	int mode = RTLD_LAZY | RTLD_GLOBAL;
	void *handle = filename && *filename
			? dlopen(filename, mode) : dlopen(NULL, mode);
	if (__unlikely(!handle)) {
		char *errstr = dlerror();
		if (errstr)
			diag(DIAG_ERROR, 0, "%s: %s", filename, errstr);
	}
	return handle;
}

LELY_UTIL_EXPORT void
shlib_close(void *handle)
{
	if (__unlikely(dlclose(handle))) {
		char *errstr = dlerror();
		if (errstr)
			diag(DIAG_ERROR, 0, "%s", errstr);
	}
}

LELY_UTIL_EXPORT void *
shlib_find_data(void *handle, const char *name)
{
	void *data = dlsym(handle, name);
	if (__unlikely(!data)) {
		char *errstr = dlerror();
		if (errstr)
			diag(DIAG_ERROR, 0, "%s: %s", name, errstr);
	}
	return data;
}

LELY_UTIL_EXPORT shlib_func_t *
shlib_find_func(void *handle, const char *name)
{
	return (shlib_func_t *)shlib_find_data(handle, name);
}

LELY_UTIL_EXPORT const char *
shlib_prefix(void)
{
#ifdef __CYGWIN__
	return "cyg";
#else
	return "lib";
#endif
}

LELY_UTIL_EXPORT const char *
shlib_suffix(void)
{
#ifdef __CYGWIN__
	return ".dll";
#else
	return ".so";
#endif
}

#endif // _WIN32

