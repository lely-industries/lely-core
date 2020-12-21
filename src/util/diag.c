/**@file
 * This file is part of the utilities library; it contains the diagnostic
 * function definitions.
 *
 * @see lely/util/diag.h
 *
 * @copyright 2013-2020 Lely Industries N.V.
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

#include "util.h"
#if LELY_NO_DIAG
#define LELY_UTIL_DIAG_INLINE extern inline
#endif
#if !LELY_NO_STDIO
#include <lely/libc/stdio.h>
#endif
#include <lely/util/diag.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if _WIN32
#include <wtsapi32.h>
#ifdef _MSC_VER
#pragma comment(lib, "wtsapi32.lib")
#endif
#elif _POSIX_C_SOURCE >= 200809L && !defined(__NEWLIB__)
#include <syslog.h>
#endif

#if _WIN32
#ifndef LELY_DIALOG_DIAG_TIMEOUT
#define LELY_DIALOG_DIAG_TIMEOUT 10
#endif
#endif

#if !LELY_NO_DIAG

static diag_handler_t *diag_handler = &default_diag_handler;
static void *diag_handle;
static diag_at_handler_t *diag_at_handler = &default_diag_at_handler;
static void *diag_at_handle;

#endif // !LELY_NO_DIAG

#if !LELY_NO_STDIO

size_t
floc_lex(struct floc *at, const char *begin, const char *end)
{
	assert(begin);
	assert(!end || end >= begin);

	const char *cp = begin;
	while ((!end || cp < end) && *cp) {
		switch (*cp++) {
		case '\r':
			if ((!end || cp < end) && *cp == '\n')
				cp++;
		// ... falls through ...
		case '\n':
			if (at) {
				at->line++;
				at->column = 1;
			}
			break;
		case '\t':
			if (at)
				at->column = ((at->column + 7) & ~7) + 1;
			break;
		default:
			if (at)
				at->column++;
			break;
		}
	}
	return cp - begin;
}

int
snprintf_floc(char *s, size_t n, const struct floc *at)
{
	assert(at);

	if (!s)
		n = 0;

	int t = 0;

	if (at->filename) {
		int r = snprintf(s, n, "%s:", at->filename);
		if (r < 0)
			return r;
		t += r;
		r = MIN((size_t)r, n);
		s += r;
		n -= r;
		if (at->line) {
			r = snprintf(s, n, "%d:", at->line);
			if (r < 0)
				return r;
			t += r;
			r = MIN((size_t)r, n);
			s += r;
			n -= r;
			if (at->column) {
				r = snprintf(s, n, "%d:", at->column);
				if (r < 0)
					return r;
				t += r;
			}
		}
	}

	return t;
}

#endif // !LELY_NO_STDIO

#if !LELY_NO_DIAG

void
diag_get_handler(diag_handler_t **phandler, void **phandle)
{
	if (phandler)
		*phandler = diag_handler;
	if (phandle)
		*phandle = diag_handle;
}

void
diag_set_handler(diag_handler_t *handler, void *handle)
{
	diag_handler = handler;
	diag_handle = handle;
}

void
diag_at_get_handler(diag_at_handler_t **phandler, void **phandle)
{
	if (phandler)
		*phandler = diag_at_handler;
	if (phandle)
		*phandle = diag_at_handle;
}

void
diag_at_set_handler(diag_at_handler_t *handler, void *handle)
{
	diag_at_handler = handler;
	diag_at_handle = handle;
}

void
diag(enum diag_severity severity, int errc, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vdiag(severity, errc, format, ap);
	va_end(ap);
}

void
vdiag(enum diag_severity severity, int errc, const char *format, va_list ap)
{
	if (diag_handler)
		diag_handler(diag_handle, severity, errc, format, ap);
}

void
diag_at(enum diag_severity severity, int errc, const struct floc *at,
		const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vdiag_at(severity, errc, at, format, ap);
	va_end(ap);
}

void
vdiag_at(enum diag_severity severity, int errc, const struct floc *at,
		const char *format, va_list ap)
{
	if (diag_at_handler)
		diag_at_handler(diag_at_handle, severity, errc, at, format, ap);
}

void
diag_if(enum diag_severity severity, int errc, const struct floc *at,
		const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vdiag_if(severity, errc, at, format, ap);
	va_end(ap);
}

void
vdiag_if(enum diag_severity severity, int errc, const struct floc *at,
		const char *format, va_list ap)
{
	if (at)
		vdiag_at(severity, errc, at, format, ap);
}

void
default_diag_handler(void *handle, enum diag_severity severity, int errc,
		const char *format, va_list ap)
{
	default_diag_at_handler(handle, severity, errc, NULL, format, ap);
}

void
default_diag_at_handler(void *handle, enum diag_severity severity, int errc,
		const struct floc *at, const char *format, va_list ap)
{
	(void)handle;

#if LELY_NO_STDIO
	(void)errc;
	(void)at;
	(void)format;
	(void)ap;
#else
	int errsv = errno;
	char *s = NULL;
	if (vasprintf_diag_at(&s, severity, errc, at, format, ap) >= 0) {
		fprintf(stderr, "%s\n", s);
		fflush(stderr);
	}
	free(s);
	errno = errsv;
#endif

	if (severity == DIAG_FATAL)
		abort();
}

#if !LELY_NO_STDIO

void
cmd_diag_handler(void *handle, enum diag_severity severity, int errc,
		const char *format, va_list ap)
{
	const char *cmd = handle;
	if (cmd && *cmd) {
		int errsv = errno;
		fprintf(stderr, "%s: ", cmd);
		fflush(stderr);
		errno = errsv;
	}

	default_diag_handler(handle, severity, errc, format, ap);
}

void
daemon_diag_handler(void *handle, enum diag_severity severity, int errc,
		const char *format, va_list ap)
{
	daemon_diag_at_handler(handle, severity, errc, NULL, format, ap);
}

void
daemon_diag_at_handler(void *handle, enum diag_severity severity, int errc,
		const struct floc *at, const char *format, va_list ap)
{
#if _WIN32
	dialog_diag_at_handler(handle, severity, errc, at, format, ap);
#else
	syslog_diag_at_handler(handle, severity, errc, at, format, ap);
#endif
}

#if _WIN32

void
dialog_diag_handler(void *handle, enum diag_severity severity, int errc,
		const char *format, va_list ap)
{
	dialog_diag_at_handler(handle, severity, errc, NULL, format, ap);
}

void
dialog_diag_at_handler(void *handle, enum diag_severity severity, int errc,
		const struct floc *at, const char *format, va_list ap)
{
	LPSTR pTitle = (LPSTR)(handle ? handle : "");

	DWORD Style = MB_OK | MB_SETFOREGROUND | MB_TOPMOST;
	switch (severity) {
	case DIAG_DEBUG:
	case DIAG_INFO: Style |= MB_ICONINFORMATION; break;
	case DIAG_WARNING: Style |= MB_ICONWARNING; break;
	case DIAG_ERROR:
	case DIAG_FATAL: Style |= MB_ICONERROR; break;
	default: break;
	}

	int errsv = errno;
	char *pMessage = NULL;
	if (vasprintf_diag_at(&pMessage, severity, errc, at, format, ap) >= 0) {
		DWORD dwResponse;
		WTSSendMessageA(WTS_CURRENT_SERVER_HANDLE,
				WTSGetActiveConsoleSessionId(), pTitle,
				strlen(pTitle), pMessage, strlen(pMessage),
				Style, LELY_DIALOG_DIAG_TIMEOUT, &dwResponse,
				FALSE);
	}
	free(pMessage);
	errno = errsv;

	if (severity == DIAG_FATAL)
		abort();
}

#endif // _WIN32

void
log_diag_handler(void *handle, enum diag_severity severity, int errc,
		const char *format, va_list ap)
{
	log_diag_at_handler(handle, severity, errc, NULL, format, ap);
}

void
log_diag_at_handler(void *handle, enum diag_severity severity, int errc,
		const struct floc *at, const char *format, va_list ap)
{
	int errsv = errno;
	time_t timer;
	if (time(&timer) != -1) {
#if _WIN32
		struct tm tm;
		struct tm *timeptr = NULL;
		if (localtime_s(&tm, &timer))
			timeptr = &tm;
#elif defined(_POSIX_C_SOURCE)
		struct tm tm;
		struct tm *timeptr = localtime_r(&timer, &tm);
#else
		struct tm *timeptr = localtime(&timer);
#endif
		if (timeptr) {
			char buf[80];
			// clang-format off
			if (strftime(buf, sizeof(buf),
					"%a, %d %b %Y %H:%M:%S %z", timeptr)
					> 0) {
				// clang-format on
				fprintf(stderr, "%s: ", buf);
				fflush(stderr);
			}
		}
	}
	errno = errsv;

	default_diag_at_handler(handle, severity, errc, at, format, ap);
}

void
syslog_diag_handler(void *handle, enum diag_severity severity, int errc,
		const char *format, va_list ap)
{
	syslog_diag_at_handler(handle, severity, errc, NULL, format, ap);
}

void
syslog_diag_at_handler(void *handle, enum diag_severity severity, int errc,
		const struct floc *at, const char *format, va_list ap)
{
#if _POSIX_C_SOURCE >= 200809L && !defined(__NEWLIB__)
	(void)handle;

	int priority = LOG_USER;
	switch (severity) {
	case DIAG_DEBUG: priority |= LOG_DEBUG; break;
	case DIAG_INFO: priority |= LOG_INFO; break;
	case DIAG_WARNING: priority |= LOG_WARNING; break;
	case DIAG_ERROR: priority |= LOG_ERR; break;
	case DIAG_FATAL: priority |= LOG_EMERG; break;
	}

	int errsv = errno;
	char *s = NULL;
	if (vasprintf_diag_at(&s, DIAG_INFO, errc, at, format, ap) >= 0)
		syslog(priority, "%s", s);
	free(s);
	errno = errsv;

	if (severity == DIAG_FATAL)
		abort();
#else
	log_diag_at_handler(handle, severity, errc, at, format, ap);
#endif
}

int
vsnprintf_diag(char *s, size_t n, enum diag_severity severity, int errc,
		const char *format, va_list ap)
{
	return vsnprintf_diag_at(s, n, severity, errc, NULL, format, ap);
}

int
vasprintf_diag(char **ps, enum diag_severity severity, int errc,
		const char *format, va_list ap)
{
	return vasprintf_diag_at(ps, severity, errc, NULL, format, ap);
}

int
vsnprintf_diag_at(char *s, size_t n, enum diag_severity severity, int errc,
		const struct floc *at, const char *format, va_list ap)
{
	assert(format);

	if (!s)
		n = 0;

	int r, t = 0;

	if (at && (r = snprintf_floc(s, n, at)) != 0) {
		if (r < 0)
			return r;
		t += r;
		r = MIN((size_t)r, n);
		s += r;
		n -= r;
		r = snprintf(s, n, " ");
		if (r < 0)
			return r;
		t += r;
		r = MIN((size_t)r, n);
		s += r;
		n -= r;
	}

	switch (severity) {
	case DIAG_DEBUG: r = snprintf(s, n, "debug: "); break;
	case DIAG_INFO: r = 0; break;
	case DIAG_WARNING: r = snprintf(s, n, "warning: "); break;
	case DIAG_ERROR: r = snprintf(s, n, "error: "); break;
	case DIAG_FATAL: r = snprintf(s, n, "fatal: "); break;
	default: r = 0; break;
	}
	if (r < 0)
		return r;
	t += r;
	r = MIN((size_t)r, n);
	s += r;
	n -= r;

	if (format && *format) {
		r = vsnprintf(s, n, format, ap);
		if (r < 0)
			return r;
		t += r;
		r = MIN((size_t)r, n);
		s += r;
		n -= r;
		if (errc) {
			r = snprintf(s, n, ": ");
			if (r < 0)
				return r;
			t += r;
			r = MIN((size_t)r, n);
			s += r;
			n -= r;
		}
	}

	if (errc) {
		const char *errstr = errc2str(errc);
		if (errstr) {
			r = snprintf(s, n, "%s", errstr);
			if (r < 0)
				return r;
			t += r;
		}
	}

	return t;
}

int
vasprintf_diag_at(char **ps, enum diag_severity severity, int errc,
		const struct floc *at, const char *format, va_list ap)
{
	assert(ps);

	va_list aq;
	va_copy(aq, ap);
	int n = vsnprintf_diag_at(NULL, 0, severity, errc, at, format, aq);
	va_end(aq);
	if (n < 0)
		return n;

	char *s = malloc(n + 1);
	if (!s)
		return -1;

	n = vsnprintf_diag_at(s, n + 1, severity, errc, at, format, ap);
	if (n < 0) {
		int errsv = errno;
		free(s);
		errno = errsv;
		return n;
	}

	*ps = s;
	return n;
}

#endif // !LELY_NO_STDIO

#endif // !LELY_NO_DIAG

#if !LELY_NO_STDIO

const char *
cmdname(const char *path)
{
	assert(path);

	const char *cmd = path;
	while (*cmd)
		cmd++;
#if _WIN32
	while (cmd >= path && *cmd != '\\')
#else
	while (cmd >= path && *cmd != '/')
#endif
		cmd--;
	return ++cmd;
}

#endif // !LELY_NO_STDIO
