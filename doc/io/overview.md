I/O library overview
====================

All I/O devices are represented by the same handle (#io_handle_t,
see lely/io/io.h), providing a uniform interface. This handle is a
reference-counted wrapper around a platform-specific file descriptor or handle.
A C++ interface for I/O device handles can be found in lely/io/io.hpp.

The following I/O devices are supported:
- CAN devices (lely/io/can.h, lely/io/can.hpp)
- regular files (lely/io/file.h, lely/io/file.hpp)
- pipes (lely/io/pipe.h, lely/io/pipe.hpp)
- serial I/O devices (lely/io/serial.h, lely/io/serial.hpp)
- network sockets (lely/io/sock.h, lely/io/sock.hpp)

Additionally, serial I/O device attributes can be manipulated with the functions
in lely/io/attr.h, while network addresses and interfaces can be queried and
manipulated with the functions in lely/io/addr.h and lely/io/if.h, respectively.

Depending on the platform, some or all of the I/O devices can be polled for
events. Combined with non-blocking access, this allows an application to use the
reactor pattern to concurrently handle multiple I/O channels. The polling
interface is provided by lely/io/poll.h (see lely/io/poll.hpp for the C++
interface).

