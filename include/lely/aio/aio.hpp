/*!\file
 * This is the public header file of the C++ asynchronous I/O library.
 *
 * \copyright 2018 Lely Industries N.V.
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

#ifndef LELY_AIO_AIO_HPP_
#define LELY_AIO_AIO_HPP_

#include <lely/aio/aio.h>

#if defined(__cplusplus) && __cplusplus < 201103L
#error This file requires compiler and library support for the ISO C++11 standard.
#endif

#include <cstddef>

//! Global namespace for the Lely Industries N.V. libraries.
namespace lely {

//! Namespace for the C++ asynchronous I/O library.
namespace aio {

//! Namespace for implementation details of the C++ asynchronous I/O library.
namespace detail {}

class ContextBase;

class LoopBase;

class FutureBase;

template <class> class Future;

class ReactorBase;

}  // namespace lely

}  // namespace lely

#endif  // LELY_AIO_AIO_HPP_
