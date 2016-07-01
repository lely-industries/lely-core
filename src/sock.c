/*!\file
 * This file is part of the I/O library; it contains the implementation of the
 * network socket functions.
 *
 * \see lely/io/sock.h
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

#include "io.h"
#include <lely/util/errnum.h>
#include <lely/io/addr.h>
#include <lely/io/sock.h>
#include "handle.h"

#include <assert.h>
#include <string.h>

#if defined(_WIN32) || _POSIX_C_SOURCE >= 200112L

//! A network socket.
struct sock {
	//! The I/O device base handle.
	struct io_handle base;
	/*!
	 * The domain of the socket (one of #IO_SOCK_BTH, #IO_SOCK_IPV4,
	 * #IO_SOCK_IPV6 or #IO_SOCK_UNIX).
	 */
	int domain;
	//! The type of the socket (#IO_SOCK_STREAM or #IO_SOCK_DGRAM).
	int type;
};

static void sock_fini(struct io_handle *handle);
static int sock_flags(struct io_handle *handle, int flags);
static ssize_t sock_read(struct io_handle *handle, void *buf, size_t nbytes);
static ssize_t sock_write(struct io_handle *handle, const void *buf,
		size_t nbytes);
static ssize_t sock_recv(struct io_handle *handle, void *buf, size_t nbytes,
		io_addr_t *addr, int flags);
static ssize_t sock_send(struct io_handle *handle, const void *buf,
		size_t nbytes, const io_addr_t *addr, int flags);
static struct io_handle *sock_accept(struct io_handle *handle, io_addr_t *addr);
static int sock_connect(struct io_handle *handle, const io_addr_t *addr);

static const struct io_handle_vtab sock_vtab = {
	.type = IO_TYPE_SOCK,
	.size = sizeof(struct sock),
	.fini = &sock_fini,
	.flags = &sock_flags,
	.read = &sock_read,
	.write = &sock_write,
	.recv = &sock_recv,
	.send = &sock_send,
	.accept = &sock_accept,
	.connect = &sock_connect
};

int _socketpair(int af, int type, int protocol, SOCKET sv[2]);

LELY_IO_EXPORT io_handle_t
io_open_socket(int domain, int type)
{
	errc_t errc = 0;

	int flags = 0;
#if defined(__CYGWIN__) || defined(__linux__)
	flags |= SOCK_CLOEXEC;
#endif

	SOCKET s;
	switch (domain) {
#ifdef _WIN32
	case IO_SOCK_BTH:
		switch (type) {
		case IO_SOCK_STREAM:
			s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
			break;
		default:
			errc = errnum2c(ERRNUM_PROTOTYPE);
			goto error_type;
		}
		break;
#elif defined(__linux__) && defined(HAVE_BLUETOOTH_BLUETOOTH_H) \
		&& defined(HAVE_BLUETOOTH_RFCOMM_H)
	case IO_SOCK_BTH:
		switch (type) {
		case IO_SOCK_STREAM:
			s = socket(AF_BLUETOOTH, SOCK_STREAM | flags,
					BTPROTO_RFCOMM);
			break;
		default:
			errc = errnum2c(ERRNUM_PROTOTYPE);
			goto error_type;
		}
		break;
#endif
	case IO_SOCK_IPV4:
		switch (type) {
		case IO_SOCK_STREAM:
			s = socket(AF_INET, SOCK_STREAM | flags, 0);
			break;
		case IO_SOCK_DGRAM:
			s = socket(AF_INET, SOCK_DGRAM | flags, 0);
			break;
		default:
			errc = errnum2c(ERRNUM_PROTOTYPE);
			goto error_type;
		}
		break;
	case IO_SOCK_IPV6:
		switch (type) {
		case IO_SOCK_STREAM:
			s = socket(AF_INET6, SOCK_STREAM | flags, 0);
			break;
		case IO_SOCK_DGRAM:
			s = socket(AF_INET6, SOCK_DGRAM | flags, 0);
			break;
		default:
			errc = errnum2c(ERRNUM_PROTOTYPE);
			goto error_type;
		}
		break;
#if _POSIX_C_SOURCE >= 200112L
	case IO_SOCK_UNIX:
		switch (type) {
		case IO_SOCK_STREAM:
			s = socket(AF_UNIX, SOCK_STREAM | flags, 0);
			break;
		case IO_SOCK_DGRAM:
			s = socket(AF_UNIX, SOCK_DGRAM | flags, 0);
			break;
		default:
			errc = errnum2c(ERRNUM_PROTOTYPE);
			goto error_type;
		}
		break;
#endif
	default:
		errc = errnum2c(ERRNUM_AFNOSUPPORT);
		goto error_domain;
	}

	if (__unlikely(s == INVALID_SOCKET)) {
		errc = get_errc();
		goto error_socket;
	}

#if _POSIX_C_SOURCE >= 200112L && !defined(__CYGINW__) && !defined(__linux__)
	if (__unlikely(fcntl(s, F_SETFD, FD_CLOEXEC) == -1)) {
		errc = get_errc();
		goto error_fcntl;
	}
#endif

	struct io_handle *handle = io_handle_alloc(&sock_vtab);
	if (__unlikely(!handle)) {
		errc = get_errc();
		goto error_alloc_handle;
	}

	handle->fd = (HANDLE)s;
	((struct sock *)handle)->domain = domain;
	((struct sock *)handle)->type = type;

	return io_handle_acquire(handle);

error_alloc_handle:
#if _POSIX_C_SOURCE >= 200112L && !defined(__CYGINW__) && !defined(__linux__)
error_fcntl:
#endif
	closesocket(s);
error_socket:
error_type:
error_domain:
	set_errc(errc);
	return IO_HANDLE_ERROR;
}

