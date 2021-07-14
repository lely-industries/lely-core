/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the NMT redundancy manager functions.
 *
 * @see lely/co/nmt_rdn.h
 *
 * @copyright 2021 N7 Space Sp. z o.o.
 *
 * The NMT redundancy manager was developed under a programme of,
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

#include "nmt_rdn.h"
#include "co.h"
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/util/diag.h>

#include <assert.h>

/// A CANopen NMT redundancy manger.
struct co_nmt_rdn {
	/// A pointer to a CAN network interface.
	can_net_t *net;
	/// A pointer to a CANopen device.
	co_dev_t *dev;
	/// A pointer to an NMT master/slave service.
	co_nmt_t *nmt;
	/// The CAN bus A identifier.
	co_unsigned8_t bus_a_id;
	/// The CAN bus B identifier.
	co_unsigned8_t bus_b_id;
	/// The Redundancy Master Node-ID.
	co_unsigned8_t master_id;
	/// The Redundancy Master consumer heartbeat timer (in milliseconds).
	co_unsigned16_t master_ms;
	/// A pointer to the CAN bus toggle timer.
	can_timer_t *bus_toggle_timer;
};

/// Allocates memory for #co_nmt_rdn_t object using allocator
/// from #can_net_t.
static void *co_nmt_rdn_alloc(can_net_t *net);

/// Frees memory allocated for #co_nmt_rdn_t object.
static void co_nmt_rdn_free(co_nmt_rdn_t *rdn);

/// Initializes #co_nmt_rdn_t object.
static co_nmt_rdn_t *co_nmt_rdn_init(
		co_nmt_rdn_t *rdn, can_net_t *net, co_nmt_t *nmt);

/// Finalizes #co_nmt_rdn_t object.
static void co_nmt_rdn_fini(co_nmt_rdn_t *rdn);

/// Performs a CAN bus switch.
static void co_nmt_rdn_switch_bus(co_nmt_rdn_t *rdn);

/**
 * The CAN bus toggle timer callback function for a redundancy manager.
 *
 * @see can_timer_func_t
 */
static int co_nmt_rdn_bus_toggle_timer(const struct timespec *tp, void *data);

/// Check sub-index 0 (Highest sub-index supported)
static int co_nmt_rdn_chk_dev_sub0(const co_obj_t *obj);
/// Check sub-index 1 (Bdefault)
static int co_nmt_rdn_chk_dev_sub_bdefault(const co_obj_t *obj);
/// Check sub-index 2 (Ttoggle)
static int co_nmt_rdn_chk_dev_sub_ttoggle(const co_obj_t *obj);
/// Check sub-index 3 (Ntoggle)
static int co_nmt_rdn_chk_dev_sub_ntoggle(const co_obj_t *obj);
/// Check sub-index 4 (Ctoggle)
static int co_nmt_rdn_chk_dev_sub_ctoggle(const co_obj_t *obj);

int
co_nmt_rdn_chk_dev(const co_dev_t *dev)
{
	const co_obj_t *obj_rdn =
			co_dev_find_obj(dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX);

	// Check Redundancy Configuration object
	if (!obj_rdn)
		return 1;

	// Check sub-objects
	if ((co_nmt_rdn_chk_dev_sub0(obj_rdn) != 1)
			|| (co_nmt_rdn_chk_dev_sub_bdefault(obj_rdn) != 1)
			|| (co_nmt_rdn_chk_dev_sub_ttoggle(obj_rdn) != 1)
			|| (co_nmt_rdn_chk_dev_sub_ntoggle(obj_rdn) != 1)
			|| (co_nmt_rdn_chk_dev_sub_ctoggle(obj_rdn) != 1))
		return 0;

	return 1;
}

size_t
co_nmt_rdn_alignof(void)
{
	return _Alignof(co_nmt_rdn_t);
}

size_t
co_nmt_rdn_sizeof(void)
{
	return sizeof(co_nmt_rdn_t);
}

co_nmt_rdn_t *
co_nmt_rdn_create(can_net_t *net, co_nmt_t *nmt)
{
	int errc = 0;

	co_nmt_rdn_t *rdn = co_nmt_rdn_alloc(net);
	if (!rdn) {
		errc = get_errc();
		goto error_alloc_rdn;
	}

	if (!co_nmt_rdn_init(rdn, net, nmt)) {
		errc = get_errc();
		goto error_init_rdn;
	}

	return rdn;

error_init_rdn:
	co_nmt_rdn_free(rdn);
error_alloc_rdn:
	set_errc(errc);
	return NULL;
}

void
co_nmt_rdn_destroy(co_nmt_rdn_t *rdn)
{
	if (rdn != NULL) {
		co_nmt_rdn_fini(rdn);
		co_nmt_rdn_free(rdn);
	}
}

alloc_t *
co_nmt_rdn_get_alloc(const co_nmt_rdn_t *rdn)
{
	assert(rdn);

	return can_net_get_alloc(rdn->net);
}

