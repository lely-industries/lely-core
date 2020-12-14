/**@file
 * This header file is part of the utilities library; it contains the C to C++
 * interface declarations.
 *
 * @copyright 2017-2020 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#ifndef LELY_UTIL_C_TYPE_HPP_
#define LELY_UTIL_C_TYPE_HPP_

#include <lely/util/exception.hpp>

#include <algorithm>
#include <memory>
#include <new>
#include <utility>

namespace lely {

/**
 * The type of objects thrown as exceptions to report a failure to initialize an
 * instantiation of a C type.
 */
class bad_init : public error {};

/**
 * The type of objects thrown as exceptions to report a failure to copy an
 * instantiation of a C type.
 */
class bad_copy : public error {};

/**
 * The type of objects thrown as exceptions to report a failure to move an
 * instantiation of a C type.
 */
class bad_move : public error {};

namespace impl {

inline void
throw_bad_init() {
  if (get_errnum() == ERRNUM_NOMEM) throw_or_abort(::std::bad_alloc());
  throw_or_abort(bad_init());
}

inline void
throw_bad_copy() {
  if (get_errnum() == ERRNUM_NOMEM) throw_or_abort(::std::bad_alloc());
  throw_or_abort(bad_copy());
}

inline void
throw_bad_move() {
  if (get_errnum() == ERRNUM_NOMEM) throw_or_abort(::std::bad_alloc());
  throw_or_abort(bad_move());
}

}  // namespace impl

/// The deleter for trivial, standard layout and incomplete C types.
template <class T>
struct delete_c_type {
  constexpr delete_c_type() noexcept = default;
  template <class U>
  delete_c_type(const delete_c_type<U>&) noexcept {}

  void
  operator()(T* p) const {
    destroy(p);
  }
};

/**
 * Creates an instance of a trivial, standard layout or incomplete C type and
 * wraps it in a std::shared_ptr, using #lely::delete_c_type as the deleter.
 */
template <class T, class... Args>
inline ::std::shared_ptr<T>
make_shared_c(Args&&... args) {
  return ::std::shared_ptr<T>(new T(::std::forward<Args>(args)...),
                              delete_c_type<T>());
}

/**
 * A specialization of std::unique_ptr for trivial, standard layout or
 * incomplete C types, using #lely::delete_c_type as the deleter.
 */
template <class T>
using unique_c_ptr = ::std::unique_ptr<T, delete_c_type<T>>;

/**
 * Creates an instance of a trivial, standard layout or incomplete C type and
 * wraps it in a lely::unique_c_ptr.
 */
template <class T, class... Args>
inline unique_c_ptr<T>
make_unique_c(Args&&... args) {
  return unique_c_ptr<T>(new T(::std::forward<Args>(args)...));
}

/**
 * A class template supplying a uniform interface to certain attributes of C
 * types.
 */
template <class T>
struct c_type_traits;

/// The base class for a C++ interface to a trivial C type.
template <class T>
struct trivial_c_type {
  typedef typename c_type_traits<T>::value_type c_value_type;
  typedef typename c_type_traits<T>::reference c_reference;
  typedef typename c_type_traits<T>::const_reference c_const_reference;
  typedef typename c_type_traits<T>::pointer c_pointer;
  typedef typename c_type_traits<T>::const_pointer c_const_pointer;

  operator c_value_type() const noexcept { return c_ref(); }
  operator c_reference() noexcept { return c_ref(); }
  operator c_const_reference() const noexcept { return c_ref(); }

  c_reference
  c_ref() noexcept {
    return *c_ptr();
  }

  c_const_reference
  c_ref() const noexcept {
    return *c_ptr();
  }

  c_pointer
  c_ptr() noexcept {
    return reinterpret_cast<c_pointer>(this);
  }

  c_const_pointer
  c_ptr() const noexcept {
    return reinterpret_cast<c_const_pointer>(this);
  }

  static void
  dtor(trivial_c_type* p) noexcept {
    delete p;
  }

  void
  destroy() noexcept {
    dtor(this);
  }
};

template <class T>
inline void
destroy(trivial_c_type<T>* p) noexcept {
  p->destroy();
}

/// The base class for a C++ interface to a standard layout C type.
template <class T>
class standard_c_type {
 public:
  typedef typename c_type_traits<T>::value_type c_value_type;
  typedef typename c_type_traits<T>::reference c_reference;
  typedef typename c_type_traits<T>::const_reference c_const_reference;
  typedef typename c_type_traits<T>::pointer c_pointer;
  typedef typename c_type_traits<T>::const_pointer c_const_pointer;

  operator c_value_type() const noexcept { return c_ref(); }
  operator c_reference() noexcept { return c_ref(); }
  operator c_const_reference() const noexcept { return c_ref(); }

  c_reference
  c_ref() noexcept {
    return *c_ptr();
  }

  c_const_reference
  c_ref() const noexcept {
    return *c_ptr();
  }

  c_pointer
  c_ptr() noexcept {
    return reinterpret_cast<c_pointer>(this);
  }

  c_const_pointer
  c_ptr() const noexcept {
    return reinterpret_cast<c_const_pointer>(this);
  }

  static void
  dtor(standard_c_type* p) noexcept {
    delete p;
  }

  void
  destroy() noexcept {
    dtor(this);
  }