LELY_IO_EXPORT int
io_open_socketpair(int domain, int type, io_handle_t handle_vector[2])
{
	assert(handle_vector);
	handle_vector[0] = handle_vector[1] = IO_HANDLE_ERROR;

	errc_t errc = 0;

	int flags = 0;
#if defined(__CYGWIN__) || defined(__linux__)
	flags |= SOCK_CLOEXEC;
#endif

	int result;
	SOCKET socket_vector[2];
	switch (domain) {
	case IO_SOCK_IPV4:
		switch (type) {
		case IO_SOCK_STREAM:
			result = _socketpair(AF_INET, SOCK_STREAM | flags, 0,
					socket_vector);
			break;
		case IO_SOCK_DGRAM:
			result = _socketpair(AF_INET, SOCK_DGRAM | flags, 0,
					socket_vector);
			break;
		default:
			errc = errnum2c(ERRNUM_PROTOTYPE);
			goto error_type;
		}
		break;
	case IO_SOCK_IPV6:
		switch (type) {
		case IO_SOCK_STREAM:
			result = _socketpair(AF_INET6, SOCK_STREAM | flags, 0,
					socket_vector);
			break;
		case IO_SOCK_DGRAM:
			result = _socketpair(AF_INET6, SOCK_DGRAM | flags, 0,
					socket_vector);
			break;
		default:
			errc = errnum2c(ERRNUM_PROTOTYPE);
			goto error_type;
		}
		break;
#if _POSIX_C_SOURCE >= 200112L
	case IO_SOCK_UNIX:
		switch (type) {
		case IO_SOCK_STREAM:
			result = _socketpair(AF_UNIX, SOCK_STREAM | flags, 0,
					socket_vector);
			break;
		case IO_SOCK_DGRAM:
			result = _socketpair(AF_UNIX, SOCK_DGRAM | flags, 0,
					socket_vector);
			break;
		default:
			errc = errnum2c(ERRNUM_PROTOTYPE);
			goto error_type;
		}
		break;
#endif
	default:
		errc = errnum2c(ERRNUM_AFNOSUPPORT);
		goto error_domain;
	}

	if (__unlikely(result == SOCKET_ERROR)) {
		errc = get_errc();
		goto error_socketpair;
	}

#if _POSIX_C_SOURCE >= 200112L && !defined(__CYGINW__) && !defined(__linux__)
	if (__unlikely(fcntl(socket_vector[0], F_SETFD, FD_CLOEXEC) == -1)) {
		errc = get_errc();
		goto error_fcntl;
	}
	if (__unlikely(fcntl(socket_vector[1], F_SETFD, FD_CLOEXEC) == -1)) {
		errc = get_errc();
		goto error_fcntl;
	}
#endif

	handle_vector[0] = io_handle_alloc(&sock_vtab);
	if (__unlikely(!handle_vector[0])) {
		errc = get_errc();
		goto error_alloc_handle_vector_0;
	}

	handle_vector[0]->fd = (HANDLE)socket_vector[0];
	((struct sock *)handle_vector[0])->domain = domain;
	((struct sock *)handle_vector[0])->type = type;

	handle_vector[1] = io_handle_alloc(&sock_vtab);
	if (__unlikely(!handle_vector[1])) {
		errc = get_errc();
		goto error_alloc_handle_vector_1;
	}

	handle_vector[1]->fd = (HANDLE)socket_vector[1];
	((struct sock *)handle_vector[1])->domain = domain;
	((struct sock *)handle_vector[1])->type = type;

	io_handle_acquire(handle_vector[0]);
	io_handle_acquire(handle_vector[1]);

	return 0;

error_alloc_handle_vector_1:
	handle_vector[1] = IO_HANDLE_ERROR;
	io_handle_free(handle_vector[0]);
error_alloc_handle_vector_0:
	handle_vector[0] = IO_HANDLE_ERROR;
#if _POSIX_C_SOURCE >= 200112L && !defined(__CYGINW__) && !defined(__linux__)
error_fcntl:
#endif
	closesocket(socket_vector[1]);
	closesocket(socket_vector[0]);
error_socketpair:
error_type:
error_domain:
	set_errc(errc);
	return -1;
}

LELY_IO_EXPORT int
io_open_pipe(io_handle_t handle_vector[2])
{
#if _POSIX_C_SOURCE >= 200112L
	return io_open_socketpair(IO_SOCK_UNIX, IO_SOCK_STREAM, handle_vector);
#else
	return io_open_socketpair(IO_SOCK_IPV4, IO_SOCK_STREAM, handle_vector);
#endif
}

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

