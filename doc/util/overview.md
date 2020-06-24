Utilities library overview
==========================

Data structures
---------------

The utilities library implements the following data structures:
- bidirectional map: lely/util/bimap.h
- doubly linked list: lely/util/dllist.h
- pairing heap: lely/util/pheap.h
- red-black tree: lely/util/rbtree.h
- singly linked list: lely/util/sllist.h

All implementations are intrusive. The user must embed a node in the struct
containing the value to be stored. This is generally more performant than a
non-intrusive implementation. Moreover, it is trivial to build a non-intrusive
version on top of the intrusive implementation if necessary.

Numerics
--------

Bit counting and manipulation functions are provided in lely/util/bits.h and
lely/util/bitset.h.

Comparison functions for integers, pointers and strings can be found in
lely/util/cmp.h.

System support
--------------

Functions to keep track of platform-dependent and platform-independent error
numbers can be found in lely/util/errnum.h, as well as functions to convert
error numbers to human-readable strings.

Diagnostic messages can be generated with the functions in lely/util/diag.h.
These functions support user-defined message handlers and several handlers are
predefined for printing diagnostic messages to screen, log files or Windows
dialog boxes.

On POSIX and Windows platforms, functions are provided to run a process in the
background as a daemon/service (see lely/util/daemon.h). Note that this
functionality can be disabled when liblely-util is built.

The byte order of integers and floating-point values is platform specific.
Functions to convert between native, big-endian and little-endian, and network
byte order can be found in lely/util/endian.h.

File and memory management
--------------------------

Accessing files is generally most convenient using memory maps. I/O performance
is improved and reading/writing is simplified, since the user can rely on
pointer manipulation instead of repeated calls to `read()`/`write()` and
`seek()`. Two file buffers are provided, one for reading (lely/util/frbuf.h) and
one for writing (lely/util/fwbuf.h). Both are implemented using native memory
maps, but provide a standard C fallback implementation if necessary. The write
buffer supports atomic access: the final file is only created (or replaced) if
all operations completed successfully.

A lightweight in-memory buffer with a similar API is provided in
lely/util/membuf.h.

Text handling
-------------

According to the Unix philosophy, text streams are the universal interface. To
ease the implementation of parsers, several lexing functions are provided in
lely/util/lex.h. Corresponding printing functions can be found in
lely/util/print.h.

These functions are used to implement the INI parser and printer of the
configuration struct (lely/util/config.h), a simple string-based key-value store
with sections.

C++ support
-----------

Although the Lely libraries are written in C, a C++ interface is often
available. To simplify the task of providing such an interface, wrapper classes
can be found in lely/util/c_type.hpp for trivial, standard layout and incomplete
C types. See [doc/util/object.md](@ref md_doc_util_object) for a template of the
C and C++ interfaces for objects in the Lely libraries.

Callback functions in C typically have a user-provided void pointer as the last
parameter. This can be used to allow, for example, C++ member functions to be
called from C. lely/util/c_call.hpp contains a wrapper that allows any C++
callable to be used as a C callback function. See
[doc/util/callback.md](@ref md_doc_util_callback) for a detailed description of
the C and C++ callback interface.

Other
-----

Convenience functions to compare and manipulate `timespec` structs can be found
in lely/util/time.h. External clock and timer functions, with an API similar to
the POSIX clock and timer functions, can be found in lely/util/xtime.h.
