inc =
inc += lely/features.h

inc += lely/libc/sys/types.h
if !NO_CXX
inc += lely/libc/chrono.hpp
inc += lely/libc/functional.hpp
endif
inc += lely/libc/stdatomic.h
inc += lely/libc/stddef.h
inc += lely/libc/stdint.h
inc += lely/libc/stdio.h
inc += lely/libc/stdlib.h
inc += lely/libc/string.h
inc += lely/libc/strings.h
if !NO_THREADS
inc += lely/libc/threads.h
endif
inc += lely/libc/time.h
if !NO_CXX
inc += lely/libc/type_traits.hpp
endif
inc += lely/libc/uchar.h
inc += lely/libc/unistd.h
if !NO_CXX
inc += lely/libc/utility.hpp
endif

inc += lely/tap/tap.h

if !ECSS_COMPLIANCE
inc += lely/util/bimap.h
inc += lely/util/coroutine.h
if !NO_CXX
inc += lely/util/coroutine.hpp
endif
inc += lely/util/mkjmp.h
inc += lely/util/sllist.h
inc += lely/util/spscring.h
endif # !ECSS_COMPLIANCE
inc += lely/util/def/type.def
inc += lely/util/bits.h
if !NO_MALLOC
inc += lely/util/bitset.h
endif
if !NO_CXX
inc += lely/util/c_call.hpp
inc += lely/util/c_type.hpp
inc += lely/util/chrono.hpp
endif
inc += lely/util/cmp.h
if !NO_MALLOC
inc += lely/util/config.h
endif
if !NO_DAEMON
inc += lely/util/daemon.h
endif
inc += lely/util/diag.h
inc += lely/util/dllist.h
inc += lely/util/endian.h
inc += lely/util/errnum.h
if !NO_CXX
inc += lely/util/error.hpp
inc += lely/util/exception.hpp
endif
if !NO_MALLOC
inc += lely/util/fiber.h
if !NO_CXX
inc += lely/util/fiber.hpp
endif
endif # !NO_MALLOC
inc += lely/util/float.h
if !NO_STDIO
inc += lely/util/frbuf.h
inc += lely/util/fwbuf.h
endif
if !NO_CXX
inc += lely/util/invoker.hpp
endif
if !NO_STDIO
inc += lely/util/lex.h
endif
inc += lely/util/membuf.h
if !NO_CXX
inc += lely/util/mutex.hpp
endif
inc += lely/util/pheap.h
if !NO_STDIO
inc += lely/util/print.h
endif
inc += lely/util/rbtree.h
if !NO_CXX
inc += lely/util/result.hpp
endif
if !NO_MALLOC
inc += lely/util/stop.h
if !NO_CXX
inc += lely/util/stop.hpp
endif
endif # !NO_MALLOC
inc += lely/util/time.h
inc += lely/util/ustring.h
inc += lely/util/util.h

inc += lely/can/buf.h
if !NO_CXX
inc += lely/can/buf.hpp
endif
inc += lely/can/err.h
inc += lely/can/msg.h
inc += lely/can/net.h
if !NO_CXX
inc += lely/can/net.hpp
endif
if !NO_STDIO
if HAVE_SOCKET_CAN
inc += lely/can/socket.h
endif
if HAVE_VCI
inc += lely/can/vci.h
endif
endif # !NO_STDIO

if !NO_STDIO
inc += lely/io/addr.h
inc += lely/io/attr.h
inc += lely/io/can.h
if !NO_CXX
inc += lely/io/can.hpp
endif
inc += lely/io/file.h
if !NO_CXX
inc += lely/io/file.hpp
endif
inc += lely/io/if.h
inc += lely/io/io.h
if !NO_CXX
inc += lely/io/io.hpp
endif
inc += lely/io/pipe.h
if !NO_CXX
inc += lely/io/pipe.hpp
endif
inc += lely/io/poll.h
if !NO_CXX
inc += lely/io/poll.hpp
endif
inc += lely/io/serial.h
if !NO_CXX
inc += lely/io/serial.hpp
endif
inc += lely/io/sock.h
if !NO_CXX
inc += lely/io/sock.hpp
endif
endif # !NO_STDIO