LELY_IO_EXPORT ssize_t
io_recv(io_handle_t handle, void *buf, size_t nbytes, io_addr_t *addr,
		int flags)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (__unlikely(!handle->vtab->recv)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

	return handle->vtab->recv(handle, buf, nbytes, addr, flags);
}

LELY_IO_EXPORT ssize_t
io_send(io_handle_t handle, const void *buf, size_t nbytes,
		const io_addr_t *addr, int flags)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (__unlikely(!handle->vtab->send)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

	return handle->vtab->send(handle, buf, nbytes, addr, flags);
}

LELY_IO_EXPORT io_handle_t
io_accept(io_handle_t handle, io_addr_t *addr)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return IO_HANDLE_ERROR;
	}

	assert(handle->vtab);
	if (__unlikely(!handle->vtab->accept)) {
		set_errnum(ERRNUM_NOTSOCK);
		return IO_HANDLE_ERROR;
	}

	return handle->vtab->accept(handle, addr);
}

LELY_IO_EXPORT int
io_connect(io_handle_t handle, const io_addr_t *addr)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (__unlikely(!handle->vtab->connect)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

	return handle->vtab->connect(handle, addr);
}

#if defined(_WIN32) || _POSIX_C_SOURCE >= 200112L

LELY_IO_EXPORT int
io_sock_get_domain(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &sock_vtab)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

	return ((struct sock *)handle)->domain;
}

LELY_IO_EXPORT int
io_sock_get_type(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &sock_vtab)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

	return ((struct sock *)handle)->type;
}

LELY_IO_EXPORT int
io_sock_bind(io_handle_t handle, const io_addr_t *addr)
{
	assert(addr);

	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	return bind((SOCKET)handle->fd, (const struct sockaddr *)&addr->addr,
			addr->addrlen);
}

LELY_IO_EXPORT int
io_sock_listen(io_handle_t handle, int backlog)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	return listen((SOCKET)handle->fd, backlog) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_shutdown(io_handle_t handle, int how)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	switch (how) {
	case IO_SHUT_RD:
#ifdef _WIN32
		how = SD_RECEIVE;
#else
		how = SHUT_RD;
#endif
		break;
	case IO_SHUT_WR:
#ifdef _WIN32
		how = SD_SEND;
#else
		how = SHUT_WR;
#endif
		break;
	case IO_SHUT_RDWR:
#ifdef _WIN32
		how = SD_BOTH;
#else
		how = SHUT_RDWR;
#endif
		break;
	default:
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	return shutdown((SOCKET)handle->fd, how) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_get_sockname(io_handle_t handle, io_addr_t *addr)
{
	assert(addr);

	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	addr->addrlen = sizeof(addr->addr);
	return getsockname((SOCKET)handle->fd, (struct sockaddr *)&addr->addr,
			(socklen_t *)&addr->addrlen) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_get_peername(io_handle_t handle, io_addr_t *addr)
{
	assert(addr);

	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	addr->addrlen = sizeof(addr->addr);
	return getpeername((SOCKET)handle->fd, (struct sockaddr *)&addr->addr,
			(socklen_t *)&addr->addrlen) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_get_maxconn(void)
{
	return SOMAXCONN;
}

LELY_IO_EXPORT int
io_sock_get_acceptconn(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval;
#else
	int optval;
#endif
	if (__unlikely(getsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_ACCEPTCONN,
			(char *)&optval, &(socklen_t){ sizeof(optval) })))
		return -1;
	return !!optval;
}

LELY_IO_EXPORT int
io_sock_get_broadcast(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval;
#else
	int optval;
#endif
	if (__unlikely(getsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_BROADCAST,
			(char *)&optval, &(socklen_t){ sizeof(optval) })))
		return -1;
	return !!optval;
}

LELY_IO_EXPORT int
io_sock_set_broadcast(io_handle_t handle, int broadcast)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval = !!broadcast;
#else
	int optval = !!broadcast;
#endif
	return setsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_BROADCAST,
			(const char *)&optval, sizeof(optval)) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_get_debug(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval;
#else
	int optval;
#endif
	if (__unlikely(getsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_DEBUG,
			(char *)&optval, &(socklen_t){ sizeof(optval) })))
		return -1;
	return !!optval;
}

LELY_IO_EXPORT int
io_sock_set_debug(io_handle_t handle, int debug)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval = !!debug;
#else
	int optval = !!debug;
#endif
	return setsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_DEBUG,
			(const char *)&optval, sizeof(optval)) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_get_dontroute(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval;
#else
	int optval;
#endif
	if (__unlikely(getsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_DONTROUTE,
			(char *)&optval, &(socklen_t){ sizeof(optval) })))
		return -1;
	return !!optval;
}

LELY_IO_EXPORT int
io_sock_set_dontroute(io_handle_t handle, int dontroute)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval = !!dontroute;
#else
	int optval = !!dontroute;
#endif
	return setsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_DONTROUTE,
			(const char *)&optval, sizeof(optval)) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_get_error(io_handle_t handle, int *perror)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	return getsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_ERROR,
			(char *)perror, &(socklen_t){ sizeof(*perror) })
			? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_get_keepalive(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval;
#else
	int optval;
#endif
	if (__unlikely(getsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_KEEPALIVE,
			(char *)&optval, &(socklen_t){ sizeof(optval) })))
		return -1;
	return !!optval;
}

