/*!\file
 * This is the public header file of the C++ CANopen application library.
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

#ifndef LELY_COAPP_COAPP_HPP_
#define LELY_COAPP_COAPP_HPP_

#if defined(__cplusplus) && __cplusplus < 201103L
#error This file requires compiler and library support for the ISO C++11 standard.
#endif

#include <lely/libc/features.h>

#ifndef LELY_COAPP_EXTERN
#ifdef LELY_COAPP_INTERN
#define LELY_COAPP_EXTERN LELY_DLL_EXPORT
#else
#define LELY_COAPP_EXTERN LELY_DLL_IMPORT
#endif
#endif

//! Global namespace for the Lely Industries N.V. libraries.
namespace lely {

//! Namespace for the C++ CANopen application library.
namespace canopen {

//! An abstract interface conforming to the BasicLockable concept.
class BasicLockable {
 public:
  /*!
   * Blocks until a lock can be obtained for the current execution agent
   * (thread, process, task). If an exception is thrown, no lock is obtained.
   */
  virtual void lock() = 0;

  //! Releases the lock held by the execution agent. Throws no exceptions.
  virtual void unlock() = 0;

 protected:
  ~BasicLockable() = default;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_COAPP_HPP_