void
co_nmt_rdn_select_default_bus(co_nmt_rdn_t *rdn)
{
	assert(rdn);

	const co_unsigned8_t bdefault = co_dev_get_val_u8(rdn->dev,
			CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
			CO_NMT_RDN_BDEFAULT_SUBIDX);

	rdn->bus_a_id = bdefault;

	can_net_set_active_bus(rdn->net, bdefault);

	if (co_nmt_is_master(rdn->nmt) == 0) {
		const co_unsigned8_t ttoggle = co_dev_get_val_u8(rdn->dev,
				CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
				CO_NMT_RDN_TTOGGLE_SUBIDX);
		can_timer_timeout(rdn->bus_toggle_timer, rdn->net,
				rdn->master_ms * ttoggle);
	}
}

void
co_nmt_rdn_set_active_bus_default(co_nmt_rdn_t *rdn)
{
	assert(rdn);

	const co_unsigned8_t bdefault = can_net_get_active_bus(rdn->net);

	co_dev_set_val_u8(rdn->dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
			CO_NMT_RDN_BDEFAULT_SUBIDX, bdefault);

	can_timer_stop(rdn->bus_toggle_timer);
}

void
co_nmt_rdn_slave_missed_hb(co_nmt_rdn_t *rdn)
{
	assert(rdn);

	const co_unsigned8_t ttoggle = co_dev_get_val_u8(rdn->dev,
			CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
			CO_NMT_RDN_TTOGGLE_SUBIDX);

	if (ttoggle != 0) {
		int timeout = rdn->master_ms
				* (ttoggle - 1u); // the first HB interval already passed
		can_timer_timeout(rdn->bus_toggle_timer, rdn->net, timeout);
	}
}

void
co_nmt_rdn_set_alternate_bus_id(co_nmt_rdn_t *rdn, co_unsigned8_t bus_id)
{
	assert(rdn);

	rdn->bus_b_id = bus_id;
}