if !ECSS_COMPLIANCE
if !NO_CXX
inc += lely/ev/co_task.hpp
endif
inc += lely/ev/ev.h
inc += lely/ev/exec.h
if !NO_CXX
inc += lely/ev/exec.hpp
endif
inc += lely/ev/fiber_exec.h
if !NO_CXX
inc += lely/ev/fiber_exec.hpp
endif
inc += lely/ev/future.h
if !NO_CXX
inc += lely/ev/future.hpp
endif
inc += lely/ev/loop.h
if !NO_CXX
inc += lely/ev/loop.hpp
endif
inc += lely/ev/poll.h
if !NO_CXX
inc += lely/ev/poll.hpp
endif
inc += lely/ev/std_exec.h
inc += lely/ev/strand.h
if !NO_CXX
inc += lely/ev/strand.hpp
endif
inc += lely/ev/task.h
if !NO_CXX
inc += lely/ev/task.hpp
endif
inc += lely/ev/thrd_loop.h
if !NO_CXX
inc += lely/ev/thrd_loop.hpp
endif
endif # !ECSS_COMPLIANCE

if !ECSS_COMPLIANCE
inc += lely/io2/can/err.h
if !NO_CXX
inc += lely/io2/can/err.hpp
endif
inc += lely/io2/can/msg.h
if !NO_CXX
inc += lely/io2/can/msg.hpp
endif
if !NO_STDIO
if PLATFORM_LINUX
inc += lely/io2/linux/can.h
if !NO_CXX
inc += lely/io2/linux/can.hpp
endif
endif # PLATFORM_LINUX
if PLATFORM_POSIX
inc += lely/io2/posix/poll.h
if !NO_CXX
inc += lely/io2/posix/poll.hpp
endif
endif # PLATFORM_POSIX
endif # !NO_STDIO
inc += lely/io2/sys/clock.h
if !NO_CXX
inc += lely/io2/sys/clock.hpp
endif
inc += lely/io2/sys/io.h
if !NO_CXX
inc += lely/io2/sys/io.hpp
endif
if !NO_STDIO
inc += lely/io2/sys/sigset.h
if !NO_CXX
inc += lely/io2/sys/sigset.hpp
endif
inc += lely/io2/sys/timer.h
if !NO_CXX
inc += lely/io2/sys/timer.hpp
endif
endif # !NO_STDIO
inc += lely/io2/user/can.h
if !NO_CXX
inc += lely/io2/user/can.hpp
endif
inc += lely/io2/user/timer.h
if !NO_CXX
inc += lely/io2/user/timer.hpp
endif
if !NO_STDIO
if PLATFORM_WIN32
if HAVE_IXXAT
inc += lely/io2/win32/ixxat.h
if !NO_CXX
inc += lely/io2/win32/ixxat.hpp
endif
endif
inc += lely/io2/win32/poll.h
if !NO_CXX
inc += lely/io2/win32/poll.hpp
endif
endif # PLATFORM_WIN32
endif # !NO_STDIO
inc += lely/io2/can.h
if !NO_CXX
inc += lely/io2/can.hpp
endif
inc += lely/io2/can_net.h
if !NO_CXX
inc += lely/io2/can_net.hpp
endif
inc += lely/io2/can_rt.h
if !NO_CXX
inc += lely/io2/can_rt.hpp
endif
inc += lely/io2/clock.h
if !NO_CXX
inc += lely/io2/clock.hpp
inc += lely/io2/co_can.hpp
inc += lely/io2/co_can_rt.hpp
inc += lely/io2/co_sigset.hpp
inc += lely/io2/co_timer.hpp
inc += lely/io2/co_tqueue.hpp
endif
inc += lely/io2/ctx.h
if !NO_CXX
inc += lely/io2/ctx.hpp
endif
inc += lely/io2/dev.h
if !NO_CXX
inc += lely/io2/dev.hpp
endif
inc += lely/io2/event.h
if !NO_CXX
inc += lely/io2/event.hpp
endif
inc += lely/io2/io2.h
inc += lely/io2/sigset.h
if !NO_CXX
inc += lely/io2/sigset.hpp
endif
inc += lely/io2/timer.h
if !NO_CXX
inc += lely/io2/timer.hpp
endif
inc += lely/io2/tqueue.h
if !NO_CXX
inc += lely/io2/tqueue.hpp
endif
inc += lely/io2/vcan.h
if !NO_CXX
inc += lely/io2/vcan.hpp
endif
endif # !ECSS_COMPLIANCE

