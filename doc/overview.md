Overview
========

The generic CAN message type is struct #can_msg (see lely/can/msg.h). This
struct can represent both CAN and CAN FD frames (if CAN FD support is enabled).
The #can_msg struct does not correspond to any specific driver or interface.
Conversion functions to and from IXXAT VCI V3 and SocketCAN can be found in
lely/can/ixxat.h and lely/can/socket.h, respectively.

The CAN bus is a broadcast bus, so each node receives all messages. Applications
typically only listen to messages with a specific CAN identifier. The CAN
network interface (#can_net_t, see lely/can/net.h) assists the user with
processing CAN frames by allowing callback functions to be registered for
individual CAN identifiers (#can_recv_t). Additionally, callback functions can
be registered to be invoked at specific times (#can_timer_t). The CAN network
interface is passive, relying on the application to feed it CAN frames or notify
it of the current time. A C++ interface can be found in lely/can/net.hpp.

When processing CAN frames it is often necessary to buffer incoming or outgoing
messages. A thread-safe, lock-free circular buffer (#can_buf) is provided in
lely/can/buf.h (see lely/can/buf.hpp for the C++ interface).

