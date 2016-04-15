/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the Layer Setting Services (LSS) and protocols functions.
 *
 * \see lely/co/lss.h
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

#ifndef LELY_NO_CO_LSS

#include <lely/util/errnum.h>
#include <lely/co/lss.h>

#include <assert.h>
#include <stdlib.h>

//! A CANopen LSS master/slave service.
struct __co_lss {
	//! A pointer to a CAN network interface.
	can_net_t *net;
	//! A pointer to a CANopen device.
	co_dev_t *dev;
	//! A pointer to an NMT master/slave service.
	co_nmt_t *nmt;
};

LELY_CO_EXPORT void *
__co_lss_alloc(void)
{
	return malloc(sizeof(struct __co_lss));
}

LELY_CO_EXPORT void
__co_lss_free(void *ptr)
{
	free(ptr);
}

LELY_CO_EXPORT struct __co_lss *
__co_lss_init(struct __co_lss *lss, can_net_t *net, co_dev_t *dev,
		co_nmt_t *nmt)
{
	assert(lss);
	assert(net);
	assert(dev);
	assert(nmt);

	lss->net = net;
	lss->dev = dev;
	lss->nmt = nmt;

	return lss;
}

LELY_CO_EXPORT void
__co_lss_fini(struct __co_lss *lss)
{
	__unused_var(lss);
}

LELY_CO_EXPORT co_lss_t *
co_lss_create(can_net_t *net, co_dev_t *dev, co_nmt_t *nmt)
{
	errc_t errc = 0;

	co_lss_t *lss = __co_lss_alloc();
	if (__unlikely(!lss)) {
		errc = get_errc();
		goto error_alloc_lss;
	}

	if (__unlikely(!__co_lss_init(lss, net, dev, nmt))) {
		errc = get_errc();
		goto error_init_lss;
	}

	return lss;

error_init_lss:
	__co_lss_free(lss);
error_alloc_lss:
	set_errc(errc);
	return NULL;
}

LELY_CO_EXPORT void
co_lss_destroy(co_lss_t *lss)
{
	if (lss) {
		__co_lss_fini(lss);
		__co_lss_free(lss);
	}
}

#endif // !LELY_NO_CO_LSS

