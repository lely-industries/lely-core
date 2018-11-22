/**@file
 * This header file is part of the utilities library; it contains the C++
 * interface of the factory pattern.
 *
 * @copyright 2017-2018 Lely Industries N.V.
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

#ifndef LELY_UTIL_FACTORY_HPP_
#define LELY_UTIL_FACTORY_HPP_

#include <lely/util/util.h>

#include <memory>

namespace lely {

/// An abstract factory.
template <class T>
class factory {
 public:
  typedef T value_type;

  virtual ~factory() = default;

  virtual void destroy(T) = 0;
};

/// An abstract factory for heap-allocated objects.
template <class T>
class factory<T*> {
 public:
  typedef T* value_type;

  /**
   * The deleter used by `shared_ptr` and `unique_ptr` to delete objects
   * created with this factory.
   */
  class deleter_type {
    friend class factory;

   public:
    void
    operator()(T* p) const noexcept {
      m_f.destroy(p);
    }

   private:
    deleter_type(factory& f) : m_f(f) {}

    factory& m_f;
  };

  virtual ~factory() = default;

  virtual void destroy(T*) noexcept = 0;

  deleter_type
  get_deleter() const noexcept {
    return deleter_type(this);
  }
};

/**
 * An abstract factory for heap-allocated objects that can be constructed with
 * an arbitrary number of arguments.
 */
template <class R, class... Args>
class factory<R*(Args...)> : public virtual factory<R*> {
 public:
  using typename factory<R*>::deleter_type;

  virtual R* create(Args...) = 0;

  ::std::shared_ptr<R>
  make_shared(Args... args) {
    return ::std::shared_ptr<R>(create(args...), factory::get_deleter());
  }

  ::std::unique_ptr<R, deleter_type>
  make_unique(Args... args) {
    return ::std::unique_ptr<R, deleter_type>(create(args...),
                                              factory::get_deleter());
  }
};

namespace impl {

/// Removes the arguments from the given function type.
template <class T>
struct remove_arguments {
  typedef T type;
};

/// Removes the arguments from the given function type.
template <class R, class... Args>
struct remove_arguments<R(Args...)> {
  using type = R;
};

}  // namespace impl

/// The default factory.
template <class, class>
class default_factory;

/// The default factory for heap-allocated objects.
template <class T, class U>
class default_factory<T*, U*> : public virtual factory<U*> {
 public:
  virtual void
  destroy(U* p) noexcept override {
    delete p;
  }
};

/**
 * The default factory for heap-allocated objects that can be constructed with
 * an arbitrary number of arguments.
 */
template <class R, class... Args, class U>
class default_factory<R*(Args...), U*> : public default_factory<R*, U*>,
                                         public factory<U*(Args...)> {
 public:
  virtual U*
  create(Args... args) override {
    return new R(args...);
  }
};

}  // namespace lely

#endif  // !LELY_UTIL_FACTORY_HPP_