LELY_IO_EXPORT int
io_sock_set_keepalive(io_handle_t handle, int keepalive, int time, int interval)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	struct tcp_keepalive kaInBuffer = {
		.onoff = keepalive,
		// The timeout is specified in milliseconds.
		.keepalivetime = time * 1000,
		// The interval is specified in milliseconds.
		.keepaliveinterval = interval * 1000
	};
	DWORD dwBytesReturned;
	return WSAIoctl((SOCKET)handle->fd, SIO_KEEPALIVE_VALS, &kaInBuffer,
			sizeof(kaInBuffer), NULL, 0, &dwBytesReturned, NULL,
			NULL) ? -1 : 0;
#else
	if (__unlikely(setsockopt(handle->fd, SOL_SOCKET, SO_KEEPALIVE,
			&keepalive, sizeof(keepalive)) == -1))
		return -1;
#ifdef __linux__
	if (keepalive) {
		if (__unlikely(setsockopt(handle->fd, SOL_TCP, TCP_KEEPIDLE,
				&time, sizeof(time)) == -1))
			return -1;
		if (__unlikely(setsockopt(handle->fd, SOL_TCP, TCP_KEEPINTVL,
				&interval, sizeof(interval)) == -1))
			return -1;
	}
#else
	__unused_var(time);
	__unused_var(interval);
#endif
	return 0;
#endif
}

LELY_IO_EXPORT int
io_sock_get_linger(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	struct linger optval;
	if (__unlikely(getsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_LINGER,
			(char *)&optval, &(socklen_t){ sizeof(optval) })))
		return -1;
	return optval.l_onoff ? optval.l_linger : 0;
}

LELY_IO_EXPORT int
io_sock_set_linger(io_handle_t handle, int time)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(time < 0)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	struct linger optval = { !!time, time };
	return setsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_LINGER,
			(const char *)&optval, sizeof(optval)) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_get_oobinline(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval;
#else
	int optval;
#endif
	if (__unlikely(getsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_OOBINLINE,
			(char *)&optval, &(socklen_t){ sizeof(optval) })))
		return -1;
	return !!optval;
}

LELY_IO_EXPORT int
io_sock_set_oobinline(io_handle_t handle, int oobinline)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval = !!oobinline;
#else
	int optval = !!oobinline;
#endif
	return setsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_OOBINLINE,
			(const char *)&optval, sizeof(optval)) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_get_rcvbuf(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	int optval;
	if (__unlikely(getsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_RCVBUF,
			(char *)&optval, &(socklen_t){ sizeof(optval) })))
		return -1;
	return !!optval;
}

LELY_IO_EXPORT int
io_sock_set_rcvbuf(io_handle_t handle, int size)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	return setsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_RCVBUF,
			(const char *)&size, sizeof(size)) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_set_rcvtimeo(io_handle_t handle, int timeout)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	DWORD optval = timeout;
#else
	struct timeval optval = {
		.tv_sec = timeout / 1000,
		.tv_usec = (timeout % 1000) * 1000
	};
#endif
	return setsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_RCVTIMEO,
			(const char *)&optval, sizeof(optval)) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_get_reuseaddr(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval;
#else
	int optval;
#endif
	if (__unlikely(getsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_REUSEADDR,
			(char *)&optval, &(socklen_t){ sizeof(optval) })))
		return -1;
	return !!optval;
}

LELY_IO_EXPORT int
io_sock_set_reuseaddr(io_handle_t handle, int reuseaddr)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval = !!reuseaddr;
#else
	int optval = !!reuseaddr;
#endif
	return setsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_REUSEADDR,
			(const char *)&optval, sizeof(optval)) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_get_sndbuf(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	int optval;
	if (__unlikely(getsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_SNDBUF,
			(char *)&optval, &(socklen_t){ sizeof(optval) })))
		return -1;
	return !!optval;
}

LELY_IO_EXPORT int
io_sock_set_sndbuf(io_handle_t handle, int size)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	return setsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_SNDBUF,
			(const char *)&size, sizeof(size)) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_set_sndtimeo(io_handle_t handle, int timeout)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	DWORD optval = timeout;
#else
	struct timeval optval = {
		.tv_sec = timeout / 1000,
		.tv_usec = (timeout % 1000) * 1000
	};
#endif
	return setsockopt((SOCKET)handle->fd, SOL_SOCKET, SO_SNDTIMEO,
			(const char *)&optval, sizeof(optval)) ? -1 :0;
}

LELY_IO_EXPORT int
io_sock_get_tcp_nodelay(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval;
#else
	int optval;
#endif
	if (__unlikely(getsockopt((SOCKET)handle->fd, IPPROTO_TCP, TCP_NODELAY,
			(char *)&optval, &(socklen_t){ sizeof(optval) })))
		return -1;
	return !!optval;
}

LELY_IO_EXPORT int
io_sock_set_tcp_nodelay(io_handle_t handle, int nodelay)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	BOOL optval = !!nodelay;
#else
	int optval = !!nodelay;
