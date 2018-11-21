/**@file
 * This header file is part of the I/O library; it contains the network socket
 * declarations.
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

#ifndef LELY_IO_SOCK_H_
#define LELY_IO_SOCK_H_

#include <lely/io/io.h>

enum {
	/// A Bluetooth socket.
	IO_SOCK_BTH = 1,
	/// An IPv4 socket.
	IO_SOCK_IPV4,
	/// An IPv6 socket.
	IO_SOCK_IPV6,
	/// A UNIX domain socket (only supported on POSIX platforms).
	IO_SOCK_UNIX
};

enum {
	/**
	 * A stream-oriented connection-mode socket type. This corresponds to
	 * TCP for IPv4 or IPv6 sockets and RFCOMM for Bluetooth.
	 */
	IO_SOCK_STREAM = 1,
	/**
	 * A datagram-oriented, typically connectionless-mode, socket type. This
	 * corresponds to UDP for IPv4 or IPv6 sockets.
	 */
	IO_SOCK_DGRAM
};

enum {
	/// Peeks at incoming data.
	IO_MSG_PEEK = 1 << 0,
	/// Requests out-of-band data.
	IO_MSG_OOB = 1 << 1,
	/**
	 * On stream-oriented sockets, block until the full amount of data can
	 * be returned.
	 */
	IO_MSG_WAITALL = 1 << 2
};