int
co_nmt_rdn_set_master_id(
		co_nmt_rdn_t *rdn, co_unsigned8_t id, co_unsigned16_t ms)
{
	assert(rdn);

	if (co_nmt_is_master(rdn->nmt) != 0) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	if ((id == 0) || (id > CO_NUM_NODES)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	rdn->master_id = id;
	rdn->master_ms = ms;

	return 0;
}

co_unsigned8_t
co_nmt_rdn_get_master_id(const co_nmt_rdn_t *rdn)
{
	assert(rdn);

	if (co_nmt_is_master(rdn->nmt) != 0)
		return co_nmt_get_id(rdn->nmt);
	else
		return rdn->master_id;
}

static void *
co_nmt_rdn_alloc(can_net_t *net)
{
	co_nmt_rdn_t *rdn = mem_alloc(can_net_get_alloc(net),
			co_nmt_rdn_alignof(), co_nmt_rdn_sizeof());
	if (!rdn)
		return NULL;

	rdn->net = net;

	return rdn;
}

static void
co_nmt_rdn_free(co_nmt_rdn_t *rdn)
{
	mem_free(co_nmt_rdn_get_alloc(rdn), rdn);
}

static co_nmt_rdn_t *
co_nmt_rdn_init(co_nmt_rdn_t *rdn, can_net_t *net, co_nmt_t *nmt)
{
	assert(rdn);
	assert(net);
	assert(nmt);

	int errc = 0;

	rdn->net = net;
	rdn->nmt = nmt;
	rdn->dev = co_nmt_get_dev(nmt);

	const co_unsigned8_t bdefault = co_dev_get_val_u8(rdn->dev,
			CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
			CO_NMT_RDN_BDEFAULT_SUBIDX);

	rdn->bus_a_id = bdefault;
	rdn->bus_b_id = bdefault;

	rdn->master_id = 0;
	rdn->master_ms = 0;

	rdn->bus_toggle_timer = can_timer_create(co_nmt_rdn_get_alloc(rdn));
	if (!rdn->bus_toggle_timer) {
		errc = get_errc();
		goto error_create_timer;
	}
	can_timer_set_func(rdn->bus_toggle_timer, &co_nmt_rdn_bus_toggle_timer,
			rdn);

	return rdn;

	// can_timer_destroy(rdn->bus_toggle_timer);
error_create_timer:
	set_errc(errc);
	return NULL;
}

static void
co_nmt_rdn_fini(co_nmt_rdn_t *rdn)
{
	assert(rdn);

	(void)rdn;
}

static void
co_nmt_rdn_switch_bus(co_nmt_rdn_t *rdn)
{
	assert(rdn);

	const int active_bus = can_net_get_active_bus(rdn->net);
	const co_unsigned8_t new_bus = (active_bus == rdn->bus_a_id)
			? rdn->bus_b_id
			: rdn->bus_a_id;

	if (new_bus != active_bus)
		can_net_set_active_bus(rdn->net, new_bus);
}

static int
co_nmt_rdn_bus_toggle_timer(const struct timespec *tp, void *data)
{
	(void)tp;
	co_nmt_rdn_t *rdn = data;
	assert(rdn);

	diag(DIAG_INFO, 0, "NMT: redundancy manager performs a bus switch");

	const co_unsigned8_t ntoggle = co_dev_get_val_u8(rdn->dev,
			CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
			CO_NMT_RDN_NTOGGLE_SUBIDX);
	co_unsigned8_t ctoggle = co_dev_get_val_u8(rdn->dev,
			CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
			CO_NMT_RDN_CTOGGLE_SUBIDX);

	co_nmt_rdn_switch_bus(rdn);
	ctoggle++;
	co_dev_set_val_u8(rdn->dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
			CO_NMT_RDN_CTOGGLE_SUBIDX, ctoggle);

	co_nmt_ecss_rdn_ind(rdn->nmt, co_nmt_get_active_bus_id(rdn->nmt),
			CO_NMT_ECSS_RDN_BUS_SWITCH);

	if (ctoggle < ntoggle) {
		const co_unsigned8_t ttoggle = co_dev_get_val_u8(rdn->dev,
				CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
				CO_NMT_RDN_TTOGGLE_SUBIDX);

		can_timer_timeout(rdn->bus_toggle_timer, rdn->net,
				rdn->master_ms * ttoggle);
	} else {
		co_nmt_rdn_set_active_bus_default(rdn);
		co_nmt_ecss_rdn_ind(rdn->nmt,
				co_nmt_get_active_bus_id(rdn->nmt),
				CO_NMT_ECSS_RDN_NO_MASTER);
	}

	return 0;
}

static int
co_nmt_rdn_chk_dev_sub0(const co_obj_t *obj_rdn)
{
	const co_sub_t *sub_rdn_00 = co_obj_find_sub(obj_rdn, 0x00);
	if (!sub_rdn_00) {
		diag(DIAG_ERROR, 0, "NMT: mandatory object %04X:00 missing",
				CO_NMT_RDN_REDUNDANCY_OBJ_IDX);
		return 0;
	}
	if (co_sub_get_type(sub_rdn_00) != CO_DEFTYPE_UNSIGNED8) {
		diag(DIAG_ERROR, 0, "NMT: object %04X:00 is not UNSIGNED8",
				CO_NMT_RDN_REDUNDANCY_OBJ_IDX);
		return 0;
	}

	return 1;
}

static int
co_nmt_rdn_chk_dev_sub_bdefault(const co_obj_t *obj_rdn)
{
	const co_sub_t *sub_rdn_bdefault =
			co_obj_find_sub(obj_rdn, CO_NMT_RDN_BDEFAULT_SUBIDX);
	if (sub_rdn_bdefault == NULL) {
		diag(DIAG_ERROR, 0, "NMT: mandatory object %04X:%02X missing",
				CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
				CO_NMT_RDN_BDEFAULT_SUBIDX);
		return 0;
	}
	if (co_sub_get_type(sub_rdn_bdefault) != CO_DEFTYPE_UNSIGNED8) {
		diag(DIAG_ERROR, 0, "NMT: object %04X:%02X is not UNSIGNED8",
				CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
				CO_NMT_RDN_BDEFAULT_SUBIDX);
		return 0;
	}

	return 1;
}

static int
co_nmt_rdn_chk_dev_sub_ttoggle(const co_obj_t *obj_rdn)
{
	const co_sub_t *sub_rdn_ttoggle =
			co_obj_find_sub(obj_rdn, CO_NMT_RDN_TTOGGLE_SUBIDX);
	if ((sub_rdn_ttoggle != NULL)
			&& (co_sub_get_type(sub_rdn_ttoggle)
					!= CO_DEFTYPE_UNSIGNED8)) {
		diag(DIAG_ERROR, 0, "NMT: object %04X:%02X is not UNSIGNED8",
				CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
				CO_NMT_RDN_TTOGGLE_SUBIDX);
		return 0;
	}

	return 1;
}

static int
co_nmt_rdn_chk_dev_sub_ntoggle(const co_obj_t *obj_rdn)
{
	const co_sub_t *sub_rdn_ntoggle =
			co_obj_find_sub(obj_rdn, CO_NMT_RDN_NTOGGLE_SUBIDX);
	if ((sub_rdn_ntoggle != NULL)
			&& (co_sub_get_type(sub_rdn_ntoggle)
					!= CO_DEFTYPE_UNSIGNED8)) {
		diag(DIAG_ERROR, 0, "NMT: object %04X:%02X is not UNSIGNED8",
				CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
				CO_NMT_RDN_NTOGGLE_SUBIDX);
		return 0;
	}

	return 1;
}

static int
co_nmt_rdn_chk_dev_sub_ctoggle(const co_obj_t *obj_rdn)
{
	const co_sub_t *sub_rdn_ctoggle =
			co_obj_find_sub(obj_rdn, CO_NMT_RDN_CTOGGLE_SUBIDX);
	if ((sub_rdn_ctoggle != NULL)
			&& (co_sub_get_type(sub_rdn_ctoggle)
					!= CO_DEFTYPE_UNSIGNED8)) {
		diag(DIAG_ERROR, 0, "NMT: object %04X:%02X is not UNSIGNED8",
				CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
				CO_NMT_RDN_CTOGGLE_SUBIDX);
		return 0;
	}

	return 1;
}