#endif
	return setsockopt((SOCKET)handle->fd, IPPROTO_TCP, TCP_NODELAY,
			(const char *)&optval, sizeof(optval)) ? -1 : 0;
}

#if defined(_WIN32) || defined(HAVE_SYS_IOCTL_H)
LELY_IO_EXTERN ssize_t
io_sock_get_nread(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	u_long optval;
	if (__unlikely(ioctlsocket((SOCKET)handle->fd, FIONREAD, &optval)))
		return -1;
	return optval;
#else
	int optval;
	int result;
	do result = ioctl(handle->fd, FIONREAD, &optval);
	while (__unlikely(result == -1 && errno == EINTR));
	if (__unlikely(result == -1))
		return -1;
	return optval;
#endif
}
#endif

#if defined(_WIN32) || defined(__CYGWIN__) || defined(__linux__)

LELY_IO_EXPORT int
io_sock_get_mcast_loop(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &sock_vtab)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

#ifdef _WIN32
	DWORD optval;
#else
	int optval;
#endif
	switch (((struct sock *)handle)->domain) {
	case IO_SOCK_IPV4:
		if (__unlikely(getsockopt((SOCKET)handle->fd, IPPROTO_IP,
				IP_MULTICAST_LOOP, (char *)&optval,
				&(socklen_t){ sizeof(optval) })))
			return -1;
		break;
	case IO_SOCK_IPV6:
		if (__unlikely(getsockopt((SOCKET)handle->fd, IPPROTO_IPV6,
				IPV6_MULTICAST_LOOP, (char *)&optval,
				&(socklen_t){ sizeof(optval) })))
			return -1;
		break;
	default:
		set_errnum(ERRNUM_AFNOSUPPORT);
		return -1;
	}
	return !!optval;
}

LELY_IO_EXPORT int
io_sock_set_mcast_loop(io_handle_t handle, int loop)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &sock_vtab)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

#ifdef _WIN32
	DWORD optval = !!loop;
#else
	int optval = !!loop;
#endif
	switch (((struct sock *)handle)->domain) {
	case IO_SOCK_IPV4:
		return setsockopt((SOCKET)handle->fd, IPPROTO_IP,
				IP_MULTICAST_LOOP, (const char *)&optval,
				sizeof(optval)) ? -1 : 0;
	case IO_SOCK_IPV6:
		return setsockopt((SOCKET)handle->fd, IPPROTO_IPV6,
				IPV6_MULTICAST_LOOP, (const char *)&optval,
				sizeof(optval)) ? -1 : 0;
	default:
		set_errnum(ERRNUM_AFNOSUPPORT);
		return -1;
	}
}

LELY_IO_EXPORT int
io_sock_get_mcast_ttl(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &sock_vtab)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

#ifdef _WIN32
	DWORD optval;
#else
	int optval;
#endif
	switch (((struct sock *)handle)->domain) {
	case IO_SOCK_IPV4:
		if (__unlikely(getsockopt((SOCKET)handle->fd, IPPROTO_IP,
				IP_MULTICAST_TTL, (char *)&optval,
				&(socklen_t){ sizeof(optval) })))
			return -1;
		break;
	case IO_SOCK_IPV6:
		if (__unlikely(getsockopt((SOCKET)handle->fd, IPPROTO_IPV6,
				IPV6_MULTICAST_HOPS, (char *)&optval,
				&(socklen_t){ sizeof(optval) })))
			return -1;
		break;
	default:
		set_errnum(ERRNUM_AFNOSUPPORT);
		return -1;
	}
	return optval;
}

LELY_IO_EXPORT int
io_sock_set_mcast_ttl(io_handle_t handle, int ttl)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &sock_vtab)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

#ifdef _WIN32
	DWORD optval = ttl;
#else
	int optval = ttl;
#endif
	switch (((struct sock *)handle)->domain) {
	case IO_SOCK_IPV4:
		return setsockopt((SOCKET)handle->fd, IPPROTO_IP,
				IP_MULTICAST_TTL, (const char *)&optval,
				sizeof(optval)) ? -1 :0;
	case IO_SOCK_IPV6:
		return setsockopt((SOCKET)handle->fd, IPPROTO_IPV6,
				IPV6_MULTICAST_HOPS, (const char *)&optval,
				sizeof(optval)) ? -1 :0;
	default:
		set_errnum(ERRNUM_AFNOSUPPORT);
		return -1;
	}
}

LELY_IO_EXPORT int
io_sock_mcast_join_group(io_handle_t handle, unsigned int index,
		const io_addr_t *group)
{
	assert(group);

	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &sock_vtab)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

	struct group_req greq;
	memset(&greq, 0, sizeof(greq));
	greq.gr_interface = index;
	memcpy(&greq.gr_group, &group->addr, sizeof(greq.gr_group));

	int level = ((struct sock *)handle)->domain == IO_SOCK_IPV6
			? IPPROTO_IPV6 : IPPROTO_IP;
	return setsockopt((SOCKET)handle->fd, level, MCAST_JOIN_GROUP,
			(const char *)&greq, sizeof(greq)) ? 0 : -1;
}

