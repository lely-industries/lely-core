/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the ASCII gateway functions.
 *
 * \see lely/co/gw_txt.h
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

#include "co.h"

#ifndef LELY_NO_CO_GW_TXT

#include <lely/util/diag.h>
#include <lely/util/lex.h>
#include <lely/co/gw_txt.h>

#include <assert.h>
#include <stdlib.h>

//! A CANopen ASCII gateway.
struct __co_gw_txt {
	//! A pointer to the callback function invoked by co_gw_txt_recv().
	co_gw_txt_recv_func_t *recv_func;
	//! A pointer to the user-specified data for #recv_func.
	void *recv_data;
	//! A pointer to the callback function invoked by co_gw_txt_send().
	co_gw_txt_send_func_t *send_func;
	//! A pointer to the user-specified data for #send_func.
	void *send_data;
};

LELY_CO_EXPORT void *
__co_gw_txt_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_gw_txt));
	if (__unlikely(!ptr))
		set_errno(errno);
	return ptr;
}

LELY_CO_EXPORT void
__co_gw_txt_free(void *ptr)
{
	free(ptr);
}

LELY_CO_EXPORT struct __co_gw_txt *
__co_gw_txt_init(struct __co_gw_txt *gw)
{
	assert(gw);

	gw->recv_func = NULL;
	gw->recv_data = NULL;

	gw->send_func = NULL;
	gw->send_data = NULL;

	return gw;
}

LELY_CO_EXPORT void
__co_gw_txt_fini(struct __co_gw_txt *gw)
{
	assert(gw);
}

LELY_CO_EXPORT co_gw_txt_t *
co_gw_txt_create(void)
{
	errc_t errc = 0;

	co_gw_txt_t *gw = __co_gw_txt_alloc();
	if (__unlikely(!gw)) {
		errc = get_errc();
		goto error_alloc_gw;
	}

	if (__unlikely(!__co_gw_txt_init(gw))) {
		errc = get_errc();
		goto error_init_gw;
	}

	return gw;

error_init_gw:
	__co_gw_txt_free(gw);
error_alloc_gw:
	set_errc(errc);
	return NULL;
}

LELY_CO_EXPORT void
co_gw_txt_destroy(co_gw_txt_t *gw)
{
	if (gw) {
		__co_gw_txt_fini(gw);
		__co_gw_txt_free(gw);
	}
}

LELY_CO_EXPORT int
co_gw_txt_recv(co_gw_txt_t *gw, const struct co_gw_srv *srv)
{
	assert(srv);

	if (__unlikely(srv->size < sizeof(*srv))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	switch (srv->srv) {
	default:
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
}

LELY_CO_EXPORT void
co_gw_txt_get_recv_func(const co_gw_txt_t *gw, co_gw_txt_recv_func_t **pfunc,
		void **pdata)
{
	assert(gw);

	if (pfunc)
		*pfunc = gw->recv_func;
	if (pdata)
		*pdata = gw->recv_data;
}

LELY_CO_EXPORT size_t
co_gw_txt_send(co_gw_txt_t *gw, const char *begin, const char *end,
		struct floc *at)
{
	assert(gw);
	assert(begin);
	assert(!end || end >= begin);

	struct floc *floc = NULL;
	struct floc floc_;
	if (at) {
		floc = &floc_;
		*floc = *at;
	}

	const char *cp = begin;
	size_t chars = 0;

	// Skip leading whitespace.
	cp += lex_ctype(&isspace, cp, end, floc);

	// Skip trailing whitespace.
	cp += lex_ctype(&isspace, cp, end, floc);

	if ((!end || cp < end) && *cp && !isbreak((unsigned char)*cp))
		diag_if(DIAG_ERROR, 0, floc,
				"expected line break after request");

	// Skip all characters until (and including) the next line break.
	chars = 0;
	while ((!end || cp + chars < end) && cp[chars] && cp[chars++] != '\n');
	cp += chars;

	return floc_lex(at, begin, cp);
}

LELY_CO_EXPORT void
co_gw_txt_set_recv_func(co_gw_txt_t *gw, co_gw_txt_recv_func_t *func,
		void *data)
{
	assert(gw);

	gw->recv_func = func;
	gw->recv_data = data;
}

LELY_CO_EXPORT void
co_gw_txt_get_send_func(const co_gw_txt_t *gw, co_gw_txt_send_func_t **pfunc,
		void **pdata)
{
	assert(gw);

	if (pfunc)
		*pfunc = gw->send_func;
	if (pdata)
		*pdata = gw->send_data;
}

LELY_CO_EXPORT void
co_gw_txt_set_send_func(co_gw_txt_t *gw, co_gw_txt_send_func_t *func,
		void *data)
{
	assert(gw);

	gw->send_func = func;
	gw->send_data = data;
}

#endif // !LELY_NO_CO_GW_TXT

