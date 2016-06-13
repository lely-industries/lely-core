/*!\file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the Layer Setting Services (LSS) and protocols declarations. See
 * lely/co/lss.h for the C interface.
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

#ifndef LELY_CO_LSS_HPP
#define LELY_CO_LSS_HPP

#ifndef __cplusplus
#error "include <lely/co/lss.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/lss.h>

namespace lely {

//! The attributes of #co_lss_t required by #lely::COLSS.
template <>
struct c_type_traits<__co_lss> {
	typedef __co_lss value_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;

	static void* alloc() noexcept { return __co_lss_alloc(); }
	static void free(void* ptr) noexcept { __co_lss_free(ptr); }

	static pointer
	init(pointer p, CANNet* net, CODev* dev, CONMT* nmt) noexcept
	{
		return __co_lss_init(p, net, dev, nmt);
	}

	static void fini(pointer p) noexcept { __co_lss_fini(p); }
};


//! An opaque CANopen LSS master/slave service type.
class COLSS: public incomplete_c_type<__co_lss> {
	typedef incomplete_c_type<__co_lss> c_base;
public:
	COLSS(CANNet* net, CODev* dev, CONMT* nmt): c_base(net, dev, nmt) {}

	void
	getRateInd(co_lss_rate_ind_t** pind, void** pdata = 0) const noexcept
	{
		co_lss_get_rate_ind(this, pind, pdata);
	}

	void
	setRateInd(co_lss_rate_ind_t* ind, void* data = 0) noexcept
	{
		co_lss_set_rate_ind(this, ind, data);
	}

	void
	getStoreInd(co_lss_store_ind_t** pind, void** pdata = 0) const noexcept
	{
		co_lss_get_store_ind(this, pind, pdata);
	}

	void
	setStoreInd(co_lss_store_ind_t* ind, void* data = 0) noexcept
	{
		co_lss_set_store_ind(this, ind, data);
	}

	int getTimeout() const noexcept { return co_lss_get_timeout(this); }

	void
	setTimeout(int timeout) noexcept
	{
		co_lss_set_timeout(this, timeout);
	}

	bool isMaster() const noexcept { return !!co_lss_is_master(this); }

	bool isIdle() const noexcept { return !!co_lss_is_idle(this); }

	void abortReq() noexcept { co_lss_abort_req(this); }

	int
	switchReq(co_unsigned8_t mode) noexcept
	{
		return co_lss_switch_req(this, mode);
	}


	int
	switchSelReq(const co_id& id, co_lss_cs_ind_t* ind, void* data = 0)
			noexcept
	{
		return co_lss_switch_sel_req(this, &id, ind, data);
	}

	int
	setIdReq(co_unsigned8_t id, co_lss_err_ind_t* ind, void* data = 0)
			noexcept
	{
		return co_lss_set_id_req(this, id, ind, data);
	}

	int
	setRateReq(co_unsigned16_t rate, co_lss_err_ind_t* ind, void* data = 0)
			noexcept
	{
		return co_lss_set_rate_req(this, rate, ind, data);
	}

	int
	switchRateReq(int delay) noexcept
	{
		return co_lss_switch_rate_req(this, delay);
	}

	int
	storeReq(co_lss_err_ind_t* ind, void* data = 0) noexcept
	{
		return co_lss_store_req(this, ind, data);
	}

	int
	getVendorIdReq(co_lss_lssid_ind_t* ind, void* data = 0) noexcept
	{
		return co_lss_get_vendor_id_req(this, ind, data);
	}

	int
	getProductCodeReq(co_lss_lssid_ind_t* ind, void* data = 0) noexcept
	{
		return co_lss_get_product_code_req(this, ind, data);
	}

	int
	getRevisionReq(co_lss_lssid_ind_t* ind, void* data = 0) noexcept
	{
		return co_lss_get_revision_req(this, ind, data);
	}

	int
	getSerialNrReq(co_lss_lssid_ind_t* ind, void* data = 0) noexcept
	{
		return co_lss_get_serial_nr_req(this, ind, data);
	}

	int
	getIdReq(co_lss_nid_ind_t *ind, void *data) noexcept
	{
		return co_lss_get_id_req(this, ind, data);
	}

	int
	idSlaveReq(const co_id& lo, const co_id& hi, co_lss_cs_ind_t* ind,
			void* data = 0) noexcept
	{
		return co_lss_id_slave_req(this, &lo, &hi, ind, data);
	}

	int
	idNonCfgSlaveReq(co_lss_cs_ind_t* ind, void* data = 0) noexcept
	{
		return co_lss_id_non_cfg_slave_req(this, ind, data);
	}

	int
	slowscanReq(const co_id& lo, const co_id& hi, co_lss_scan_ind_t* ind,
			void* data = 0) noexcept
	{
		return co_lss_slowscan_req(this, &lo, &hi, ind, data);
	}

	int
	fastscanReq(const co_id* id, const co_id* mask, co_lss_scan_ind_t* ind,
			void* data = 0) noexcept
	{
		return co_lss_fastscan_req(this, id, mask, ind, data);
	}

protected:
	~COLSS() {}
};

} // lely

#endif

