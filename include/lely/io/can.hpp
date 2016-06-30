/*!\file
 * This header file is part of the I/O library; it contains the C++ interface of
 * the Controller Area Network (CAN) device handle. \see lely/io/can.h for the C
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

#ifndef LELY_IO_CAN_HPP
#define LELY_IO_CAN_HPP

#ifndef __cplusplus
#error "include <lely/io/can.h> for the C interface"
#endif

#include <lely/io/can.h>
#include <lely/io/io.hpp>

namespace lely {

//! A Controller Area Network (CAN) device handle.
class IOCAN: public IOHandle {
public:
	IOCAN(const char* path)
		: IOHandle(io_open_can(path))
	{
		if (!operator bool())
			throw_or_abort(bad_init());
	}

	IOCAN(const IOCAN& can) noexcept : IOHandle(can) {}

#if __cplusplus >= 201103L
	IOCAN(IOCAN&& can) noexcept
		: IOHandle(::std::forward<IOCAN>(can))
	{}
#endif

	IOCAN&
	operator=(const IOCAN& can) noexcept
	{
		IOHandle::operator=(can);
		return *this;
	}

#if __cplusplus >= 201103L
	IOCAN&
	operator=(IOCAN&& can) noexcept
	{
		IOHandle::operator=(::std::forward<IOCAN>(can));
		return *this;
	}
#endif

	int read(can_msg& msg) noexcept { return io_can_read(*this, &msg); }

	int
	write(const can_msg& msg) noexcept
	{
		return io_can_write(*this, &msg);
	}
};

} //lely

#endif

