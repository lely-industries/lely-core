inc =
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

nobase_include_HEADERS = $(inc)