enum {
	/// Disables further receive operations.
	IO_SHUT_RD,
	/// Disables further send operations.
	IO_SHUT_WR,
	/// Disables further send and receive operations.
	IO_SHUT_RDWR
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opens a network socket.
 *
 * @param domain the domain of the socket (one of #IO_SOCK_BTH, #IO_SOCK_IPV4,
 *               #IO_SOCK_IPV6 or #IO_SOCK_UNIX).
 * @param type   the type of the socket (either #IO_SOCK_STREAM or
 *               #IO_SOCK_DGRAM).
 *
 * @returns a new I/O device handle, or #IO_HANDLE_ERROR on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
io_handle_t io_open_socket(int domain, int type);

/**
 * Opens a pair of connected sockets.
 *
 * @param domain        the domain of the sockets (one of #IO_SOCK_IPV4,
 *                      #IO_SOCK_IPV6 or #IO_SOCK_UNIX).
 * @param type          the type of the sockets (either #IO_SOCK_STREAM or
 *                      #IO_SOCK_DGRAM).
 * @param handle_vector a 2-value array which, on success, contains the device
 *                      handles of the socket pair.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_open_socketpair(int domain, int type, io_handle_t handle_vector[2]);

/**
 * Performs a receive operation on a network socket.
 *
 * @param handle a valid socket device handle.
 * @param buf    a pointer to the destination buffer.
 * @param nbytes the number of bytes to receive.
 * @param addr   an optional pointer to a value which, on success, contains the
 *               source address.
 * @param flags  the type of message reception (any combination of #IO_MSG_PEEK,
 *               #IO_MSG_OOB and #IO_MSG_WAITALL).
 *
 * @returns the number of bytes received on success, or -1 on error. In the
 * latter case, the error number can be obtained with get_errc().
 */
ssize_t io_recv(io_handle_t handle, void *buf, size_t nbytes, io_addr_t *addr,
		int flags);

/**
 * Performs a send operation on a network socket.
 *
 * @param handle a valid socket device handle.
 * @param buf    a pointer to the source buffer.
 * @param nbytes the number of bytes to send.
 * @param addr   an optional pointer to the destination address (ignored for
 *               connection-mode sockets).
 * @param flags  type type of message transmission (0 or #IO_MSG_OOB).
 *
 * @returns the number of bytes sent on success, or -1 on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
ssize_t io_send(io_handle_t handle, const void *buf, size_t nbytes,
		const io_addr_t *addr, int flags);

/**
 * Accepts an incoming connection on a listening socket.
 *
 * @param handle a valid socket device handle.
 * @param addr   an optional pointer to a value which, on success, contains the
 *               incoming network address.
 *
 * @returns a new I/O device handle, or #IO_HANDLE_ERROR on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see io_sock_listen()
 */
io_handle_t io_accept(io_handle_t handle, io_addr_t *addr);

/**
 * Connects a socket to a network address.
 *
 * @param handle a valid socket device handle.
 * @param addr   a pointer to the network address to which to connect.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_connect(io_handle_t handle, const io_addr_t *addr);

/**
 * Obtains the domain of a socket (the first parameter in a call to
 * io_open_socket() or io_open_socketpair()).
 *
 * @returns #IO_SOCK_BTH, #IO_SOCK_IPV4, #IO_SOCK_IPV6 or #IO_SOCK_UNIX, or -1
 * on error. In the latter case, the error number can be obtained with
 * get_errc().
 *
 * @see io_sock_get_type(), io_addr_get_domain()
 */
int io_sock_get_domain(io_handle_t handle);

/**
 * Obtains the type of a network socket (the second parameter in a call to
 * io_open_socket() or io_open_socketpair()).
 *
 * @returns #IO_SOCK_STREAM or #IO_SOCK_DGRAM, or -1 on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see io_sock_get_domain()
 */
int io_sock_get_type(io_handle_t handle);

/**
 * Binds a local network address to a socket.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_sock_bind(io_handle_t handle, const io_addr_t *addr);

/**
 * Marks a connection-mode socket (#IO_SOCK_STREAM) as accepting connections.
 *
 * @param handle  a valid socket device handle.
 * @param backlog the number of pending connections in the socket's listen
 *                queue. The maximum value can be obtained with
 *                io_sock_get_maxconn(). If <b>backlog</b> is 0, an
 *                implementation-defined minimum value is used.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_accept()
 */
int io_sock_listen(io_handle_t handle, int backlog);

/**
 * Causes all or part of a full-duplex connection on a socket to be shut down.
 *
 * @param handle a valid socket device handle.
 * @param how    the type of shutdown (one of #IO_SHUT_RD, #IO_SHUT_WR or
 *               #IO_SHUT_RDWR).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_sock_shutdown(io_handle_t handle, int how);

/**
 * Obtains the locally-bound name of a socket and stores the resulting address
 * in *<b>addr</b>. The socket name is set by io_sock_bind() and io_connect().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_sock_get_sockname(io_handle_t handle, io_addr_t *addr);

/**
 * Obtains the peer address of a socket and stores the result in *<b>addr</b>.
 * The peer name is set by io_accept() and io_connect().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_sock_get_peername(io_handle_t handle, io_addr_t *addr);

/**
 * Returns the maximum queue length for pending connections. This value can be
 * used as the <b>backlog</b> parameter in a call to `io_sock_listen()`.
 *
 * @returns the value of <b>SOMAXCONN</b>, or -1 on error.
 */
int io_sock_get_maxconn(void);

/**
 * Checks if a socket is currently listening for incoming connections. This
 * function implements the SOL_SOCKET/SO_ACCEPTCONN option.
 *
 * @returns 1 if the socket is accepting connections and 0 if not, or -1 on
 * error. In the latter case, the error number can be obtained with
 * get_errc().
 *
 * @see io_sock_listen()
 */
int io_sock_get_acceptconn(io_handle_t handle);

/**
 * Checks if a socket is allowed to send broadcast messages. This function
 * implements the SOL_SOCKET/SO_BROADCAST option.
 *
 * @returns 1 if address reuse is enabled and 0 if not, or -1 on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see io_sock_set_broadcast()
 */
int io_sock_get_broadcast(io_handle_t handle);

/**
 * Enables a socket to send broadcast messages if <b>broadcast</b> is non-zero,
 * and disables this option otherwise (disabled by default). This function
 * implements the SOL_SOCKET/SO_BROADCAST option.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_get_broadcast()
 */
int io_sock_set_broadcast(io_handle_t handle, int broadcast);

/**
 * Checks if debugging is enabled for a socket. This function implements the
 * SOL_SOCKET/SO_DEBUG option.
 *
 * @returns 1 if debugging is enabled and 0 if not, or -1 on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see io_sock_set_debug()
 */
int io_sock_get_debug(io_handle_t handle);

/**
 * Enables (platform dependent) debugging output for a socket if <b>debug</b> is
 * non-zero, and disables this option otherwise (disabled by default). This
 * function implements the SOL_SOCKET/SO_DEBUG option.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_get_debug()
 */
int io_sock_set_debug(io_handle_t handle, int debug);

/**
 * Checks if routing is disabled for a socket. This function implements the
 * SOL_SOCKET/SO_DONTROUTE option.
 *
 * @returns 1 if routing is disabled and 0 if not, or -1 on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see io_sock_set_dontroute()
 */
int io_sock_get_dontroute(io_handle_t handle);

/**
 * Bypasses normal routing for a socket if <b>dontroute</b> is non-zero, and
 * disables this option otherwise (disabled by default). This function
 * implements the SOL_SOCKET/SO_DONTROUTE option.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_get_dontroute()
 */
int io_sock_set_dontroute(io_handle_t handle, int dontroute);

/**
 * Obtains and clears the current error number of a socket, and stores the value
 * in *<b>perror</b>. This function implements the SOL_SOCKET/SO_ERROR option.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_sock_get_error(io_handle_t handle, int *perror);

/**
 * Checks if the TCP keep-alive option is enabled for a socket. This function
 * implements the SOL_SOCKET/KEEPALIVE option.
 *
 * @returns 1 if TCP keep-alive is enabled and 0 if not, or -1 on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see io_sock_set_keepalive()
 */
int io_sock_get_keepalive(io_handle_t handle);

/**
 * Enables or disables the TCP keep-alive option for a socket (disabled by
 * default). Note that the <b>time</b> and <b>interval</b> options are supported
 * only on Windows and Linux. This function implements the
 * SOL_SOCKET/SO_KEEPALIVE option.
 *
 * @param handle    a valid socket device handle.
 * @param keepalive a boolean option specifying whether TCP keep-alive should be
 *                  enabled (non-zero) or disabled (zero).
 * @param time      the timeout (in seconds) after which the first keep-alive
 *                  packet is sent. This parameter is unused if <b>keepalive</b>
 *                  is zero.
 * @param interval  the interval (in seconds) between successive keep-alive
 *                  packets if no acknowledgment is received. This parameter is
 *                  unused if <b>keepalive</b> is zero.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_get_keepalive()
 */
int io_sock_set_keepalive(
		io_handle_t handle, int keepalive, int time, int interval);

/**
 * Obtains the linger time (in seconds) of a socket. This function implements
 * the SOL_SOCKET/SO_LINGER option.
 *
 * @returns the linger time, or -1 on error. In the latter case, the error
 * number can be obtained with get_errc().
 *
 * @see io_sock_set_linger()
 */
int io_sock_get_linger(io_handle_t handle);

/**
 * Sets the time (in seconds) io_close() will wait for unsent messages to be
 * sent. If <b>time</b> is 0, lingering is disabled. This function implements
 * the SOL_SOCKET/SO_LINGER option.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_getlinger()
 */
int io_sock_set_linger(io_handle_t handle, int time);

/**
 * Checks if out-of-band data is received in the normal data stream of a socket.
 * This function implements the SOL_SOCKET/SO_OOBINLINE option.
 *
 * @returns 1 if out-of-band data is received in the normal data stream and 0 if
 * not, or -1 on error. In the latter case, the error number can be obtained
 * with get_errc().
 *
 * @see io_sock_set_oobinline()
 */
int io_sock_get_oobinline(io_handle_t handle);

/**
 * Requests that out-of-band data is placed into the normal data stream of
 * socket if <b>oobinline</b> is non-zero, and disables this option otherwise
 * (disabled by default). This function implements the SOL_SOCKET/SO_OOBINLINE
 * option.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_get_oobinline()
 */
int io_sock_set_oobinline(io_handle_t handle, int oobinline);

/**
 * Obtains the size (in bytes) of the receive buffer of a socket. This function
 * implements the SOL_SOCKET/SO_RCVBUF option.
 *
 * @returns the size of the receive buffer, or -1 on error. In the latter case,
 * the error number can be obtained with get_errc().
 *
 * @see io_sock_set_rcvbuf()
 */
int io_sock_get_rcvbuf(io_handle_t handle);

/**
 * Sets the size (in bytes) of the receive buffer of a socket. This function
 * implements the SOL_SOCKET/SO_RCVBUF option.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_get_rcvbuf()
 */
int io_sock_set_rcvbuf(io_handle_t handle, int size);

/**
 * Sets the timeout (in milliseconds) of a receive operation on a socket. This
 * function implements the SOL_SOCKET/SO_RCVTIMEO option.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_sock_set_rcvtimeo(io_handle_t handle, int timeout);

/**
 * Checks if a socket is allowed to be bound to an address that is already in
 * use. This function implements the SOL_SOCKET/SO_REUSEADDR option.
 *
 * @returns 1 if address reuse is enabled and 0 if not, or -1 on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see io_sock_set_reuseaddr()
 */
int io_sock_get_reuseaddr(io_handle_t handle);

/**
 * Enables a socket to be bound to an address that is already in use if
 * <b>reuseaddr</b> is non-zero, and disables this option otherwise (disabled by
 * default). This function implements the SOL_SOCKET/SO_REUSEADDR option.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_get_reuseaddr()
 */
int io_sock_set_reuseaddr(io_handle_t handle, int reuseaddr);

/**
 * Obtains the size (in bytes) of the send buffer of a socket. This function
 * implements the SOL_SOCKET/SO_SNDBUF option.
 *
 * @returns the size of the send buffer, or -1 on error. In the latter case, the
 * error number can be obtained with get_errc().
 *
 * @see io_sock_set_sndbuf()
 */
int io_sock_get_sndbuf(io_handle_t handle);

/**
 * Sets the size (in bytes) of the send buffer of a socket. This function
 * implements the SOL_SOCKET/SO_SNDBUF option.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_get_sndbuf()
 */
int io_sock_set_sndbuf(io_handle_t handle, int size);

/**
 * Sets the timeout (in milliseconds) of a send operation on a socket. This
 * function implements the SOL_SOCKET/SO_SNDTIMEO option.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_sock_set_sndtimeo(io_handle_t handle, int timeout);

/**
 * Checks if Nagle's algorithm for send coalescing is enabled for a socket. This
 * function implements the IPPROTO_TCP/TCP_NODELAY option.
 *
 * @returns 1 if Nagle's algorithm is disabled and 0 otherwise, or -1 on error.
 * In the latter case, the error number can be obtained with get_errc().
 *
 * @see io_sock_set_tcp_nodelay()
 */
int io_sock_get_tcp_nodelay(io_handle_t handle);

/**
 * Disables Nagle's algorithm for send coalescing if <b>nodelay</b> is non-zero,
 * and enables it otherwise. This function implements the
 * IPPROTO_TCP/TCP_NODELAY option. Nagle's algorithm is enabled by default.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_get_tcp_nodelay()
 */
int io_sock_set_tcp_nodelay(io_handle_t handle, int nodelay);

/**
 * Obtains the amount of data (in bytes) in the input buffer of a socket.
 *
 * @returns the number of bytes that can be read, or -1 on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
ssize_t io_sock_get_nread(io_handle_t handle);

/**
 * Checks if the loopback of outgoing multicast datagrams is enabled for a
 * socket. This function implements the IPPROTO_IP/IP_MULTICAST_LOOP and
 * IPPROTO_IPV6/IPV6_MULTICAST_LOOP options.
 *
 * @returns 1 if multicast loopback is enabled and 0 if not, or -1 on error. In
 * the latter case, the error number can be obtained with get_errc().
 *
 * @see io_sock_set_mcast_loop()
 */
int io_sock_get_mcast_loop(io_handle_t handle);

/**
 * Enables the loopback of outgoing multicast datagrams for a socket if
 * <b>loop</b> is non-zero, and disables this option otherwise (enabled by
 * default). This function implements the IPPROTO_IP/IP_MULTICAST_LOOP and
 * IPPROTO_IPV6/IPV6_MULTICAST_LOOP options.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_get_mcast_loop()
 */
int io_sock_set_mcast_loop(io_handle_t handle, int loop);

/**
 * Obtains the TTL (time to live) value for IP multicast traffic on a socket.
 * This function implements the IPPROTO_IP/IP_MULTICAST_TTL and
 * IPPROTO_IPV6/IPV6_MULTICAST_HOPS options.
 *
 * @returns the TTL for IP multicast traffic, or -1 on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see io_sock_set_mcast_ttl()
 */
int io_sock_get_mcast_ttl(io_handle_t handle);

/**
 * Sets the TTL (time to live) value for IP multicast traffic on a socket (the
 * default is 1). This function implements the IPPROTO_IP/IP_MULTICAST_TTL and
 * IPPROTO_IPV6/IPV6_MULTICAST_HOPS options.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_get_mcast_ttl()
 */
int io_sock_set_mcast_ttl(io_handle_t handle, int ttl);

/**
 * Joins an any-source multicast group.
 *
 * @param handle a valid IPv4 or IPv6 connectionless socket device handle.
 * @param index  a network interface index. On Linux, if <b>index</b> is 0, the
 *               interface is chosen automatically.
 * @param group  a pointer to the address of the group to join.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_mcast_leave_group()
 */
int io_sock_mcast_join_group(
		io_handle_t handle, unsigned int index, const io_addr_t *group);

/**
 * Blocks data from a given source to a given multicast group.
 *
 * @param handle a valid IPv4 or IPv6 connectionless socket device handle.
 * @param index  a network interface index. On Linux, if <b>index</b> is 0, the
 *               interface is chosen automatically.
 * @param group  a pointer to the address of the multicast group.
 * @param source a pointer to the address of the source to block.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_mcast_unblock_source()
 */
int io_sock_mcast_block_source(io_handle_t handle, unsigned int index,
		const io_addr_t *group, const io_addr_t *source);

/**
 * Unblocks data from a given source to a given multicast group.
 *
 * @param handle a valid IPv4 or IPv6 connectionless socket device handle.
 * @param index  a network interface index. On Linux, if <b>index</b> is 0, the
 *               interface is chosen automatically.
 * @param group  a pointer to the address of the multicast group.
 * @param source a pointer to the address of the source to unblock.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_mcast_block_source()
 */
int io_sock_mcast_unblock_source(io_handle_t handle, unsigned int index,
		const io_addr_t *group, const io_addr_t *source);

/**
 * Leaves an any-source multicast group.
 *
 * @param handle a valid IPv4 or IPv6 connectionless socket device handle.
 * @param index  a network interface index. On Linux, if <b>index</b> is 0, the
 *               interface is chosen automatically.
 * @param group  a pointer to the address of the group to leave.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_mcast_join_group()
 */
int io_sock_mcast_leave_group(
		io_handle_t handle, unsigned int index, const io_addr_t *group);

/**
 * Joins a source-specific multicast group.
 *
 * @param handle a valid IPv4 or IPv6 connectionless socket device handle.
 * @param index  a network interface index. On Linux, if <b>index</b> is 0, the
 *               interface is chosen automatically.
 * @param group  a pointer to the address of the group to join.
 * @param source a pointer to the address of the source from which to receive
 *               data.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_mcast_join_source_group()
 */
int io_sock_mcast_join_source_group(io_handle_t handle, unsigned int index,
		const io_addr_t *group, const io_addr_t *source);

/**
 * Leaves a source-specific multicast group.
 *
 * @param handle a valid IPv4 or IPv6 connectionless socket device handle.
 * @param index  a network interface index. On Linux, if <b>index</b> is 0, the
 *               interface is chosen automatically.
 * @param group  a pointer to the address of the group to leave.
 * @param source a pointer to the address of the source from which data was
 *               received.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sock_mcast_leave_source_group()
 */
int io_sock_mcast_leave_source_group(io_handle_t handle, unsigned int index,
		const io_addr_t *group, const io_addr_t *source);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_SOCK_H_
