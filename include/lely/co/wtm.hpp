/*!\file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the Wireless Transmission Media (WTM) declarations. See
 * lely/co/wtm.h for the C interface.
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

#ifndef LELY_CO_WTM_HPP
#define LELY_CO_WTM_HPP

#ifndef __cplusplus
#error "include <lely/co/wtm.h> for the C interface"
#endif

#include <lely/util/c_type.hpp>

namespace lely { class COWTM; }
//! An opaque CANopen Wireless Transmission Media (WTM) interface type.
typedef lely::COWTM co_wtm_t;

#include <lely/co/wtm.h>

namespace lely {

//! The attributes of #co_wtm_t required by #lely::COWTM.
template <>
struct c_type_traits<__co_wtm> {
	typedef __co_wtm value_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;

	static void* alloc() noexcept { return __co_wtm_alloc(); }
	static void free(void* ptr) noexcept { __co_wtm_free(ptr); }

	static pointer init(pointer p) noexcept { return __co_wtm_init(p); }
	static void fini(pointer p) noexcept { __co_wtm_fini(p); }
};

//! An opaque CANopen Wireless Transmission Media (WTM) interface type.
class COWTM: public incomplete_c_type<__co_wtm> {
	typedef incomplete_c_type<__co_wtm> c_base;
public:
	COWTM(): c_base() {}

	uint8_t getNIF() const noexcept { return co_wtm_get_nif(this); }

	int
	setNIF(uint8_t nif = 1) noexcept
	{
		return co_wtm_set_nif(this, nif);
	}

	void
	getDiagFunc(co_wtm_diag_func_t** pfunc, void** pdata = 0) const noexcept
	{
		co_wtm_get_diag_func(this, pfunc, pdata);
	}

	void
	setDiagFunc(co_wtm_diag_func_t* func, void* data = 0) noexcept
	{
		co_wtm_set_diag_func(this, func, data);
	}

	void
	recv(const void* buf, size_t nbytes) noexcept
	{
		co_wtm_recv(this, buf, nbytes);
	}

	void
	getRecvFunc(co_wtm_recv_func_t** pfunc, void** pdata = 0) const noexcept
	{
		co_wtm_get_recv_func(this, pfunc, pdata);
	}

	void
	setRecvFunc(co_wtm_recv_func_t* func, void* data = 0) noexcept
	{
		co_wtm_set_recv_func(this, func, data);
	}

	int
	getTime(uint8_t nif, timespec* tp) const noexcept
	{
		return co_wtm_get_time(this, nif, tp);
	}

	int
	setTime(uint8_t nif, const timespec& tp) noexcept
	{
		return co_wtm_set_time(this, nif, &tp);
	}

	int
	send(uint8_t nif, const can_msg& msg) noexcept
	{
		return co_wtm_send(this, nif, &msg);
	}

	int sendAlive() noexcept { return co_wtm_send_alive(this); }

	int
	sendAbort(uint32_t ac) noexcept
	{
		return co_wtm_send_abort(this, ac);
	}

	int flush() noexcept { return co_wtm_flush(this); }

	void
	getSendFunc(co_wtm_send_func_t** pfunc, void** pdata = 0) const noexcept
	{
		co_wtm_get_send_func(this, pfunc, pdata);
	}

	void
	setSendFunc(co_wtm_send_func_t* func, void* data = 0) noexcept
	{
		co_wtm_set_send_func(this, func, data);
	}

protected:
	~COWTM() {}
};

} // lely

#endif

