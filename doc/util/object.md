Object interface
================

Objects in the Lely libraries are all defined and implemented according to the
pattern shown here. This pattern is designed to allow a header-only C++
interface for C objects which is as close as possible to a native C++ interface.

Example of a public C header (lely/lib/obj.h):
~~~{.c}
#ifndef LELY_LIB_OBJ_H_
#define LELY_LIB_OBJ_H_

// The obj_t typedef is only defined for C. This allows the C++ interface to
// define its own typedef but still call all C functions without requiring a
// cast.
struct __obj;
#ifndef __cplusplus
typedef struct __obj obj_t;
#endif

#ifdef __cplusplus
// Disable C++ name mangling and ensure the C calling convention is used.
extern "C" {
#endif

// In C++, memory allocation and object initialization are separate steps. The
// C implementation of these steps is exposed so they can be used by the C++
// interface. Users of the C interface SHOULD not invoke these functions
// directly, but use obj_create() and obj_destroy() instead.
void *__obj_alloc(void);
void __obj_free(void *ptr);
struct __obj *__obj_init(struct __obj *obj, Args... args);
void __obj_fini(struct __obj *obj);

// Creates a new object instance. Internally, this function invokes
// __obj_alloc() followed by __obj_init().
obj_t *obj_create(Args... args);

// Destroys an object instance. Internally, this function invokes __obj_fini()
// followed by obj_free().
void obj_destroy(obj_t *obj);

// An example of an object method. The first parameter is always a pointer to
// the object (cf. "this" in C++).
int obj_method(obj_t *obj, Args... args);

#ifdef __cplusplus
}
#endif

#endif // !LELY_LIB_OBJ_H_
~~~

Example of the C implementation (obj.c):
~~~{.c}
#include <lely/util/errnum.h>
#include <lely/lib/obj.h>

#include <assert.h>
#include <stdlib.h>

struct __obj {
	...
};

void *
__obj_alloc(void)
{
	void *ptr = malloc(sizeof(struct __obj));
	if (!ptr)
		// On POSIX platforms this is a no-op (errno = errno), but on
		// Windows this converts errno into a system error code and
		// invokes SetLastError().
		set_errno(errno);
	return ptr;
}

void
__obj_free(void *ptr)
{
	free(ptr);
}

struct __obj *
__obj_init(struct __obj *obj, Args... args)
{
	assert(obj);

	// Initialize all object members. If an error occurs, clean up and
	// return NULL.
	...

	return obj;
}

void
__obj_fini(struct __obj *obj)
{
	assert(obj);

	// Finalize all object members.
	...;
}

obj_t *
obj_create(Args... args)
{
	errc_t errc = 0;

	obj_t *obj = __obj_alloc();
	if (!obj) {
		errc = get_errc();
		goto error_alloc_obj;
	}

	if (!__obj_init(obj, args...)) {
		errc = get_errc();
		goto error_init_obj;
	}

	return obj;

error_init_obj:
	__obj_free(obj);
error_alloc_obj:
	set_errc(errc);
	return NULL;
}

void
obj_destroy(obj_t *obj)
{
	// obj_destroy() and obj_free() are the only methods which can be called
	// with `obj == NULL`.
	if (obj) {
		__obj_fini(obj);
		__obj_free(obj);
	}
}

int
obj_method(obj_t *obj, Args... args)
{
	assert(obj);

	// Do something with the object.
	...
}
~~~

Example of a public C++ header (lely/lib/obj.hpp):
~~~{.cpp}
#ifndef LELY_LIB_OBJ_HPP_
#define LELY_LIB_OBJ_HPP_

#ifndef __cplusplus
#error "include <lely/lib/obj.h> for the C interface"
#endif

// Include the incomplete_c_type<> and c_type_traits<> templates.
#include <lely/util/c_type.hpp>

// Provide the obj_t typedef. This is necessary to include lely/lib/obj.h
// without errors.
namespace lely { class Obj; }
typedef lely::Obj obj_t;

#include <lely/lib/obj.h>

namespace lely {

// The c_type_traits<> template specialization provides a uniform wrapper for
// the the alloc(), free(), init() and fini() methods from the C interface.
// These methods are invoked by incomplete_c_type<> from which the C++ interface
// inherits.
template <>
struct c_type_traits<__obj> {
	typedef __obj value_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;

	static void* alloc() noexcept { return __obj_alloc(); }
	static void free(void* ptr) noexcept { __obj_free(ptr); }

	static pointer
	init(pointer p, Args... args) noexcept
	{
		return __obj_init(p, args...);
	}

	static void fini(pointer p) noexcept { __obj_fini(p); }
};

class Obj: public incomplete_c_type<__obj> {
	typedef incomplete_c_type<__obj> c_base;
public:
	// c_base calls the alloc() and init() methods of c_type_traits<__obj>
	// that construct the object.
	Obj(Args... args): c_base(args...) {}

	int
	method(Args... args) noexcept
	{
		// Due to the (re)definition of obj_t we can pass "this" as the
		// first argument.
		return obj_method(this, args...);
	}

protected:
	// The destructor is protected to prevent the creation of an instance on
	// the heap. This would lead to errors, since the size of the object is
	// unknown.
	~Obj() {}
};

} // lely

#endif  // !LELY_LIB_OBJ_HPP_
~~~

Defining the destructor as `protected` prevents allocating objects on the stack.
Unfortunately, it also prevents invoking `delete` on heap-allocated objects.
Instead, objects must be destroy by invoking `destroy()`, either as a member
function (`obj->destroy()`) or as a global function (`destroy(obj)`).

From C++11 onwards, using raw pointers is discouraged. Shared or unique pointers
are preferred. lely/util/c_type.hpp provides the `make_shared_c()` and
`make_unique_c()` convenience functions for this purpose. They are equivalent to
the standard `make_shared()` and `make_unique()` functions, but specify
`destroy()` as the deleter.

