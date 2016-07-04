/*!\file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the network management (NMT) declarations. See lely/co/nmt.h for
 * the C interface.
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

#ifndef LELY_CO_NMT_HPP
#define LELY_CO_NMT_HPP

#ifndef __cplusplus
#error "include <lely/co/nmt.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/nmt.h>

namespace lely {

//! The attributes of #co_nmt_t required by #lely::CONMT.
template <>
struct c_type_traits<__co_nmt> {
	typedef __co_nmt value_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;

	static void* alloc() noexcept { return __co_nmt_alloc(); }
	static void free(void* ptr) noexcept { __co_nmt_free(ptr); }

	static pointer
	init(pointer p, CANNet* net, CODev* dev) noexcept
	{
		return __co_nmt_init(p, net, dev);
	}

	static void fini(pointer p) noexcept { __co_nmt_fini(p); }
};


//! An opaque CANopen NMT master/slave service type.
class CONMT: public incomplete_c_type<__co_nmt> {
	typedef incomplete_c_type<__co_nmt> c_base;
public:
	CONMT(CANNet* net, CODev* dev): c_base(net, dev) {}

	void
	getCsInd(co_nmt_cs_ind_t** pind, void** pdata = 0) const noexcept
	{
		co_nmt_get_cs_ind(this, pind, pdata);
	}

	void
	setCsInd(co_nmt_cs_ind_t* ind, void* data = 0) noexcept
	{
		co_nmt_set_cs_ind(this, ind, data);
	}

	void
	getEcInd(co_nmt_ec_ind_t** pind, void** pdata = 0) const noexcept
	{
		co_nmt_get_ec_ind(this, pind, pdata);
	}

	void
	setEcInd(co_nmt_ec_ind_t *ind, void* data = 0) noexcept
	{
		co_nmt_set_ec_ind(this, ind, data);
	}

	void
	getStInd(co_nmt_st_ind_t** pind, void** pdata = 0) const noexcept
	{
		co_nmt_get_st_ind(this, pind, pdata);
	}

	void
	setStInd(co_nmt_st_ind_t *ind, void* data = 0) noexcept
	{
		co_nmt_set_st_ind(this, ind, data);
	}

	void
	getLgInd(co_nmt_lg_ind_t** pind, void** pdata = 0) const noexcept
	{
		co_nmt_get_lg_ind(this, pind, pdata);
	}

	void
	setLgInd(co_nmt_lg_ind_t *ind, void* data = 0) noexcept
	{
		co_nmt_set_lg_ind(this, ind, data);
	}

	void
	getBootInd(co_nmt_boot_ind_t** pind, void** pdata = 0) const noexcept
	{
		co_nmt_get_boot_ind(this, pind, pdata);
	}

	void
	setBootInd(co_nmt_boot_ind_t *ind, void* data = 0) noexcept
	{
		co_nmt_set_boot_ind(this, ind, data);
	}

	void
	getCfgInd(co_nmt_cfg_ind_t** pind, void** pdata = 0) const noexcept
	{
		co_nmt_get_cfg_ind(this, pind, pdata);
	}

	void
	setCfgInd(co_nmt_cfg_ind_t *ind, void* data = 0) noexcept
	{
		co_nmt_set_cfg_ind(this, ind, data);
	}

	co_unsigned8_t getId() const noexcept { return co_nmt_get_id(this); }

	int
	setId(co_unsigned8_t id) noexcept
	{
		return co_nmt_set_id(this, id);
	}

	co_unsigned8_t getSt() const noexcept { return co_nmt_get_st(this); }

	bool isMaster() const noexcept { return !!co_nmt_is_master(this); }

	int
	csReq(co_unsigned8_t cs, co_unsigned8_t id = 0) noexcept
	{
		return co_nmt_cs_req(this, cs, id);
	}

	int
	bootReq(co_unsigned8_t id, int timeout) noexcept
	{
		return co_nmt_boot_req(this, id, timeout);
	}

	int
	cfgReq(co_unsigned8_t id, int timeout, co_nmt_cfg_con_t* con,
			void* data = 0) noexcept
	{
		return co_nmt_cfg_req(this, id, timeout, con, data);
	}

	int
	cfgRes(co_unsigned8_t id, co_unsigned32_t ac) noexcept
	{
		return co_nmt_cfg_res(this, id, ac);
	}

	int
	csInd(co_unsigned8_t cs) noexcept
	{
		return co_nmt_cs_ind(this, cs);
	}

	void commErrInd() noexcept { co_nmt_comm_err_ind(this); }

	int
	nodeErrInd(co_unsigned8_t id) noexcept
	{
		return co_nmt_node_err_ind(this, id);
	}

	CORPDO*
	getRPDO(co_unsigned16_t n) const noexcept
	{
		return co_nmt_get_rpdo(this, n);
	}

	COTPDO*
	getTPDO(co_unsigned16_t n) const noexcept
	{
		return co_nmt_get_tpdo(this, n);
	}

	COSSDO*
	getSSDO(co_unsigned8_t n) const noexcept
	{
		return co_nmt_get_ssdo(this, n);
	}

	COCSDO*
	getCSDO(co_unsigned8_t n) const noexcept
	{
		return co_nmt_get_csdo(this, n);
	}

	COSync* getSync() const noexcept { return co_nmt_get_sync(this); }

	COTime* getTime() const noexcept { return co_nmt_get_time(this); }

	COEmcy* getEmcy() const noexcept { return co_nmt_get_emcy(this); }

	COLSS* getLSS() const noexcept { return co_nmt_get_lss(this); }

protected:
	~CONMT() {}
};

} // lely

#endif