  standard_c_type&
  operator=(const standard_c_type& val) {
    if (!c_type_traits<T>::copy(c_ptr(), val.c_ptr())) impl::throw_bad_copy();
    return *this;
  }

  standard_c_type&
  operator=(standard_c_type&& val) {
    if (!c_type_traits<T>::move(c_ptr(), val.c_ptr())) impl::throw_bad_move();
    return *this;
  }

 protected:
  template <class... Args>
  explicit standard_c_type(Args&&... args) {
    if (!c_type_traits<T>::init(c_ptr(), std::forward<Args>(args)...))
      impl::throw_bad_init();
  }

  ~standard_c_type() { c_type_traits<T>::fini(c_ptr()); }
};

template <class T>
inline void
destroy(standard_c_type<T>* p) noexcept {
  p->destroy();
}

/**
 * The base class for a C++ interface to an incomplete C type. This class
 * requires a specialization of `c_type_traits<T>`.
 */
template <class T>
class incomplete_c_type {
 public:
  typedef typename c_type_traits<T>::value_type c_value_type;
  typedef typename c_type_traits<T>::reference c_reference;
  typedef typename c_type_traits<T>::const_reference c_const_reference;
  typedef typename c_type_traits<T>::pointer c_pointer;
  typedef typename c_type_traits<T>::const_pointer c_const_pointer;

  operator c_reference() noexcept { return c_ref(); }
  operator c_const_reference() const noexcept { return c_ref(); }

  c_reference
  c_ref() noexcept {
    return *c_ptr();
  }

  c_const_reference
  c_ref() const noexcept {
    return *c_ptr();
  }

  c_pointer
  c_ptr() noexcept {
    return reinterpret_cast<c_pointer>(this);
  }

  c_const_pointer
  c_ptr() const noexcept {
    return reinterpret_cast<c_const_pointer>(this);
  }

  static void
  dtor(incomplete_c_type* p) noexcept {
    delete p;
  }

  void
  destroy() noexcept {
    dtor(this);
  }

  void*
  operator new(std::size_t size) {
    void* ptr = operator new(size, ::std::nothrow);
    if (!ptr) throw_or_abort(std::bad_alloc());
    return ptr;
  }

  void*
  operator new(std::size_t, const ::std::nothrow_t&) noexcept {
    return c_type_traits<T>::alloc();
  }

  void
  operator delete(void* ptr) noexcept {
    operator delete(ptr, ::std::nothrow);
  }

  void
  operator delete(void* ptr, const ::std::nothrow_t&) noexcept {
    c_type_traits<T>::free(ptr);
  }

  incomplete_c_type&
  operator=(const incomplete_c_type& val) {
    if (!c_type_traits<T>::copy(c_ptr(), val.c_ptr())) impl::throw_bad_copy();
    return *this;
  }

  incomplete_c_type&
  operator=(incomplete_c_type&& val) {
    if (!c_type_traits<T>::move(c_ptr(), val.c_ptr())) impl::throw_bad_move();
    return *this;
  }

  void* operator new[](std::size_t) = delete;
  void* operator new[](std::size_t, const ::std::nothrow_t&) = delete;
  void operator delete[](void*) = delete;
  void operator delete[](void*, const ::std::nothrow_t&) = delete;

 protected:
  template <class... Args>
  explicit incomplete_c_type(Args&&... args) {
    if (!c_type_traits<T>::init(c_ptr(), ::std::forward<Args>(args)...))
      impl::throw_bad_init();
  }

  ~incomplete_c_type() { c_type_traits<T>::fini(c_ptr()); }
};

template <class T>
inline void
destroy(incomplete_c_type<T>* p) noexcept {
  p->destroy();
}

/**
 * A class template supplying a uniform interface to certain attributes of
 * trivial and standard layout C types.
 */
template <class T>
struct c_type_traits {
  typedef T value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return operator new(sizeof(T), ::std::nothrow);
  }

  static void
  free(void* ptr) noexcept {
    operator delete(ptr, ::std::nothrow);
  }

  static pointer
  init(pointer p) noexcept {
    return p;
  }

  static pointer
  init(pointer p, const T& val) noexcept {
    return new (static_cast<void*>(p)) T(val);
  }

  static void
  fini(pointer p) noexcept {
    p->~T();
  }

  static pointer
  copy(pointer p1, const_pointer p2) noexcept {
    *p1 = *p2;
    return p1;
  }

  static pointer
  move(pointer p1, pointer p2) noexcept {
    *p1 = ::std::move(*p2);
    return p1;
  }
};

/**
 * A class template supplying a uniform interface to certain attributes of the C
 * type `void`.
 */
template <>
struct c_type_traits<void> {
  typedef void value_type;
  typedef struct {
  } __type;
  typedef __type& reference;
  typedef const __type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return operator new(0, ::std::nothrow);
  }

  static void
  free(void* ptr) noexcept {
    operator delete(ptr, ::std::nothrow);
  }

  static pointer
  init(pointer p) noexcept {
    return p;
  }

  static void
  fini(pointer p) noexcept {
    (void)p;
  }

  static pointer
  copy(pointer p1, const_pointer p2) noexcept {
    (void)p2;

    return p1;
  }

  static pointer
  move(pointer p1, pointer p2) noexcept {
    (void)p2;

    return p1;
  }
};

}  // namespace lely

#endif  // !LELY_UTIL_C_TYPE_HPP_