LELY_IO_EXPORT int
io_sock_mcast_block_source(io_handle_t handle, unsigned int index,
		const io_addr_t *group, const io_addr_t *source)
{
	assert(group);
	assert(source);

	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &sock_vtab)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

	struct group_source_req greq;
	memset(&greq, 0, sizeof(greq));
	greq.gsr_interface = index;
	memcpy(&greq.gsr_group, &group->addr, sizeof(greq.gsr_group));
	memcpy(&greq.gsr_source, &source->addr, sizeof(greq.gsr_source));

	int level = ((struct sock *)handle)->domain == IO_SOCK_IPV6
			? IPPROTO_IPV6 : IPPROTO_IP;
	return setsockopt((SOCKET)handle->fd, level, MCAST_BLOCK_SOURCE,
			(const char *)&greq, sizeof(greq)) ? -1 :0;
}

LELY_IO_EXPORT int
io_sock_mcast_unblock_source(io_handle_t handle, unsigned int index,
		const io_addr_t *group, const io_addr_t *source)
{
	assert(group);
	assert(source);

	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &sock_vtab)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

	struct group_source_req greq;
	memset(&greq, 0, sizeof(greq));
	greq.gsr_interface = index;
	memcpy(&greq.gsr_group, &group->addr, sizeof(greq.gsr_group));
	memcpy(&greq.gsr_source, &source->addr, sizeof(greq.gsr_source));

	int level = ((struct sock *)handle)->domain == IO_SOCK_IPV6
			? IPPROTO_IPV6 : IPPROTO_IP;
	return setsockopt((SOCKET)handle->fd, level, MCAST_UNBLOCK_SOURCE,
			(const char *)&greq, sizeof(greq)) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_mcast_leave_group(io_handle_t handle, unsigned int index,
		const io_addr_t *group)
{
	assert(group);

	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &sock_vtab)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

	struct group_req greq;
	memset(&greq, 0, sizeof(greq));
	greq.gr_interface = index;
	memcpy(&greq.gr_group, &group->addr, sizeof(greq.gr_group));

	int level = ((struct sock *)handle)->domain == IO_SOCK_IPV6
			? IPPROTO_IPV6 : IPPROTO_IP;
	return setsockopt((SOCKET)handle->fd, level, MCAST_LEAVE_GROUP,
			(const char *)&greq, sizeof(greq)) ? -1 : 0;
}

LELY_IO_EXPORT int
io_sock_mcast_join_source_group(io_handle_t handle, unsigned int index,
		const io_addr_t *group, const io_addr_t *source)
{
	assert(group);
	assert(source);

	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &sock_vtab)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

	struct group_source_req greq;
	memset(&greq, 0, sizeof(greq));
	greq.gsr_interface = index;
	memcpy(&greq.gsr_group, &group->addr, sizeof(greq.gsr_group));
	memcpy(&greq.gsr_source, &source->addr, sizeof(greq.gsr_source));

	int level = ((struct sock *)handle)->domain == IO_SOCK_IPV6
			? IPPROTO_IPV6 : IPPROTO_IP;
	return setsockopt((SOCKET)handle->fd, level, MCAST_JOIN_SOURCE_GROUP,
			(const char *)&greq, sizeof(greq)) ? -1 :0;
}

LELY_IO_EXPORT int
io_sock_mcast_leave_source_group(io_handle_t handle, unsigned int index,
		const io_addr_t *group, const io_addr_t *source)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &sock_vtab)) {
		set_errnum(ERRNUM_NOTSOCK);
		return -1;
	}

	struct group_source_req greq;
	memset(&greq, 0, sizeof(greq));
	greq.gsr_interface = index;
	memcpy(&greq.gsr_group, &group->addr, sizeof(greq.gsr_group));
	memcpy(&greq.gsr_source, &source->addr, sizeof(greq.gsr_source));

	int level = ((struct sock *)handle)->domain == IO_SOCK_IPV6
			? IPPROTO_IPV6 : IPPROTO_IP;
	return setsockopt((SOCKET)handle->fd, level, MCAST_LEAVE_SOURCE_GROUP,
			(const char *)&greq, sizeof(greq)) ? -1 : 0;
}

#endif // _WIN32 || __CYGWIN__ || __linux__

static void
sock_fini(struct io_handle *handle)
{
	if (!(handle->flags & IO_FLAG_NO_CLOSE))
		closesocket((SOCKET)handle->fd);
}

static int
sock_flags(struct io_handle *handle, int flags)
{
	assert(handle);

#ifdef _WIN32
	u_long iMode = !!(flags & IO_FLAG_NONBLOCK);
	return ioctlsocket((SOCKET)handle->fd, FIONBIO, &iMode) ? -1 : 0;
#else
	int arg = fcntl(handle->fd, F_GETFL, 0);
	if (__unlikely(arg == -1))
		return -1;

	if (flags & IO_FLAG_NONBLOCK)
		arg |= O_NONBLOCK;
	else
		arg &= ~O_NONBLOCK;

	return fcntl(handle->fd, F_SETFL, arg);
#endif
}

static ssize_t
sock_read(struct io_handle *handle, void *buf, size_t nbytes)
{
	return sock_recv(handle, buf, nbytes, NULL, 0);
}

