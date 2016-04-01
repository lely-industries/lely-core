/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the NMT error control functions.
 *
 * \see src/nmt_ec.h
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
#include <lely/util/errnum.h>
#include <lely/co/dev.h>
#include "nmt_ec.h"

#include <assert.h>
#include <stdlib.h>

//! A CANopen NMT heartbeat consumer.
struct __co_nmt_hb {
	//! A pointer to a CAN network interface.
	can_net_t *net;
	//! A pointer to a CANopen device.
	co_dev_t *dev;
	//! A pointer to an NMT master/slave service.
	co_nmt_t *nmt;
	//! A pointer to the CAN frame receiver.
	can_recv_t *recv;
	//! A pointer to the CAN timer.
	can_timer_t *timer;
	//! The Node-ID.
	co_unsigned8_t id;
	//! The state of the node (excluding the toggle bit).
	co_unsigned8_t st;
	//! The consumer heartbeat time (in milliseconds).
	co_unsigned16_t ms;
	/*!
	 * Indicates whether a heartbeat error occurred (#CO_NMT_EC_OCCURRED or
	 * #CO_NMT_EC_RESOLVED).
	 */
	int state;
	//! A pointer to the heartbeat event indication function.
	co_nmt_hb_ind_t *hb_ind;
	//! A pointer to user-specified data for #hb_ind.
	void *hb_data;
	//! A pointer to the state change indication function.
	co_nmt_st_ind_t *st_ind;
	//! A pointer to user-specified data for #st_ind.
	void *st_data;
};

/*!
 * The CAN receive callback function for a heartbeat consumer.
 *
 * \see can_recv_func_t
 */
static int co_nmt_hb_recv(const struct can_msg *msg, void *data);

/*!
 * The CAN timer callback function for a heartbeat consumer.
 *
 * \see can_timer_func_t
 */
static int co_nmt_hb_timer(const struct timespec *tp, void *data);

void *
__co_nmt_hb_alloc(void)
{
	return malloc(sizeof(struct __co_nmt_hb));
}

void
__co_nmt_hb_free(void *ptr)
{
	free(ptr);
}

struct __co_nmt_hb *
__co_nmt_hb_init(struct __co_nmt_hb *hb, can_net_t *net, co_dev_t *dev,
		co_nmt_t *nmt)
{
	assert(hb);
	assert(net);
	assert(dev);
	assert(nmt);

	errc_t errc = 0;

	hb->net = net;
	hb->dev = dev;
	hb->nmt = nmt;

	hb->recv = can_recv_create();
	if (__unlikely(!hb->recv)) {
		errc = get_errc();
		goto error_create_recv;
	}
	can_recv_set_func(hb->recv, &co_nmt_hb_recv, hb);

	hb->timer = can_timer_create();
	if (__unlikely(!hb->timer)) {
		errc = get_errc();
		goto error_create_timer;
	}
	can_timer_set_func(hb->timer, &co_nmt_hb_timer, hb);

	hb->id = 0;
	hb->st = 0;
	hb->ms = 0;
	hb->state = CO_NMT_EC_RESOLVED;

	hb->hb_ind = NULL;
	hb->hb_data = NULL;
	hb->st_ind = NULL;
	hb->st_data = NULL;

	return hb;

	can_timer_destroy(hb->timer);
error_create_timer:
	can_recv_destroy(hb->recv);
error_create_recv:
	set_errc(errc);
	return NULL;
}

void
__co_nmt_hb_fini(struct __co_nmt_hb *hb)
{
	assert(hb);

	can_timer_destroy(hb->timer);

	can_recv_destroy(hb->recv);
}

co_nmt_hb_t *
co_nmt_hb_create(can_net_t *net, co_dev_t *dev, co_nmt_t *nmt)
{
	errc_t errc = 0;

	co_nmt_hb_t *hb = __co_nmt_hb_alloc();
	if (__unlikely(!hb)) {
		errc = get_errc();
		goto error_alloc_hb;
	}

	if (__unlikely(!__co_nmt_hb_init(hb, net, dev, nmt))) {
		errc = get_errc();
		goto error_init_hb;
	}

	return hb;

error_init_hb:
	__co_nmt_hb_free(hb);
error_alloc_hb:
	set_errc(errc);
	return NULL;
}

