Callback interface
==================

Many [objects](@ref md_doc_object) in the Lely libraries allow the user to
register callback functions. The function signature and registration method all
follow the pattern shown here.

Example of a public C header (lely/lib/obj.h):
~~~{.c}
#ifdef __cplusplus
// Ensure that callback functions also use the C calling convention.
extern "C" {
#endif

// The last argument of a callback function is always a pointer to
// user-specified data.
typedef void callback_func_t(Args... args, void *data);

// Retrieves a pointer to the callback function and the user-specified data.
LELY_LIB_EXTERN void obj_get_func(const obj_t *obj, callback_func_t **pfunc,
		void **pdata);

// Sets a callback function. The data pointer is passed as the last argument in
// each invocation of func.
LELY_LIB_EXTERN void obj_set_func(obj_t *obj, callback_func_t *func,
		void *data);

#ifdef __cplusplus
}
#endif
~~~

Example of the C implementation (obj.c):
~~~{.c}
struct obj {
	...
	callback_func_t *callback_func;
	void *callback_data;
	...
};

// An internal helper function that invokes the callback if it is set.
static void obj_callback(obj_t *obj, Args... args);

LELY_LIB_EXPORT struct __obj *
__obj_init(struct __obj *obj, Args... args)
{
	...

	obj->callback_func = NULL;
	obj->callback_data = NULL;

	...
}

LELY_LIB_EXPORT void
obj_get_func(const obj_t *obj, callback_func_t **pfunc, void **pdata)
{
	assert(obj);

	if (pfunc)
		*pfunc = obj->callback_func;
	if (pdata)
		*pdata = obj->callback_data;
}

LELY_LIB_EXPORT void
obj_set_func(obj_t *obj, callback_func_t *func, void *data)
{
	assert(obj);

	obj->callback_func = func;
	obj->callback_data = data;
}

static void
obj_callback(obj_t *obj, Args... args)
{
	assert(obj);

	if (obj->callback_func)
		obj->callback_func(args..., obj->callback_data);
}
~~~

The user-specified data pointer can be used to allow the registration of C++
function objects or member functions. lely/util/c_call.hpp provides several
templates to automatically generate the required wrapper functions.

Example of a public C++ header (lely/lib/obj.hpp):
~~~{.cpp}
// Include the c_obj_call<>, c_mem_call<> and c_mem_fn<> templates.
#include <lely/util/c_call.hpp>

namespace lely {

class Obj {
public:
	...

	void
	getFunc(callback_func_t** pfunc, void** pdata) const noexcept
	{
		obj_get_func(this, pfunc, pdata);
	}

	// Registers a C-style callback function. The user specifies the data
	// pointer.
	void
	setFunc(callback_func_t* func, void* data) noexcept
	{
		obj_set_func(this, func, data);
	}

	// Registers a function object as the callback. The data pointer is used
	// to store the address of the function object and is therefore not
	// available to the user.
	template <class F>
	void
	setFunc(F* f) noexcept
	{
		setFunc(&c_obj_call<callback_func_t*, F>::function,
				static_cast<void*>(f));
	}

	// Registers a member function as the callback. The first template
	// parameter is the class containing the member function, the second is
	// the address of the member function. The data pointer is used to store
	// the address of the class instance and is therefore not available to
	// the user.
	template <class C, typename c_mem_fn<callback_func_t*, C>::type M>
	void
	setFunc(C* obj) noexcept
	{
		setFunc(&c_mem_call<callback_func_t*, C, M>::function,
				static_cast<void*>(obj));
	}

	...
};

} // lely
~~~

Registering a C-style global (or static) function in C++ is similar to C:
~~~{.cpp}
void myFunc(Args... args, void* data) noexcept;

void
setFunc(Obj* obj, void* data)
{
	obj->setFunc(&myFunc, data);
}
~~~

The second form of `setFunc()` allows registering a function object. The user is
responsible for lifetime of the object.
~~~{.cpp}
struct MyFuncObj {
	void operator()(Args... args) noexcept;
};

void
setFunc(Obj* obj, MyFuncObj* f)
{
	obj->setFunc(f);
}
~~~

The third form of `setFunc()` allows member functions to be registered as
callbacks. Again, the user is responsible for the lifetime of the instance of
the class containing the member function.
~~~{.cpp}
class MyClass {
public:
	void myFunc(Args... args) noexcept;
};

void
setFunc(Obj* obj, MyClass* cls)
{
	obj->setFunc<MyClass, &MyClass::myFunc>(cls);
}
~~~
It is also possible to register private methods as callbacks, but only from a
function which has private access (i.e., a friend or another method).