static ssize_t
sock_write(struct io_handle *handle, const void *buf, size_t nbytes)
{
	return sock_send(handle, buf, nbytes, NULL, 0);
}

static ssize_t
sock_recv(struct io_handle *handle, void *buf, size_t nbytes, io_addr_t *addr,
		int flags)
{
	assert(handle);

	int _flags = 0;
	if (flags & IO_MSG_PEEK)
		_flags |= MSG_PEEK;
	if (flags & IO_MSG_OOB)
		_flags |= MSG_OOB;
	if (flags & IO_MSG_WAITALL)
		_flags |= MSG_WAITALL;

	ssize_t result;
	if (addr) {
		addr->addrlen = sizeof(addr->addr);
#ifdef _WIN32
		result = recvfrom((SOCKET)handle->fd, buf, nbytes, _flags,
				(struct sockaddr *)&addr->addr, &addr->addrlen);
#else
		do result = recvfrom(handle->fd, buf, nbytes, _flags,
				(struct sockaddr *)&addr->addr,
				(socklen_t *)&addr->addrlen);
		while (__unlikely(result == -1 && errno == EINTR));
#endif
	} else {
#ifdef _WIN32
		result = recv((SOCKET)handle->fd, buf, nbytes, _flags);
#else
		do result = recv(handle->fd, buf, nbytes, _flags);
		while (__unlikely(result == -1 && errno == EINTR));
#endif
	}
	return result == SOCKET_ERROR ? -1 : result;
}

static ssize_t
sock_send(struct io_handle *handle, const void *buf, size_t nbytes,
		const io_addr_t *addr, int flags)
{
	assert(handle);

	int _flags = 0;
	if (flags & IO_MSG_OOB)
		_flags |= MSG_OOB;
#ifndef _WIN32
	_flags |= MSG_NOSIGNAL;
#endif

	ssize_t result;
#ifdef _WIN32
	result = addr
			? sendto((SOCKET)handle->fd, buf, nbytes, _flags,
			(const struct sockaddr *)&addr->addr, addr->addrlen)
			: send((SOCKET)handle->fd, buf, nbytes, _flags);
#else
	do result = addr
			? sendto(handle->fd, buf, nbytes, _flags,
			(const struct sockaddr *)&addr->addr, addr->addrlen)
			: send(handle->fd, buf, nbytes, _flags);
	while (__unlikely(result == -1 && errno == EINTR));
#endif
	return result == SOCKET_ERROR ? -1 : result;
}

static struct io_handle *
sock_accept(struct io_handle *handle, io_addr_t *addr)
{
	struct sock *sock = (struct sock *)handle;
	assert(sock);

	errc_t errc = 0;

	SOCKET s;
	if (addr) {
		addr->addrlen = sizeof(addr->addr);
#ifdef _WIN32
		s = accept((SOCKET)handle->fd, (struct sockaddr *)&addr->addr,
				&addr->addrlen);
#else
#ifdef _GNU_SOURCE
		do s = accept4(handle->fd, (struct sockaddr *)&addr->addr,
				(socklen_t *)&addr->addrlen, SOCK_CLOEXEC);
#else
		do s = accept(handle->fd, (struct sockaddr *)&addr->addr,
				(socklen_t *)&addr->addrlen);
#endif
		while (__unlikely(s == -1 && errno == EINTR));
#endif
	} else {
#ifdef _WIN32
		s = accept((SOCKET)handle->fd, NULL, NULL);
#else
#ifdef _GNU_SOURCE
		do s = accept4(handle->fd, NULL, NULL, SOCK_CLOEXEC);
#else
		do s = accept(handle->fd, NULL, NULL);
#endif
		while (__unlikely(s == -1 && errno == EINTR));
#endif
	}

	if (__unlikely(s == INVALID_SOCKET)) {
		errc = get_errc();
		goto error_accept;
	}

#if _POSIX_C_SOURCE >= 200112L && !defined(_GNU_SOURCE)
	if (__unlikely(fcntl(s, F_SETFD, FD_CLOEXEC) == -1)) {
		errc = get_errc();
		goto error_fcntl;
	}
#endif

	handle = io_handle_alloc(&sock_vtab);
	if (__unlikely(!handle)) {
		errc = get_errc();
		goto error_alloc_handle;
	}

	handle->fd = (HANDLE)s;
	((struct sock *)handle)->domain = sock->domain;
	((struct sock *)handle)->type = sock->type;

	return handle;

error_alloc_handle:
#if _POSIX_C_SOURCE >= 200112L && !defined(_GNU_SOURCE)
error_fcntl:
#endif
#ifdef _WIN32
	closesocket(s);
#else
	close(s);
#endif
error_accept:
	set_errc(errc);
	return IO_HANDLE_ERROR;
}

static int
sock_connect(struct io_handle *handle, const io_addr_t *addr)
{
	assert(handle);

#ifdef _WIN32
	return connect((SOCKET)handle->fd, (const struct sockaddr *)&addr->addr,
			addr->addrlen) ? -1 : 0;
#else
	int result;
	do result = connect(handle->fd, (const struct sockaddr *)&addr->addr,
			addr->addrlen);
	while (__unlikely(result == -1 && errno == EINTR));
	return result;
#endif
}