inc += lely/co/def/array.def
inc += lely/co/def/basic.def
if !NO_CXX
inc += lely/co/def/cxx.def
endif
inc += lely/co/def/time.def
inc += lely/co/def/type.def
inc += lely/co/detail/dev.h
inc += lely/co/detail/obj.h
inc += lely/co/co.h
inc += lely/co/crc.h
if !NO_CO_CSDO
inc += lely/co/csdo.h
if !NO_CXX
inc += lely/co/csdo.hpp
endif
endif
if !NO_CO_DCF
inc += lely/co/dcf.h
if !NO_CXX
inc += lely/co/dcf.hpp
endif
endif
inc += lely/co/dev.h
if !NO_CXX
inc += lely/co/dev.hpp
endif
if !NO_CO_EMCY
inc += lely/co/emcy.h
if !NO_CXX
inc += lely/co/emcy.hpp
endif
endif
if !NO_CO_GW
inc += lely/co/gw.h
if !NO_CXX
inc += lely/co/gw.hpp
endif
endif
if !NO_CO_GW_TXT
inc += lely/co/gw_txt.h
if !NO_CXX
inc += lely/co/gw_txt.hpp
endif
endif
if !NO_CO_LSS
inc += lely/co/lss.h
if !NO_CXX
inc += lely/co/lss.hpp
endif
endif
inc += lely/co/nmt.h
if !NO_CXX
inc += lely/co/nmt.hpp
endif
inc += lely/co/obj.h
if !NO_CXX
inc += lely/co/obj.hpp
endif
inc += lely/co/pdo.h
if !NO_CO_RPDO
inc += lely/co/rpdo.h
if !NO_CXX
inc += lely/co/rpdo.hpp
endif
endif
if !NO_CO_SDEV
inc += lely/co/sdev.h
if !NO_CXX
inc += lely/co/sdev.hpp
endif
endif
inc += lely/co/sdo.h
inc += lely/co/ssdo.h
if !NO_CXX
inc += lely/co/ssdo.hpp
endif
if !NO_CO_SYNC
inc += lely/co/sync.h
if !NO_CXX
inc += lely/co/sync.hpp
endif
endif
if !NO_CO_TIME
inc += lely/co/time.h
if !NO_CXX
inc += lely/co/time.hpp
endif
endif
if !NO_CO_TPDO
inc += lely/co/tpdo.h
if !NO_CXX
inc += lely/co/tpdo.hpp
endif
endif
inc += lely/co/type.h
if !NO_CXX
inc += lely/co/type.hpp
endif
inc += lely/co/val.h
if !NO_CXX
inc += lely/co/val.hpp
endif
if !NO_CO_WTM
inc += lely/co/wtm.h
if !NO_CXX
inc += lely/co/wtm.hpp
endif
endif

if !NO_CXX
if !NO_MALLOC
inc += lely/coapp/type_traits.hpp
inc += lely/coapp/device.hpp
if !NO_COAPP_MASTER
inc += lely/coapp/driver.hpp
inc += lely/coapp/fiber_driver.hpp
inc += lely/coapp/logical_driver.hpp
inc += lely/coapp/loop_driver.hpp
if !NO_CO_LSS
inc += lely/coapp/lss_master.hpp
endif
inc += lely/coapp/master.hpp
endif
inc += lely/coapp/node.hpp
inc += lely/coapp/sdo.hpp
inc += lely/coapp/sdo_error.hpp
if !NO_COAPP_SLAVE
inc += lely/coapp/slave.hpp
endif
endif # !NO_MALLOC
endif # !NO_CXX

nobase_include_HEADERS = $(inc)
