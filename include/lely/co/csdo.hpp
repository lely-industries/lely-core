/*!\file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the Client-SDO declarations. See lely/co/csdo.h for the C
 * interface.
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

#ifndef LELY_CO_CSDO_HPP
#define LELY_CO_CSDO_HPP

#ifndef __cplusplus
#error "include <lely/co/csdo.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/csdo.h>
#include <lely/co/val.hpp>

namespace lely {

//! The attributes of #co_csdo_t required by #lely::COCSDO.
template <>
struct c_type_traits<__co_csdo> {
	typedef __co_csdo value_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;

	static void* alloc() noexcept { return __co_csdo_alloc(); }
	static void free(void* ptr) noexcept { __co_csdo_free(ptr); }

	static pointer
	init(pointer p, CANNet* net, CODev* dev, co_unsigned8_t num) noexcept
	{
		return __co_csdo_init(p, net, dev, num);
	}

	static void fini(pointer p) noexcept { __co_csdo_fini(p); }
};

//! An opaque CANopen Client-SDO service type.
class COCSDO: public incomplete_c_type<__co_csdo> {
	typedef incomplete_c_type<__co_csdo> c_base;
public:
	COCSDO(CANNet* net, CODev* dev, co_unsigned8_t num)
		: c_base(net, dev, num)
	{}

	co_unsigned8_t getNum() const noexcept { return co_csdo_get_num(this); }

	const co_sdo_par&
	getPar() const noexcept
	{
		return *co_csdo_get_par(this);
	}

	int getTimeout() const noexcept { return co_csdo_get_timeout(this); }

	void
	setTimeout(int timeout) noexcept
	{
		co_csdo_set_timeout(this, timeout);
	}

	bool isIdle() const noexcept { return !!co_csdo_is_idle(this); }

	void
	abortReq(co_unsigned32_t ac = CO_SDO_AC_ERROR) noexcept
	{
		co_csdo_abort_req(this, ac);
	}

	int
	dnReq(co_unsigned16_t idx, co_unsigned8_t subidx, const void* ptr,
			size_t n, co_csdo_dn_con_t* con, void* data = 0)
			noexcept
	{
		return co_csdo_dn_req(this, idx, subidx, ptr, n, con, data);
	}

	template <co_unsigned16_t N>
	int
	dnReq(co_unsigned16_t idx, co_unsigned8_t subidx, const COVal<N>& val,
			co_csdo_dn_con_t* con, void* data = 0) noexcept
	{
		return co_csdo_dn_val_req(this, idx, subidx, N, &val, con,
				data);
	}

	int
	upReq(co_unsigned16_t idx, co_unsigned8_t subidx,
			co_csdo_up_con_t* con, void* data = 0) noexcept
	{
		return co_csdo_up_req(this, idx, subidx, con, data);
	}

	int
	blkDnReq(co_unsigned16_t idx, co_unsigned8_t subidx, const void* ptr,
			size_t n, co_csdo_dn_con_t* con, void* data = 0)
			noexcept
	{
		return co_csdo_blk_dn_req(this, idx, subidx, ptr, n, con, data);
	}

	int
	blkUpReq(co_unsigned16_t idx, co_unsigned8_t subidx, uint8_t pst,
			co_csdo_up_con_t* con, void* data = 0) noexcept
	{
		return co_csdo_blk_up_req(this, idx, subidx, pst, con, data);
	}

protected:
	~COCSDO() {}
};

} // lely

#endif