int
_socketpair(int af, int type, int protocol, SOCKET sv[2])
{
	assert(sv);
	sv[0] = sv[1] = INVALID_SOCKET;

#if _POSIX_C_SOURCE >= 200112L
	if (af == AF_UNIX)
		return socketpair(af, type, protocol, sv);
#endif

	errc_t errc = 0;

	if (__unlikely(af != AF_INET && af != AF_INET6)) {
		errc = errnum2c(ERRNUM_AFNOSUPPORT);
		goto error_param;
	}

	int flags = 0;
#if defined(__CYGWIN__) || defined(__linux__)
	flags = type & (SOCK_NONBLOCK | SOCK_CLOEXEC);
	type &= ~flags;
#endif

	if (__unlikely(type != SOCK_STREAM && type != SOCK_DGRAM)) {
		errc = errnum2c(ERRNUM_PROTOTYPE);
		goto error_param;
	}

	sv[0] = socket(af, type | flags, protocol);
	if (__unlikely(sv[0] == INVALID_SOCKET)) {
		errc = get_errc();
		goto error_socket_0;
	}

	sv[1] = socket(af, type | flags, protocol);
	if (__unlikely(sv[1] == INVALID_SOCKET)) {
		errc = get_errc();
		goto error_socket_1;
	}

	struct sockaddr_storage name[2];
	struct sockaddr *name_0 = (struct sockaddr *)&name[0];
	struct sockaddr_in *name_in_0 = (struct sockaddr_in *)name_0;
	struct sockaddr_in6 *name_in6_0 = (struct sockaddr_in6 *)name_0;
	struct sockaddr *name_1 = (struct sockaddr *)&name[1];
	struct sockaddr_in *name_in_1 = (struct sockaddr_in *)name_1;
	struct sockaddr_in6 *name_in6_1 = (struct sockaddr_in6 *)name_1;
	socklen_t namelen_0, namelen_1;

	if (af == AF_INET) {
		name_in_1->sin_family = AF_INET;
		name_in_1->sin_port = 0;
		name_in_1->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		namelen_1 = sizeof(*name_in_1);
	} else {
		name_in6_1->sin6_family = AF_INET6;
		name_in6_1->sin6_port = 0;
		name_in6_1->sin6_addr = in6addr_loopback;
		namelen_1 = sizeof(*name_in6_1);
	}

	if (__unlikely(bind(sv[1], name_1, namelen_1) == SOCKET_ERROR)) {
		errc = get_errc();
		goto error_bind_1;
	}
	if (__unlikely(getsockname(sv[1], name_1, &namelen_1)
			== SOCKET_ERROR)) {
		errc = get_errc();
		goto error_getsockname_1;
	}

	if (type == SOCK_STREAM) {
		if (__unlikely(listen(sv[1], 1) == SOCKET_ERROR)) {
			errc = get_errc();
			goto error_listen;
		}
	} else {
		if (af == AF_INET) {
			name_in_0->sin_family = AF_INET;
			name_in_0->sin_port = 0;
			name_in_0->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			namelen_0 = sizeof(*name_in_0);
		} else {
			name_in6_0->sin6_family = AF_INET6;
			name_in6_0->sin6_port = 0;
			name_in6_0->sin6_addr = in6addr_loopback;
			namelen_0 = sizeof(*name_in6_0);
		}

		if (__unlikely(bind(sv[0], name_0, namelen_0)
				== SOCKET_ERROR)) {
			errc = get_errc();
			goto error_bind_0;
		}
		if (__unlikely(getsockname(sv[0], name_0, &namelen_0)
				== SOCKET_ERROR)) {
			errc = get_errc();
			goto error_getsockname_0;
		}
	}

	if (af == AF_INET)
		name_in_1->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	else
		name_in6_1->sin6_addr = in6addr_loopback;
	if (__unlikely(connect(sv[0], name_1, namelen_1) == SOCKET_ERROR)) {
		errc = get_errc();
		goto error_connect_0;
	}

	if (type == SOCK_STREAM) {
		SOCKET s = accept(sv[1], NULL, NULL);
		if (__unlikely(s == INVALID_SOCKET)) {
			errc = get_errc();
			goto error_accept;
		}
		closesocket(sv[1]);
		sv[1] = s;
	} else {
		if (af == AF_INET)
			name_in_0->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		else
			name_in6_0->sin6_addr = in6addr_loopback;
		if (__unlikely(connect(sv[1], name_0, namelen_0)
				== SOCKET_ERROR)) {
			errc = get_errc();
			goto error_connect_1;
		}
	}

	return 0;

error_connect_1:
error_accept:
error_connect_0:
error_getsockname_0:
error_bind_0:
error_listen:
error_getsockname_1:
error_bind_1:
	closesocket(sv[1]);
	sv[1] = INVALID_SOCKET;
error_socket_1:
	closesocket(sv[0]);
	sv[0] = INVALID_SOCKET;
error_socket_0:
error_param:
	set_errc(errc);
	return -1;
}

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

