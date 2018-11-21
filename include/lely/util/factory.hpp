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

#ifndef LELY_UTIL_FACTORY_HPP
#define LELY_UTIL_FACTORY_HPP

#include <lely/util/util.h>

#include <memory>
#if __cplusplus < 201103L && defined(__GNUC__)
#include <tr1/memory>
#endif

namespace lely {

/// An abstract factory.
template <class T>
class factory {
 public:
  typedef T value_type;

#if __cplusplus >= 201103L
  virtual ~factory() = default;
#else
  virtual ~factory() {}
#endif

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

#if __cplusplus >= 201103L
  virtual ~factory() = default;
#else
  virtual ~factory() {}
#endif

  virtual void destroy(T*) noexcept = 0;

  deleter_type
  get_deleter() const noexcept {
    return deleter_type(this);
  }
};

#if __cplusplus >= 201103L

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

#else

/**
 * An abstract factory for heap-allocated objects that can be constructed
 * without arguments.
 */
template <class R>
class factory<R*()> : public virtual factory<R*> {
 public:
  virtual R* create() = 0;

#if defined(__GNUC__) || defined(_MSC_VER)
  ::std::tr1::shared_ptr<R>
  make_shared() {
    return ::std::tr1::shared_ptr<R>(create(), factory::get_deleter());
  }
#endif
};

/**
 * An abstract factory for heap-allocated objects that can be constructed with
 * one argument.
 */
template <class R, class T0>
class factory<R*(T0)> : public virtual factory<R*> {
 public:
  virtual R* create(T0) = 0;

#if defined(__GNUC__) || defined(_MSC_VER)
  ::std::tr1::shared_ptr<R>
  make_shared(T0 t0) {
    return ::std::tr1::shared_ptr<R>(create(t0), factory::get_deleter());
  }
#endif
};

/**
 * An abstract factory for heap-allocated objects that can be constructed with
 * two arguments.
 */
template <class R, class T0, class T1>
class factory<R*(T0, T1)> : public virtual factory<R*> {
 public:
  virtual R* create(T0, T1) = 0;

#if defined(__GNUC__) || defined(_MSC_VER)
  ::std::tr1::shared_ptr<R>
  make_shared(T0 t0, T1 t1) {
    return ::std::tr1::shared_ptr<R>(create(t0, t1), factory::get_deleter());
  }
#endif
};

/**
 * An abstract factory for heap-allocated objects that can be constructed with
 * three arguments.
 */
template <class R, class T0, class T1, class T2>
class factory<R*(T0, T1, T2)> : public virtual factory<R*> {
 public:
  virtual R* create(T0, T1, T2) = 0;

#if defined(__GNUC__) || defined(_MSC_VER)
  ::std::tr1::shared_ptr<R>
  make_shared(T0 t0, T1 t1, T2 t2) {
    return ::std::tr1::shared_ptr<R>(create(t0, t1, t2),
                                     factory::get_deleter());
  }
#endif
};

/**
 * An abstract factory for heap-allocated objects that can be constructed with
 * four arguments.
 */
template <class R, class T0, class T1, class T2, class T3>
class factory<R*(T0, T1, T2, T3)> : public virtual factory<R*> {
 public:
  virtual R* create(T0, T1, T2, T3) = 0;

#if defined(__GNUC__) || defined(_MSC_VER)
  ::std::tr1::shared_ptr<R>
  make_shared(T0 t0, T1 t1, T2 t2, T3 t3) {
    return ::std::tr1::shared_ptr<R>(create(t0, t1, t2, t3),
                                     factory::get_deleter());
  }
#endif
};

/**
 * An abstract factory for heap-allocated objects that can be constructed with
 * five arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4>
class factory<R*(T0, T1, T2, T3, T4)> : public virtual factory<R*> {
 public:
  virtual R* create(T0, T1, T2, T3, T4) = 0;

#if defined(__GNUC__) || defined(_MSC_VER)
  ::std::tr1::shared_ptr<R>
  make_shared(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4) {
    return ::std::tr1::shared_ptr<R>(create(t0, t1, t2, t3, t4),
                                     factory::get_deleter());
  }
#endif
};

/**
 * An abstract factory for heap-allocated objects that can be constructed with
 * six arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5>
class factory<R*(T0, T1, T2, T3, T4, T5)> : public virtual factory<R*> {
 public:
  virtual R* create(T0, T1, T2, T3, T4, T5) = 0;

#if defined(__GNUC__) || defined(_MSC_VER)
  ::std::tr1::shared_ptr<R>
  make_shared(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5) {
    return ::std::tr1::shared_ptr<R>(create(t0, t1, t2, t3, t4, t5),
                                     factory::get_deleter());
  }
#endif
};

/**
 * An abstract factory for heap-allocated objects that can be constructed with
 * seven arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6>
class factory<R*(T0, T1, T2, T3, T4, T5, T6)> : public virtual factory<R*> {
 public:
  virtual R* create(T0, T1, T2, T3, T4, T5, T6) = 0;

#if defined(__GNUC__) || defined(_MSC_VER)
  ::std::tr1::shared_ptr<R>
  make_shared(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6) {
    return ::std::tr1::shared_ptr<R>(create(t0, t1, t2, t3, t4, t5, t6),
                                     factory::get_deleter());
  }
#endif
};

/**
 * An abstract factory for heap-allocated objects that can be constructed with
 * eight arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7>
class factory<R*(T0, T1, T2, T3, T4, T5, T6, T7)> : public virtual factory<R*> {
 public:
  virtual R* create(T0, T1, T2, T3, T4, T5, T6, T7) = 0;

#if defined(__GNUC__) || defined(_MSC_VER)
  ::std::tr1::shared_ptr<R>
  make_shared(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7) {
    return ::std::tr1::shared_ptr<R>(create(t0, t1, t2, t3, t4, t5, t6, t7),
                                     factory::get_deleter());
  }
#endif
};

/**
 * An abstract factory for heap-allocated objects that can be constructed with
 * nine arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class T8>
class factory<R*(T0, T1, T2, T3, T4, T5, T6, T7, T8)>
    : public virtual factory<R*> {
 public:
  virtual R* create(T0, T1, T2, T3, T4, T5, T6, T7, T8) = 0;

#if defined(__GNUC__) || defined(_MSC_VER)
  ::std::tr1::shared_ptr<R>
  make_shared(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8) {
    return ::std::tr1::shared_ptr<R>(create(t0, t1, t2, t3, t4, t5, t6, t7, t8),
                                     factory::get_deleter());
  }
#endif
};

/**
 * An abstract factory for heap-allocated objects that can be constructed with
 * ten arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class T8, class T9>
class factory<R*(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9)>
    : public virtual factory<R*> {
 public:
  virtual R* create(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9) = 0;

#if defined(__GNUC__) || defined(_MSC_VER)
  ::std::tr1::shared_ptr<R>
  make_shared(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8,
              T9 t9) {
    return ::std::tr1::shared_ptr<R>(
        create(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9), factory::get_deleter());
  }
#endif
};

#endif  // __cplusplus >= 201103L

namespace impl {

/// Removes the arguments from the given function type.
template <class T>
struct remove_arguments {
  typedef T type;
};

#if __cplusplus >= 201103L

/// Removes the arguments from the given function type.
template <class R, class... Args>
struct remove_arguments<R(Args...)> {
  using type = R;
};

#else

/// Removes the arguments from the given function type.
template <class R>
struct remove_arguments<R()> {
  typedef R type;
};

/// Removes the arguments from the given function type.
template <class R, class T0>
struct remove_arguments<R(T0)> {
  typedef R type;
};

/// Removes the arguments from the given function type.
template <class R, class T0, class T1>
struct remove_arguments<R(T0, T1)> {
  typedef R type;
};

/// Removes the arguments from the given function type.
template <class R, class T0, class T1, class T2>
struct remove_arguments<R(T0, T1, T2)> {
  typedef R type;
};

/// Removes the arguments from the given function type.
template <class R, class T0, class T1, class T2, class T3>
struct remove_arguments<R(T0, T1, T2, T3)> {
  typedef R type;
};

/// Removes the arguments from the given function type.
template <class R, class T0, class T1, class T2, class T3, class T4>
struct remove_arguments<R(T0, T1, T2, T3, T4)> {
  typedef R type;
};

/// Removes the arguments from the given function type.
template <class R, class T0, class T1, class T2, class T3, class T4, class T5>
struct remove_arguments<R(T0, T1, T2, T3, T4, T5)> {
  typedef R type;
};

/// Removes the arguments from the given function type.
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6>
struct remove_arguments<R(T0, T1, T2, T3, T4, T5, T6)> {
  typedef R type;
};

/// Removes the arguments from the given function type.
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7>
struct remove_arguments<R(T0, T1, T2, T3, T4, T5, T6, T7)> {
  typedef R type;
};

/// Removes the arguments from the given function type.
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class T8>
struct remove_arguments<R(T0, T1, T2, T3, T4, T5, T6, T7, T8)> {
  typedef R type;
};

/// Removes the arguments from the given function type.
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class T8, class T9>
struct remove_arguments<R(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9)> {
  typedef R type;
};

#endif  // __cplusplus >= 201103L

}  // namespace impl

/// The default factory.
template <class, class>
class default_factory;

/// The default factory for heap-allocated objects.
template <class T, class U>
class default_factory<T*, U*> : public virtual factory<U*> {
 public:
  virtual void
#if __cplusplus >= 201103L
  destroy(U* p) noexcept override
#else
  destroy(U* p)
#endif
  {
    delete p;
  }
};

#if __cplusplus >= 201103L

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

#else

/**
 * The default factory for heap-allocated objects that can be constructed
 * without arguments.
 */
template <class R, class U>
class default_factory<R*(), U*> : public default_factory<R*, U*>,
                                  public factory<U*()> {
 public:
  virtual U*
  create() {
    return new R();
  }
};

/**
 * The default factory for heap-allocated objects that can be constructed with
 * one argument.
 */
template <class R, class T0, class U>
class default_factory<R*(T0), U*> : public default_factory<R*, U*>,
                                    public factory<U*(T0)> {
 public:
  virtual U*
  create(T0 t0) {
    return new R(t0);
  }
};

/**
 * The default factory for heap-allocated objects that can be constructed with
 * two arguments.
 */
template <class R, class T0, class T1, class U>
class default_factory<R*(T0, T1), U*> : public default_factory<R*, U*>,
                                        public factory<U*(T0, T1)> {
 public:
  virtual U*
  create(T0 t0, T1 t1) {
    return new R(t0, t1);
  }
};

/**
 * The default factory for heap-allocated objects that can be constructed with
 * three arguments.
 */
template <class R, class T0, class T1, class T2, class U>
class default_factory<R*(T0, T1, T2), U*> : public default_factory<R*, U>,
                                            public factory<U*(T0, T1, T2)> {
 public:
  virtual U*
  create(T0 t0, T1 t1, T2 t2) {
    return new R(t0, t1, t2);
  }
};

/**
 * The default factory for heap-allocated objects that can be constructed with
 * four arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class U>
class default_factory<R*(T0, T1, T2, T3), U*>
    : public default_factory<R*, U>, public factory<U*(T0, T1, T2, T3)> {
 public:
  virtual U*
  create(T0 t0, T1 t1, T2 t2, T3 t3) {
    return new R(t0, t1, t2, t3);
  }
};

/**
 * The default factory for heap-allocated objects that can be constructed with
 * five arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class U>
class default_factory<R*(T0, T1, T2, T3, T4), U*>
    : public default_factory<R*, U>, public factory<U*(T0, T1, T2, T3, T4)> {
 public:
  virtual U*
  create(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4) {
    return new R(t0, t1, t2, t3, t4);
  }
};

/**
 * The default factory for heap-allocated objects that can be constructed with
 * six arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class U>
class default_factory<R*(T0, T1, T2, T3, T4, T5), U*>
    : public default_factory<R*, U>,
      public factory<U*(T0, T1, T2, T3, T4, T5)> {
 public:
  virtual U*
  create(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5) {
    return new R(t0, t1, t2, t3, t4, t5);
  }
};

/**
 * The default factory for heap-allocated objects that can be constructed with
 * seven arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class U>
class default_factory<R*(T0, T1, T2, T3, T4, T5, T6), U*>
    : public default_factory<R*, U>,
      public factory<U*(T0, T1, T2, T3, T4, T5, T6)> {
 public:
  virtual U*
  create(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6) {
    return new R(t0, t1, t2, t3, t4, t5, t6);
  }
};

/**
 * The default factory for heap-allocated objects that can be constructed with
 * eight arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class U>
class default_factory<R*(T0, T1, T2, T3, T4, T5, T6, T7), U*>
    : public default_factory<R*, U>,
      public factory<U*(T0, T1, T2, T3, T4, T5, T6, T7)> {
 public:
  virtual U*
  create(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7) {
    return new R(t0, t1, t2, t3, t4, t5, t6, t7);
  }
};

/**
 * The default factory for heap-allocated objects that can be constructed with
 * nine arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class T8, class U>
class default_factory<R*(T0, T1, T2, T3, T4, T5, T6, T7, T8), U*>
    : public default_factory<R*, U>,
      public factory<U*(T0, T1, T2, T3, T4, T5, T6, T7, T8)> {
 public:
  virtual U*
  create(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8) {
    return new R(t0, t1, t2, t3, t4, t5, t6, t7, t8);
  }
};

/**
 * The default factory for heap-allocated objects that can be constructed with
 * ten arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class T8, class T9, class U>
class default_factory<R*(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9), U*>
    : public default_factory<R*, U>,
      public factory<U*(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9)> {
 public:
  virtual U*
  create(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9) {
    return new R(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9);
  }
};

#endif  // __cplusplus >= 201103L

}  // namespace lely

#endif