void
co_nmt_hb_destroy(co_nmt_hb_t *hb)
{
	if (hb) {
		__co_nmt_hb_fini(hb);
		__co_nmt_hb_free(hb);
	}
}

void
co_nmt_hb_get_hb_ind(const co_nmt_hb_t *hb, co_nmt_hb_ind_t **pind,
		void **pdata)
{
	assert(hb);

	if (pind)
		*pind = hb->hb_ind;
	if (pdata)
		*pdata = hb->hb_data;
}

void
co_nmt_hb_set_hb_ind(co_nmt_hb_t *hb, co_nmt_hb_ind_t *ind, void *data)
{
	assert(hb);

	hb->hb_ind = ind;
	hb->hb_data = data;
}

void
co_nmt_hb_get_st_ind(const co_nmt_hb_t *hb, co_nmt_st_ind_t **pind,
		void **pdata)
{
	assert(hb);

	if (pind)
		*pind = hb->st_ind;
	if (pdata)
		*pdata = hb->st_data;
}

void
co_nmt_hb_set_st_ind(co_nmt_hb_t *hb, co_nmt_st_ind_t *ind, void *data)
{
	assert(hb);

	hb->st_ind = ind;
	hb->st_data = data;
}

void
co_nmt_hb_set_1016(co_nmt_hb_t *hb, co_unsigned8_t id, co_unsigned16_t ms)
{
	assert(hb);

	can_recv_stop(hb->recv);
	can_timer_stop(hb->timer);

	hb->id = id;
	hb->st = 0;
	hb->ms = ms;
	hb->state = CO_NMT_EC_RESOLVED;

	if (hb->id && hb->id <= CO_NUM_NODES && hb->ms)
		can_recv_start(hb->recv, hb->net, 0x700 + id, 0);
}

void
co_nmt_hb_set_st(co_nmt_hb_t *hb, co_unsigned8_t st)
{
	assert(hb);

	if (hb->id && hb->id <= CO_NUM_NODES && hb->ms) {
		hb->st = st;
		// Reset the CAN timer for the heartbeat consumer.
		can_timer_timeout(hb->timer, hb->net, hb->ms);
	}
}

static int
co_nmt_hb_recv(const struct can_msg *msg, void *data)
{
	assert(msg);
	co_nmt_hb_t *hb = data;
	assert(hb);
	assert(hb->id && hb->id <= CO_NUM_NODES);
	assert(msg->id == (uint32_t)(0x700 + hb->id));
	assert(hb->ms);

	// Obtain the node status from the CAN frame. Ignore if the toggle bit
	// is set, since then it is not a heartbeat message.
	if (__unlikely(msg->len < 1))
		return 0;
	co_unsigned8_t st = msg->data[0];
	if (__unlikely(st & CO_NMT_ST_TOGGLE))
		return 0;

	// Update the state.
	co_unsigned8_t old_st = hb->st;
	co_nmt_hb_set_st(hb, st);

	if (hb->state == CO_NMT_EC_OCCURRED) {
		// If a heartbeat error occurred, notify the user that it has
		// been resolved.
		hb->state = CO_NMT_EC_RESOLVED;
		if (hb->hb_ind)
			hb->hb_ind(hb->nmt, hb->id, hb->state, hb->hb_data);
	} else if (old_st && st != old_st) {
		// Only notify the user of the occurrence of a state change, not
		// its resolution.
		if (hb->st_ind)
			hb->st_ind(hb->nmt, hb->id, st, hb->st_data);
	}

	return 0;
}

static int
co_nmt_hb_timer(const struct timespec *tp, void *data)
{
	__unused_var(tp);
	co_nmt_hb_t *hb = data;
	assert(hb);

	// Notify the user of the occurrence of a heartbeat error.
	hb->state = CO_NMT_EC_OCCURRED;
	if (hb->hb_ind)
		hb->hb_ind(hb->nmt, hb->id, hb->state, hb->hb_data);

	return 0;
}

