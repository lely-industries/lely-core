inc =

inc += lely/libc/sys/types.h
inc += lely/libc/assert.h
inc += lely/libc/features.h
inc += lely/libc/libc.h
inc += lely/libc/signal.h
inc += lely/libc/stdalign.h
inc += lely/libc/stdatomic.h
inc += lely/libc/stddef.h
inc += lely/libc/stdint.h
inc += lely/libc/stdio.h
inc += lely/libc/stdlib.h
inc += lely/libc/stdnoreturn.h
inc += lely/libc/string.h
inc += lely/libc/strings.h
if !NO_THREADS
inc += lely/libc/threads.h
endif
inc += lely/libc/time.h
inc += lely/libc/uchar.h
inc += lely/libc/unistd.h

inc += lely/tap/tap.h

inc += lely/util/def/type.def
inc += lely/util/bimap.h
inc += lely/util/bits.h
inc += lely/util/bitset.h
if !NO_CXX
inc += lely/util/c_call.hpp
inc += lely/util/c_factory.hpp
inc += lely/util/c_type.hpp
endif
inc += lely/util/config.h
inc += lely/util/cmp.h
inc += lely/util/cpp.h
if !NO_DAEMON
inc += lely/util/daemon.h
endif
inc += lely/util/diag.h
inc += lely/util/endian.h
inc += lely/util/errnum.h
if !NO_CXX
inc += lely/util/exception.hpp
endif
inc += lely/util/factory.h
if !NO_CXX
inc += lely/util/factory.hpp
endif
inc += lely/util/frbuf.h
inc += lely/util/fwbuf.h
inc += lely/util/hash.h
inc += lely/util/htab.h
inc += lely/util/lex.h
inc += lely/util/list.h
inc += lely/util/membuf.h
inc += lely/util/pheap.h
inc += lely/util/pool.h
inc += lely/util/print.h
inc += lely/util/rand.h
if !NO_CXX
inc += lely/util/rand.hpp
endif
inc += lely/util/rbtree.h
inc += lely/util/shlib.h
inc += lely/util/time.h
inc += lely/util/util.h
inc += lely/util/xtime.h

inc += lely/can/buf.h
if !NO_CXX
inc += lely/can/buf.hpp
endif
inc += lely/can/can.h
inc += lely/can/msg.h
inc += lely/can/net.h
if !NO_CXX
inc += lely/can/net.hpp
endif
if HAVE_SOCKET_CAN
inc += lely/can/socket.h
endif
if HAVE_VCI
inc += lely/can/vci.h
endif

nobase_include_HEADERS = $(inc)
