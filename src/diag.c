/*!\file
 * This file is part of the utilities library; it contains the diagnostic
 * function definitions.
 *
 * \see lely/util/diag.h
 *
 * \copyright 2016 Lely Industries N.V.
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static diag_handler_t *diag_handler = &default_diag_handler;
static void *diag_handle;
static diag_at_handler_t *diag_at_handler = &default_diag_at_handler;
static void *diag_at_handle;

LELY_UTIL_EXPORT void
floc_strinc(struct floc *at, const char *s)
{
	assert(s);

	floc_strninc(at, s, strlen(s));
}

LELY_UTIL_EXPORT void
floc_strninc(struct floc *at, const char *s, size_t n)
{
	assert(at);
	assert(s);

	const char *end = s + n;
	while (*s && s < end) {
		switch (*s++) {
		case '\r':
			if (s < end && *s == '\n')
				s++;
		case '\n':
			at->line++;
			at->column = 1;
			break;
		case '\t':
			at->column = ((at->column + 7) & ~7) + 1;
			break;
		default:
			at->column++;
			break;
		}
	}
}

LELY_UTIL_EXPORT int
snprintf_floc(char *s, size_t n, const struct floc *at)
{
	assert(at);

	if (!s)
		n = 0;

	int r, t = 0;

	if (at->filename) {
		r = snprintf(s, n, "%s:", at->filename);
		if (__unlikely(r < 0))
			return r;
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		if (at->line) {
			r = snprintf(s, n, "%d:", at->line);
			if (__unlikely(r < 0))
				return r;
			t += r; r = MIN((size_t)r, n); s += r; n -= r;
			if (at->column) {
				r = snprintf(s, n, "%d:", at->column);
				if (__unlikely(r < 0))
					return r;
				t += r;
			}
		}
	}

	return t;
}

LELY_UTIL_EXPORT void
diag_get_handler(diag_handler_t **phandler, void **phandle)
{
	if (phandler)
		*phandler = diag_handler;
	if (phandle)
		*phandle = diag_handle;
}

LELY_UTIL_EXPORT void
diag_set_handler(diag_handler_t *handler, void *handle)
{
	diag_handler = handler;
	diag_handle = handle;
}

LELY_UTIL_EXPORT void
diag_at_get_handler(diag_at_handler_t **phandler, void **phandle)
{
	if (phandler)
		*phandler = diag_at_handler;
	if (phandle)
		*phandle = diag_at_handle;
}

LELY_UTIL_EXPORT void
diag_at_set_handler(diag_at_handler_t *handler, void *handle)
{
	diag_at_handler = handler;
	diag_at_handle = handle;
}

LELY_UTIL_EXPORT void
diag(enum diag_severity severity, errc_t errc, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vdiag(severity, errc, format, ap);
	va_end(ap);
}

LELY_UTIL_EXPORT void
vdiag(enum diag_severity severity, errc_t errc, const char *format, va_list ap)
{
	diag_handler_t *handler = NULL;
	void *handle = NULL;
	diag_get_handler(&handler, &handle);
	if (handler)
		handler(handle, severity, errc, format, ap);
}

LELY_UTIL_EXPORT void
diag_at(enum diag_severity severity, errc_t errc, const struct floc *at,
		const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vdiag_at(severity, errc, at, format, ap);
	va_end(ap);
}

LELY_UTIL_EXPORT void
vdiag_at(enum diag_severity severity, errc_t errc, const struct floc *at,
		const char *format, va_list ap)
{
	diag_at_handler_t *handler = NULL;
	void *handle = NULL;
	diag_at_get_handler(&handler, &handle);
	if (handler)
		handler(handle, severity, errc, at, format, ap);
}

LELY_UTIL_EXPORT void
default_diag_handler(void *handle, enum diag_severity severity, errc_t errc,
		const char *format, va_list ap)
{
	default_diag_at_handler(handle, severity, errc, NULL, format, ap);
}

LELY_UTIL_EXPORT void
default_diag_at_handler(void *handle, enum diag_severity severity, errc_t errc,
		const struct floc *at, const char *format, va_list ap)
{

	__unused_var(handle);

	va_list aq;
	va_copy(aq, ap);
	int n = vsnprintf_diag_at(NULL, 0, severity, errc, at, format, aq);
	va_end(aq);

	if (__likely(n > 0)) {
		char s[n + 1];
		vsnprintf_diag_at(s, n + 1, severity, errc, at, format, ap);
		fprintf(stderr, "%s\n", s);
	}

	if (severity == DIAG_FATAL)
		exit(EXIT_FAILURE);
}

LELY_UTIL_EXPORT int
vsnprintf_diag(char *s, size_t n, enum diag_severity severity, errc_t errc,
		const char *format, va_list ap)
{
	return vsnprintf_diag_at(s, n, severity, errc, NULL, format, ap);
}

LELY_UTIL_EXPORT int
vsnprintf_diag_at(char *s, size_t n, enum diag_severity severity, errc_t errc,
		const struct floc *at, const char *format, va_list ap)
{
	assert(format);

	if (!s)
		n = 0;

	int r, t = 0;

	if (at && (r = snprintf_floc(s, n, at))) {
		if (__unlikely(r < 0))
			return r;
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf(s, n, " ");
		if (__unlikely(r < 0))
			return r;
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
	}

	switch (severity) {
	case DIAG_DEBUG:
		r = snprintf(s, n, "debug: ");
		break;
	case DIAG_INFO:
		r = 0;
		break;
	case DIAG_WARNING:
		r = snprintf(s, n, "warning: ");
		break;
	case DIAG_ERROR:
		r = snprintf(s, n, "error: ");
		break;
	case DIAG_FATAL:
		r = snprintf(s, n, "fatal: ");
		break;
	default:
		r = 0;
		break;
	}
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	if (format && *format) {
		r = vsnprintf(s, n, format, ap);
		if (__unlikely(r < 0))
			return r;
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		if (errc) {
			r = snprintf(s, n, ": ");
			if (__unlikely(r < 0))
				return r;
			t += r; r = MIN((size_t)r, n); s += r; n -= r;
		}
	}

	if (errc) {
		char *errstr = errc2str(errc);
		if (__likely(errstr)) {
			r = snprintf(s, n, "%s", errstr);
			if (__unlikely(r < 0))
				return r;
			t += r; r = MIN((size_t)r, n); s += r; n -= r;
		}
	}

	return t;
}

LELY_UTIL_EXPORT const char *
cmdname(const char *path)
{
	assert(path);

	const char *cmd = path;
	while (*cmd)
		cmd++;
#ifdef _WIN32
	while (cmd >= path && *cmd != '\\')
#else
	while (cmd >= path && *cmd != '/')
#endif
		cmd--;
	return ++cmd;
}

