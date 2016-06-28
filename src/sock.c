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

#if defined(_WIN32) || _POSIX_C_SOURCE >= 200112L

//! A network socket.
struct sock {
	//! The shared I/O device handle.
	struct io_handle handle;
	/*!
	 * The domain of the socket (one of #IO_SOCK_BTH, #IO_SOCK_CAN,
	 * #IO_SOCK_IPV4, #IO_SOCK_IPV6 or #IO_SOCK_UNIX).
	 */
	int domain;
	//! The type of the socket (#IO_SOCK_STREAM or #IO_SOCK_DGRAM).
	int type;
};

static void sock_fini(struct io_handle *handle);
static ssize_t sock_read(struct io_handle *handle, void *buf, size_t nbytes);
static ssize_t sock_write(struct io_handle *handle, const void *buf,
		size_t nbytes);
static ssize_t sock_recv(struct io_handle *handle, void *buf, size_t nbytes,
		io_addr_t *addr);
static ssize_t sock_send(struct io_handle *handle, const void *buf,
		size_t nbytes, const io_addr_t *addr);
static struct io_handle *sock_accept(struct io_handle *handle, io_addr_t *addr);
static int sock_connect(struct io_handle *handle, const io_addr_t *addr);

static const struct io_handle_vtab sock_vtab = {
	.size = sizeof(struct sock),
	.fini = &sock_fini,
	.read = &sock_read,
	.write = &sock_write,
	.accept = &sock_accept,
	.connect = &sock_connect
};

#ifdef _WIN32
static int socketpair(int domain, int type, int protocol,
		SOCKET socket_vector[2]);
