/*!\file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the synchronization (SYNC) object. See lely/co/sync.h for the C
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

#ifndef LELY_CO_SYNC_HPP
#define LELY_CO_SYNC_HPP

#ifndef __cplusplus
#error "include <lely/co/sync.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/sync.h>

namespace lely {

//! The attributes of #co_sync_t required by #lely::COSync.
template <>
struct c_type_traits<__co_sync> {
	typedef __co_sync value_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;

	static void* alloc() noexcept { return __co_sync_alloc(); }
	static void free(void* ptr) noexcept { __co_sync_free(ptr); }

	static pointer
	init(pointer p, CANNet* net, CODev* dev) noexcept
	{
		return __co_sync_init(p, net, dev);
	}

	static void fini(pointer p) noexcept { __co_sync_fini(p); }
};

//! An opaque CANopen SYNC producer/consumer service type.
class COSync: public incomplete_c_type<__co_sync> {
	typedef incomplete_c_type<__co_sync> c_base;
public:
	COSync(CANNet* net, CODev* dev): c_base(net, dev) {}

	void
	getInd(co_sync_ind_t** pind, void** pdata) const noexcept
	{
		co_sync_get_ind(this, pind, pdata);
	}

	void
	setInd(co_sync_ind_t* ind, void* data) noexcept
	{
		co_sync_set_ind(this, ind, data);
	}

	template <class F>
	void
	setInd(F* f) noexcept
	{
		setInd(&c_call<co_sync_ind_t, F>::function,
				static_cast<void*>(f));
	}

	template <class T, typename c_mem_fn<co_sync_ind_t, T>::type M>
	void
	setInd(T* t) noexcept
	{
		setInd(&c_mem_call<co_sync_ind_t, T, M>::function,
				static_cast<void*>(t));
	}

protected:
	~COSync() {}
};

} // lely

#endif

