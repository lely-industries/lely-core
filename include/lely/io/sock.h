/*!\file
 * This header file is part of the I/O library; it contains network socket
 * declarations.
 *
 * \copyright 2016 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#ifndef LELY_IO_SOCK_H
#define LELY_IO_SOCK_H

#include <lely/io/io.h>

enum {
	//! A Bluetooth socket.
	IO_SOCK_BTH = 1,
	//! A CAN (Controller Area Network) socket (only supported on Linux).
	IO_SOCK_CAN,
	//! An IPv4 socket.
	IO_SOCK_IPV4,
	//! An IPv6 socket.
	IO_SOCK_IPV6,
	//! A UNIX domain socket (only supported on POSIX platforms).
	IO_SOCK_UNIX
};

enum {
	/*!
	 * A stream-oriented connection-mode socket type. This corresponds to
	 * TCP for IPv4 or IPv6 sockets and RFCOMM for Bluetooth.
	 */
	IO_SOCK_STREAM = 1,
	/*!
	 * A datagram-oriented, typically connectionless-mode, socket type. This
	 * corresponds to UDP for IPv4 or IPv6 sockets.
	 */
	IO_SOCK_DGRAM
};

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Opens a network socket.
 *
 * \param domain the domain of the socket (one of #IO_SOCK_BTH, #IO_SOCK_CAN,
 *               #IO_SOCK_IPV4, #IO_SOCK_IPV6 or #IO_SOCK_UNIX).
 * \param type   the type of the socket (either #IO_SOCK_STREAM or
 *               #IO_SOCK_DGRAM).
 *
 * \returns a new I/O device handle, or #IO_HANDLE_ERROR on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN io_handle_t io_open_socket(int domain, int type);

/*!
 * Opens a pair of connected sockets.
 *
 * \param domain        the domain of the sockets (one of #IO_SOCK_IPV4,
 *                      #IO_SOCK_IPV6 or #IO_SOCK_UNIX).
 * \param type          the type of the sockets (either #IO_SOCK_STREAM or
 *                      #IO_SOCK_DGRAM).
 * \param handle_vector a 2-value array which, on success, contains the device
 *                      handles of the socket pair.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN int io_open_socketpair(int domain, int type,
		io_handle_t handle_vector[2]);

/*!
 * Opens a pipe (a pair of connected stream-oriented sockets). This function
 * uses UNIX domain sockets (#IO_SOCK_UNIX) on POSIX platforms and IPv4 sockets
 * (#IO_SOCK_IPV4) on Windows.
 *
 * \param handle_vector a 2-value array which, on success, contains the device
 *                      handles of the pipe. `handle_vector[0]` corresponds to
 *                      the read end and `handle_vector[1]` to the write end.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see io_open_socketpair()
 */
LELY_IO_EXTERN int io_open_pipe(io_handle_t handle_vector[2]);

/*!
 * Performs a receive operation on a network socket.
 *
 * \param handle a valid socket device handle.
 * \param buf    a pointer to the destination buffer.
 * \param nbytes the number of bytes to receive.
 * \param addr   an optional pointer to a value which, on success, contains the
 *               source address.
 *
 * \returns the number of bytes received on success, or -1 on error. In the
 * latter case, the error number can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN ssize_t io_recv(io_handle_t handle, void *buf, size_t nbytes,
		io_addr_t *addr);

/*!
 * Performs a send operation on a network socket.
 *
 * \param handle a valid socket device handle.
 * \param buf    a pointer to the source buffer.
 * \param nbytes the number of bytes to send.
 * \param addr   an optional pointer to the destination address (ignored for
 *               connection-mode sockets).
 *
 * \returns the number of bytes sent on success, or -1 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN ssize_t io_send(io_handle_t handle, const void *buf,
		size_t nbytes, const io_addr_t *addr);

/*!
 * Accepts an incoming connection on a listening socket.
 *
 * \param handle a valid socket device handle.
 * \param addr   an optional pointer to a value which, on success, contains the
 *               incoming network address.
 *
 * \returns a new I/O device handle, or #IO_HANDLE_ERROR on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 *
 * \see io_sock_listen()
 */
LELY_IO_EXTERN io_handle_t io_accept(io_handle_t handle, io_addr_t *addr);

/*!
 * Connects a socket to a network address.
 *
 * \param handle a valid socket device handle.
 * \param addr   a pointer to the network address to which to connect.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN int io_connect(io_handle_t handle, const io_addr_t *addr);

/*!
 * Obtains the domain of a socket (the first parameter in a call to
 * io_open_socket() or io_open_socketpair()).
 *
 * \returns #IO_SOCK_BTH, #IO_SOCK_CAN, #IO_SOCK_IPV4, #IO_SOCK_IPV6,
 * #IO_SOCK_UNIX or -1 on error. In the latter case, the error number can be
 * obtained with `get_errnum()`.
 *
 * \see io_sock_get_type()
 */
LELY_IO_EXTERN int io_sock_get_domain(io_handle_t handle);

/*!
 * Obtains the type of a network socket (the second parameter in a call to
 * io_open_socket() or io_open_socketpair()).
 *
 * \returns #IO_SOCK_STREAM or #IO_SOCK_DGRAM, or -1 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 *
 * \see io_sock_get_domain()
 */
LELY_IO_EXTERN int io_sock_get_type(io_handle_t handle);

/*!
 * Binds a local network address to a socket.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN int io_sock_bind(io_handle_t handle, const io_addr_t *addr);

/*!
 * Marks a connection-mode socket (#IO_SOCK_STREAM) as accepting connections.
 *
 * \param handle  a valid socket device handle.
 * \param backlog the number of pending connections in the socket's listen
 *                queue. The maximum value can be obtained with
 *                io_sock_get_maxconn(). If \a backlog is 0, an
 *                implementation-defined minimum value is used.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see io_accept()
 */
LELY_IO_EXTERN int io_sock_listen(io_handle_t handle, int backlog);

/*!
 * Obtains the locally-bound name of a socket and stores the resulting address
 * in *\a addr. The socket name is set by io_sock_bind() and io_connect().
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_IO_EXPORT int io_sock_get_sockname(io_handle_t handle, io_addr_t *addr);

/*!
 * Obtains the peer address of a socket and stores the result in *\a addr. The
 * peer name is set by io_accept() and io_connect().
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_IO_EXPORT int io_sock_get_peername(io_handle_t handle, io_addr_t *addr);

/*!
 * Returns the maximum queue length for pending connections. This value can be
 * used as the \a backlog parameter in a call to `io_sock_listen()`.
 *
 * \returns the value of \a SOMAXCONN, or -1 on error.
 */
LELY_IO_EXPORT int io_sock_get_maxconn(void);

#ifdef __cplusplus
}
#endif

#endif