#endif

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
#if defined(__linux__) && defined(HAVE_LINUX_CAN_H)
	case IO_SOCK_CAN:
		switch (type) {
		case IO_SOCK_DGRAM:
			s = socket(AF_CAN, SOCK_RAW | flags, CAN_RAW);
#ifdef HAVE_CAN_RAW_FD_FRAMES
			if (__unlikely(s != -1 && setsockopt(s, SOL_CAN_RAW,
					CAN_RAW_FD_FRAMES, &(int){ 1 },
					sizeof(int)) == -1)) {
				errc = get_errc();
				goto error_setsockopt;
			}
#endif
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

	return handle;

error_alloc_handle:
#if _POSIX_C_SOURCE >= 200112L && !defined(__CYGINW__) && !defined(__linux__)
error_fcntl:
#endif
#if defined(__linux__) && defined(HAVE_LINUX_CAN_H) && HAVE_CAN_RAW_FD_FRAMES
error_setsockopt:
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
			result = socketpair(AF_INET, SOCK_STREAM | flags, 0,
					socket_vector);
			break;
		case IO_SOCK_DGRAM:
			result = socketpair(AF_INET, SOCK_DGRAM | flags, 0,
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
			result = socketpair(AF_INET6, SOCK_STREAM | flags, 0,
					socket_vector);
			break;
		case IO_SOCK_DGRAM:
			result = socketpair(AF_INET6, SOCK_DGRAM | flags, 0,
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
			result = socketpair(AF_UNIX, SOCK_STREAM | flags, 0,
					socket_vector);
			break;
		case IO_SOCK_DGRAM:
			result = socketpair(AF_UNIX, SOCK_DGRAM | flags, 0,
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

	if (__unlikely(result == -1)) {
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
#ifdef _WIN32
	return io_open_socketpair(IO_SOCK_IPV4, IO_SOCK_STREAM, handle_vector);
#else
	return io_open_socketpair(IO_SOCK_UNIX, IO_SOCK_STREAM, handle_vector);
#endif
}

LELY_IO_EXPORT ssize_t
io_recv(io_handle_t handle, void *buf, size_t nbytes, io_addr_t *addr)
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

	return handle->vtab->recv(handle, buf, nbytes, addr);
}

LELY_IO_EXPORT ssize_t
io_send(io_handle_t handle, const void *buf, size_t nbytes,
		const io_addr_t *addr)
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

	return handle->vtab->send(handle, buf, nbytes, addr);
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

static void
sock_fini(struct io_handle *handle)
{
	closesocket((SOCKET)handle->fd);
}

static ssize_t
sock_read(struct io_handle *handle, void *buf, size_t nbytes)
{
	return sock_recv(handle, buf, nbytes, NULL);
}

static ssize_t
sock_write(struct io_handle *handle, const void *buf, size_t nbytes)
{
	return sock_send(handle, buf, nbytes, NULL);
}

static ssize_t
sock_recv(struct io_handle *handle, void *buf, size_t nbytes, io_addr_t *addr)
{
	assert(handle);

	ssize_t result;
	if (addr) {
		addr->addrlen = sizeof(addr->addr);
#ifdef _WIN32
		result = recvfrom((SOCKET)handle->fd, buf, nbytes, 0,
				(struct sockaddr *)&addr->addr, &addr->addrlen);
#else
		do result = recvfrom(handle->fd, buf, nbytes, 0,
				(struct sockaddr *)&addr->addr,
				(socklen_t *)&addr->addrlen);
		while (__unlikely(result == -1 && errno == EINTR));
#endif
	} else {
#ifdef _WIN32
		result = recv((SOCKET)handle->fd, buf, nbytes, 0);
#else
		do result = recv(handle->fd, buf, nbytes, 0);
		while (__unlikely(result == -1 && errno == EINTR));
#endif
	}
	return result == SOCKET_ERROR ? -1 : result;
}

static ssize_t
sock_send(struct io_handle *handle, const void *buf, size_t nbytes,
		const io_addr_t *addr)
{
	assert(handle);

	ssize_t result;
#ifdef _WIN32
	result = addr
			? sendto((SOCKET)handle->fd, buf, nbytes, 0,
			(const struct sockaddr *)&addr->addr, addr->addrlen)
			: send((SOCKET)handle->fd, buf, nbytes, 0);
#else
	do result = addr
			? sendto(handle->fd, buf, nbytes, MSG_NOSIGNAL,
			(const struct sockaddr *)&addr->addr, addr->addrlen)
			: send(handle->fd, buf, nbytes, MSG_NOSIGNAL);
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

#ifdef _WIN32
static int
socketpair(int domain, int type, int protocol, SOCKET socket_vector[2])
{
	assert(socket_vector);
	socket_vector[0] = socket_vector[1] = INVALID_SOCKET;

	SOCKADDR_STORAGE name_vector[2];
	SOCKADDR *name_0 = (SOCKADDR *)&name_vector[0];
	SOCKADDR_IN *name_in_0 = (SOCKADDR_IN *)name_0;
	SOCKADDR_IN6 *name_in6_0 = (SOCKADDR_IN6 *)name_0;
	SOCKADDR *name_1 = (SOCKADDR *)&name_vector[1];
	SOCKADDR_IN *name_in_1 = (SOCKADDR_IN *)name_1;
	SOCKADDR_IN6 *name_in6_1 = (SOCKADDR_IN6 *)name_1;
	int namelen_0, namelen_1;

	int iError = 0;

	if (__unlikely(domain != AF_INET && domain != AF_INET6)) {
		iError = WSAEAFNOSUPPORT;
		goto error_param;
	}

	if (__unlikely(type != SOCK_STREAM && type != SOCK_DGRAM)) {
		iError = WSAEPROTOTYPE;
		goto error_param;
	}

	socket_vector[0] = socket(domain, type, protocol);
	if (__unlikely(socket_vector[0] == INVALID_SOCKET)) {
		iError = WSAGetLastError();
		goto error_socket_0;
	}

	socket_vector[1] = socket(domain, type, protocol);
	if (__unlikely(socket_vector[1] == INVALID_SOCKET)) {
		iError = WSAGetLastError();
		goto error_socket_1;
	}

	if (domain == AF_INET) {
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

	if (__unlikely(bind(socket_vector[1], name_1, namelen_1)
			== SOCKET_ERROR)) {
		iError = WSAGetLastError();
		goto error_bind_1;
	}
	if (__unlikely(getsockname(socket_vector[1], name_1, &namelen_1)
			== SOCKET_ERROR)) {
		iError = WSAGetLastError();
		goto error_getsockname_1;
	}

	if (type == SOCK_STREAM) {
		if (__unlikely(listen(socket_vector[1], 1) == SOCKET_ERROR)) {
			iError = WSAGetLastError();
			goto error_listen;
		}
	} else {
		if (domain == AF_INET) {
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

		if (__unlikely(bind(socket_vector[0], name_0, namelen_0)
				== SOCKET_ERROR)) {
			iError = WSAGetLastError();
			goto error_bind_0;
		}
		if (__unlikely(getsockname(socket_vector[0], name_0, &namelen_0)
				== SOCKET_ERROR)) {
			iError = WSAGetLastError();
			goto error_getsockname_0;
		}
	}

	if (domain == AF_INET)
		name_in_1->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	else
		name_in6_1->sin6_addr = in6addr_loopback;
	if (__unlikely(connect(socket_vector[0], name_1, namelen_1)
			== SOCKET_ERROR)) {
		iError = WSAGetLastError();
		goto error_connect_0;
	}

	if (type == SOCK_STREAM) {
		SOCKET s = accept(socket_vector[1], NULL, NULL);
		if (__unlikely(s == INVALID_SOCKET)) {
			iError = WSAGetLastError();
			goto error_accept;
		}
		closesocket(socket_vector[1]);
		socket_vector[1] = s;
	} else {
		if (domain == AF_INET)
			name_in_0->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		else
			name_in6_0->sin6_addr = in6addr_loopback;
		if (__unlikely(connect(socket_vector[1], name_0, namelen_0)
				== SOCKET_ERROR)) {
			iError = WSAGetLastError();
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
	closesocket(socket_vector[1]);
	socket_vector[1] = INVALID_SOCKET;
error_socket_1:
	closesocket(socket_vector[0]);
	socket_vector[0] = INVALID_SOCKET;
error_socket_0:
error_param:
	WSASetLastError(iError);
	return -1;
}
#endif // _WIN32

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

