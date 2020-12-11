/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * network address functions.
 *
 * @see lely/io/addr.h
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#include "io.h"

#if !LELY_NO_STDIO

#include <lely/io/addr.h>
#include <lely/io/sock.h>
#include <lely/util/cmp.h>
#include <lely/util/errnum.h>

#include <assert.h>
#include <string.h>

#if _WIN32

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static int ba2str(const BTH_ADDR *ba, char *str);
static int str2ba(const char *str, BTH_ADDR *ba);
static int bachk(const char *str);

#elif _POSIX_C_SOURCE >= 200112L
#include <netdb.h>
#endif

int
io_addr_cmp(const void *p1, const void *p2)
{
	if (p1 == p2)
		return 0;

	if (!p1)
		return -1;
	if (!p2)
		return 1;

	const io_addr_t *a1 = p1;
	const io_addr_t *a2 = p2;

	int cmp = memcmp(&a1->addr, &a2->addr, MIN(a1->addrlen, a2->addrlen));
	if (!cmp)
		cmp = (a2->addrlen < a1->addrlen) - (a1->addrlen < a2->addrlen);
	return cmp;
}

// clang-format off
#if _WIN32 || (defined(__linux__) \
		&& defined(HAVE_BLUETOOTH_BLUETOOTH_H) \
		&& defined(HAVE_BLUETOOTH_RFCOMM_H))
// clang-format on

int
io_addr_get_rfcomm_a(const io_addr_t *addr, char *ba, int *port)
{
	assert(addr);

#if _WIN32
	if (addr->addrlen < (int)sizeof(SOCKADDR_BTH)) {
		WSASetLastError(WSAEINVAL);
		return -1;
	}

	const SOCKADDR_BTH *addr_bth = (const SOCKADDR_BTH *)&addr->addr;
	if (addr_bth->addressFamily != AF_BTH) {
		WSASetLastError(WSAEAFNOSUPPORT);
		return -1;
	}

	if (port)
		*port = addr_bth->port == BT_PORT_ANY ? 0 : addr_bth->port;
	if (ba && ba2str(&addr_bth->btAddr, ba) < 0)
		return -1;
#else
	if (addr->addrlen < (int)sizeof(struct sockaddr_rc)) {
		errno = EINVAL;
		return -1;
	}

	const struct sockaddr_rc *addr_rc =
			(const struct sockaddr_rc *)&addr->addr;
	if (addr_rc->rc_family != AF_BLUETOOTH) {
		errno = EAFNOSUPPORT;
		return -1;
	}

	if (port)
		*port = btohs(addr_rc->rc_channel);
	if (ba && ba2str(&addr_rc->rc_bdaddr, ba) < 0)
		return -1;
#endif

	return 0;
}

int
io_addr_set_rfcomm_a(io_addr_t *addr, const char *ba, int port)
{
	assert(addr);

	memset(addr, 0, sizeof(*addr));
#if _WIN32
	addr->addrlen = sizeof(SOCKADDR_BTH);
	SOCKADDR_BTH *addr_bth = (SOCKADDR_BTH *)&addr->addr;

	addr_bth->addressFamily = AF_BTH;
	addr_bth->port = port ? (ULONG)port : BT_PORT_ANY;
	addr_bth->btAddr = 0;
	if (ba && *ba) {
		if (str2ba(ba, &addr_bth->btAddr) < 0)
			return -1;
	}
#else
	addr->addrlen = sizeof(struct sockaddr_rc);
	struct sockaddr_rc *addr_rc = (struct sockaddr_rc *)&addr->addr;

	addr_rc->rc_family = AF_BLUETOOTH;
	addr_rc->rc_channel = htobs(port);
	if (ba && *ba) {
		if (str2ba(ba, &addr_rc->rc_bdaddr) < 0)
			return -1;
	} else {
		bacpy(&addr_rc->rc_bdaddr, BDADDR_ANY);
	}
#endif

	return 0;
}

int
io_addr_get_rfcomm_n(const io_addr_t *addr, uint8_t ba[6], int *port)
{
	assert(addr);

#if _WIN32
	if (addr->addrlen < (int)sizeof(SOCKADDR_BTH)) {
		WSASetLastError(WSAEINVAL);
		return -1;
	}

	const SOCKADDR_BTH *addr_bth = (const SOCKADDR_BTH *)&addr->addr;
	if (addr_bth->addressFamily != AF_BTH) {
		WSASetLastError(WSAEAFNOSUPPORT);
		return -1;
	}

	if (port)
		*port = addr_bth->port == BT_PORT_ANY ? 0 : addr_bth->port;
	if (ba) {
		for (int i = 0; i < 6; i++)
			ba[i] = (addr_bth->btAddr >> (7 - i) * 8) & 0xff;
	}
#else
	if (addr->addrlen < (int)sizeof(struct sockaddr_rc)) {
		errno = EINVAL;
		return -1;
	}

	const struct sockaddr_rc *addr_rc =
			(const struct sockaddr_rc *)&addr->addr;
	if (addr_rc->rc_family != AF_BLUETOOTH) {
		errno = EAFNOSUPPORT;
		return -1;
	}

	if (port)
		*port = btohs(addr_rc->rc_channel);
	if (ba)
		memcpy(ba, &addr_rc->rc_bdaddr, 6);
#endif

	return 0;
}

void
io_addr_set_rfcomm_n(io_addr_t *addr, const uint8_t ba[6], int port)
{
	assert(addr);

	memset(addr, 0, sizeof(*addr));
#if _WIN32
	addr->addrlen = sizeof(SOCKADDR_BTH);
	SOCKADDR_BTH *addr_bth = (SOCKADDR_BTH *)&addr->addr;

	addr_bth->addressFamily = AF_BTH;
	addr_bth->port = port ? (ULONG)port : BT_PORT_ANY;
	addr_bth->btAddr = 0;
	if (ba && *ba) {
		for (int i = 0; i < 6; i++)
			addr_bth->btAddr |= (BTH_ADDR)ba[i] << (7 - i) * 8;
	}
#else
	addr->addrlen = sizeof(struct sockaddr_rc);
	struct sockaddr_rc *addr_rc = (struct sockaddr_rc *)&addr->addr;

	addr_rc->rc_family = AF_BLUETOOTH;
	addr_rc->rc_channel = htobs(port);
	if (ba && *ba)
		memcpy(&addr_rc->rc_bdaddr, ba, 6);
	else
		bacpy(&addr_rc->rc_bdaddr, BDADDR_ANY);
#endif
}

void
io_addr_set_rfcomm_local(io_addr_t *addr, int port)
{
	assert(addr);

	memset(addr, 0, sizeof(*addr));
#if _WIN32
	addr->addrlen = sizeof(SOCKADDR_BTH);
	SOCKADDR_BTH *addr_bth = (SOCKADDR_BTH *)&addr->addr;

	addr_bth->addressFamily = AF_BTH;
	addr_bth->port = port ? (ULONG)port : BT_PORT_ANY;
	addr_bth->btAddr = (BTH_ADDR)0xffffff000000ull;
#else
	addr->addrlen = sizeof(struct sockaddr_rc);
	struct sockaddr_rc *addr_rc = (struct sockaddr_rc *)&addr->addr;

	addr_rc->rc_family = AF_BLUETOOTH;
	addr_rc->rc_channel = htobs(port);
	bacpy(&addr_rc->rc_bdaddr, BDADDR_LOCAL);
#endif
}

// clang-format off
#endif // _WIN32 || (__linux__ && HAVE_BLUETOOTH_BLUETOOTH_H && HAVE_BLUETOOTH_RFCOMM_H)
// clang-format on

#if _WIN32 || _POSIX_C_SOURCE >= 200112L

int
io_addr_get_ipv4_a(const io_addr_t *addr, char *ip, int *port)
{
	assert(addr);

	if (addr->addrlen < (int)sizeof(struct sockaddr_in)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	const struct sockaddr_in *addr_in =
			(const struct sockaddr_in *)&addr->addr;
	if (addr_in->sin_family != AF_INET) {
		set_errnum(ERRNUM_AFNOSUPPORT);
		return -1;
	}

	if (port)
		*port = ntohs(addr_in->sin_port);
	// clang-format off
	if (ip && !inet_ntop(AF_INET, (void *)&addr_in->sin_addr, ip,
			IO_ADDR_IPV4_STRLEN))
		// clang-format on
		return -1;

	return 0;
}

int
io_addr_set_ipv4_a(io_addr_t *addr, const char *ip, int port)
{
	assert(addr);

	memset(addr, 0, sizeof(*addr));
	addr->addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in *addr_in = (struct sockaddr_in *)&addr->addr;

	addr_in->sin_family = AF_INET;
	addr_in->sin_port = htons(port);
	if (ip && *ip) {
		if (inet_pton(AF_INET, ip, &addr_in->sin_addr) != 1)
			return -1;
	} else {
		addr_in->sin_addr.s_addr = htonl(INADDR_ANY);
	}

	return 0;
}

int
io_addr_get_ipv4_n(const io_addr_t *addr, uint8_t ip[4], int *port)
{
	assert(addr);

	if (addr->addrlen < (int)sizeof(struct sockaddr_in)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	const struct sockaddr_in *addr_in =
			(const struct sockaddr_in *)&addr->addr;
	if (addr_in->sin_family != AF_INET) {
		set_errnum(ERRNUM_AFNOSUPPORT);
		return -1;
	}

	if (port)
		*port = ntohs(addr_in->sin_port);
	if (ip)
		memcpy(ip, &addr_in->sin_addr.s_addr, 4);

	return 0;
}

void
io_addr_set_ipv4_n(io_addr_t *addr, const uint8_t ip[4], int port)
{
	assert(addr);

	memset(addr, 0, sizeof(*addr));
	addr->addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in *addr_in = (struct sockaddr_in *)&addr->addr;

	addr_in->sin_family = AF_INET;
	addr_in->sin_port = htons(port);
	if (ip)
		memcpy(&addr_in->sin_addr.s_addr, ip, 4);
	else
		addr_in->sin_addr.s_addr = htonl(INADDR_ANY);
}

void
io_addr_set_ipv4_loopback(io_addr_t *addr, int port)
{
	assert(addr);

	memset(addr, 0, sizeof(*addr));
	addr->addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in *addr_in = (struct sockaddr_in *)&addr->addr;

	addr_in->sin_family = AF_INET;
	addr_in->sin_port = htons(port);
	addr_in->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
}

void
io_addr_set_ipv4_broadcast(io_addr_t *addr, int port)
{
	assert(addr);

	memset(addr, 0, sizeof(*addr));
	addr->addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in *addr_in = (struct sockaddr_in *)&addr->addr;

	addr_in->sin_family = AF_INET;
	addr_in->sin_port = htons(port);
	addr_in->sin_addr.s_addr = htonl(INADDR_BROADCAST);
}

int
io_addr_get_ipv6_a(const io_addr_t *addr, char *ip, int *port)
{
	assert(addr);

	if (addr->addrlen < (int)sizeof(struct sockaddr_in6)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	const struct sockaddr_in6 *addr_in6 =
			(const struct sockaddr_in6 *)&addr->addr;
	if (addr_in6->sin6_family != AF_INET6) {
		set_errnum(ERRNUM_AFNOSUPPORT);
		return -1;
	}

	if (port)
		*port = ntohs(addr_in6->sin6_port);
	// clang-format off
	if (ip && !inet_ntop(AF_INET6, (void *)&addr_in6->sin6_addr, ip,
			IO_ADDR_IPV6_STRLEN))
		// clang-format on
		return -1;

	return 0;
}

int
io_addr_set_ipv6_a(io_addr_t *addr, const char *ip, int port)
{
	assert(addr);

	memset(addr, 0, sizeof(*addr));
	addr->addrlen = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&addr->addr;

	addr_in6->sin6_family = AF_INET6;
	addr_in6->sin6_port = htons(port);
	if (ip && *ip) {
		if (inet_pton(AF_INET6, ip, &addr_in6->sin6_addr) != 1)
			return -1;
	} else {
		addr_in6->sin6_addr = in6addr_any;
	}

	return 0;
}

int
io_addr_get_ipv6_n(const io_addr_t *addr, uint8_t ip[16], int *port)
{
	assert(addr);

	if (addr->addrlen < (int)sizeof(struct sockaddr_in6)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	const struct sockaddr_in6 *addr_in6 =
			(const struct sockaddr_in6 *)&addr->addr;
	if (addr_in6->sin6_family != AF_INET6) {
		set_errnum(ERRNUM_AFNOSUPPORT);
		return -1;
	}

	if (port)
		*port = ntohs(addr_in6->sin6_port);
	if (ip)
		memcpy(ip, &addr_in6->sin6_addr.s6_addr, 16);

	return 0;
}

void
io_addr_set_ipv6_n(io_addr_t *addr, const uint8_t ip[16], int port)
{
	assert(addr);

	memset(addr, 0, sizeof(*addr));
	addr->addrlen = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&addr->addr;

	addr_in6->sin6_family = AF_INET6;
	addr_in6->sin6_port = htons(port);
	if (ip)
		memcpy(&addr_in6->sin6_addr.s6_addr, ip, 16);
	else
		addr_in6->sin6_addr = in6addr_any;
}

void
io_addr_set_ipv6_loopback(io_addr_t *addr, int port)
{
	assert(addr);

	memset(addr, 0, sizeof(*addr));
	addr->addrlen = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&addr->addr;

	addr_in6->sin6_family = AF_INET6;
	addr_in6->sin6_port = htons(port);
	addr_in6->sin6_addr = in6addr_loopback;
}

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

#if _POSIX_C_SOURCE >= 200112L

int
io_addr_get_unix(const io_addr_t *addr, char *path)
{
	assert(addr);

	if (addr->addrlen < (int)sizeof(struct sockaddr_un)) {
		errno = EINVAL;
		return -1;
	}

	const struct sockaddr_un *addr_un =
			(const struct sockaddr_un *)&addr->addr;
	if (addr_un->sun_family != AF_UNIX) {
		errno = EAFNOSUPPORT;
		return -1;
	}

	if (path)
		strncpy(path, addr_un->sun_path, IO_ADDR_UNIX_STRLEN);

	return 0;
}

void
io_addr_set_unix(io_addr_t *addr, const char *path)
{
	assert(addr);
	assert(path);

	memset(addr, 0, sizeof(*addr));
	addr->addrlen = sizeof(struct sockaddr_un);
	struct sockaddr_un *addr_un = (struct sockaddr_un *)&addr->addr;

	addr_un->sun_family = AF_UNIX;

	size_t n = MIN(strlen(path), sizeof(addr_un->sun_path) - 1);
	strncpy(addr_un->sun_path, path, n);
	addr_un->sun_path[n] = '\0';
}

#endif // _POSIX_C_SOURCE >= 200112L

#if _WIN32 || _POSIX_C_SOURCE >= 200112L

int
io_addr_get_domain(const io_addr_t *addr)
{
	assert(addr);

	switch (((const struct sockaddr *)&addr->addr)->sa_family) {
#if _WIN32
	case AF_BTH: return IO_SOCK_BTH;
#elif defined(__linux__) && defined(HAVE_BLUETOOTH_BLUETOOTH_H) \
		&& defined(HAVE_BLUETOOTH_RFCOMM_H)
	case AF_BLUETOOTH: return IO_SOCK_BTH;
#endif
	case AF_INET: return IO_SOCK_IPV4;
	case AF_INET6: return IO_SOCK_IPV6;
#if _POSIX_C_SOURCE >= 200112L
	case AF_UNIX: return IO_SOCK_UNIX;
#endif
	default: set_errnum(ERRNUM_AFNOSUPPORT); return -1;
	}
}

int
io_addr_get_port(const io_addr_t *addr, int *port)
{
	assert(addr);

	if (addr->addrlen < (int)sizeof(sa_family_t)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	switch (((const struct sockaddr *)&addr->addr)->sa_family) {
#if _WIN32
	case AF_BTH: {
		if (addr->addrlen < (int)sizeof(SOCKADDR_BTH)) {
			WSASetLastError(WSAEINVAL);
			return -1;
		}
		const SOCKADDR_BTH *addr_bth =
				(const SOCKADDR_BTH *)&addr->addr;
		if (port)
			// clang-format off
			*port = addr_bth->port == BT_PORT_ANY
					? 0 : addr_bth->port;
		// clang-format on
		return 0;
	}
#elif defined(__linux__) && defined(HAVE_BLUETOOTH_BLUETOOTH_H) \
		&& defined(HAVE_BLUETOOTH_RFCOMM_H)
	case AF_BLUETOOTH: {
		if (addr->addrlen < (int)sizeof(struct sockaddr_rc)) {
			errno = EINVAL;
			return -1;
		}
		const struct sockaddr_rc *addr_rc =
				(const struct sockaddr_rc *)&addr->addr;
		if (port)
			*port = btohs(addr_rc->rc_channel);
		return 0;
	}
#endif
	case AF_INET: {
		if (addr->addrlen < (int)sizeof(struct sockaddr_in)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		const struct sockaddr_in *addr_in =
				(const struct sockaddr_in *)&addr->addr;
		if (port)
			*port = ntohs(addr_in->sin_port);
		return 0;
	}
	case AF_INET6: {
		if (addr->addrlen < (int)sizeof(struct sockaddr_in6)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		const struct sockaddr_in6 *addr_in6 =
				(const struct sockaddr_in6 *)&addr->addr;
		if (port)
			*port = ntohs(addr_in6->sin6_port);
		return 0;
	}
	default: set_errnum(ERRNUM_AFNOSUPPORT); return -1;
	}
}

int
io_addr_set_port(io_addr_t *addr, int port)
{
	assert(addr);

	if (addr->addrlen < (int)sizeof(sa_family_t)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	switch (((struct sockaddr *)&addr->addr)->sa_family) {
#if _WIN32
	case AF_BTH:
		if (addr->addrlen < (int)sizeof(SOCKADDR_BTH)) {
			WSASetLastError(WSAEINVAL);
			return -1;
		}
		((SOCKADDR_BTH *)&addr->addr)->port =
				port ? (ULONG)port : BT_PORT_ANY;
		;
		return 0;
#elif defined(__linux__) && defined(HAVE_BLUETOOTH_BLUETOOTH_H) \
		&& defined(HAVE_BLUETOOTH_RFCOMM_H)
	case AF_BLUETOOTH:
		if (addr->addrlen < (int)sizeof(struct sockaddr_rc)) {
			errno = EINVAL;
			return -1;
		}
		((struct sockaddr_rc *)&addr->addr)->rc_channel = htobs(port);
		return 0;
#endif
	case AF_INET:
		if (addr->addrlen < (int)sizeof(struct sockaddr_in)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		((struct sockaddr_in *)&addr->addr)->sin_port = htons(port);
		return 0;
	case AF_INET6:
		if (addr->addrlen < (int)sizeof(struct sockaddr_in6)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		((struct sockaddr_in6 *)&addr->addr)->sin6_port = htons(port);
		return 0;
	default: set_errnum(ERRNUM_AFNOSUPPORT); return -1;
	}
}

int
io_addr_is_loopback(const io_addr_t *addr)
{
	assert(addr);

	if (addr->addrlen < (int)sizeof(sa_family_t))
		return 0;

	switch (((const struct sockaddr *)&addr->addr)->sa_family) {
	case AF_INET: {
		if (addr->addrlen < (int)sizeof(struct sockaddr_in))
			return 0;
		const struct sockaddr_in *addr_in =
				(const struct sockaddr_in *)&addr->addr;
		return ntohl(addr_in->sin_addr.s_addr) == INADDR_LOOPBACK;
	}
	case AF_INET6: {
		if (addr->addrlen < (int)sizeof(struct sockaddr_in6))
			return 0;
		const struct sockaddr_in6 *addr_in6 =
				(const struct sockaddr_in6 *)&addr->addr;
		return !memcmp(&addr_in6->sin6_addr, &in6addr_any,
				sizeof(in6addr_any));
	}
	default: return 0;
	}
}

int
io_addr_is_broadcast(const io_addr_t *addr)
{
	assert(addr);

	if (addr->addrlen < (int)sizeof(sa_family_t))
		return 0;

	switch (((const struct sockaddr *)&addr->addr)->sa_family) {
	case AF_INET: {
		if (addr->addrlen < (int)sizeof(struct sockaddr_in))
			return 0;
		const struct sockaddr_in *addr_in =
				(const struct sockaddr_in *)&addr->addr;
		return ntohl(addr_in->sin_addr.s_addr) == INADDR_BROADCAST;
	}
	default: return 0;
	}
}

int
io_addr_is_multicast(const io_addr_t *addr)
{
	assert(addr);

	if (addr->addrlen < (int)sizeof(sa_family_t))
		return 0;

	switch (((const struct sockaddr *)&addr->addr)->sa_family) {
	case AF_INET: {
		if (addr->addrlen < (int)sizeof(struct sockaddr_in))
			return 0;
		const struct sockaddr_in *addr_in =
				(const struct sockaddr_in *)&addr->addr;
		return (ntohl(addr_in->sin_addr.s_addr) >> 28) == 0xe;
	}
	case AF_INET6: {
		if (addr->addrlen < (int)sizeof(struct sockaddr_in6))
			return 0;
		const struct sockaddr_in6 *addr_in6 =
				(const struct sockaddr_in6 *)&addr->addr;
		return addr_in6->sin6_addr.s6_addr[0] == 0xff;
	}
	default: return 0;
	}
}

int
io_get_addrinfo(int maxinfo, struct io_addrinfo *info, const char *nodename,
		const char *servname, const struct io_addrinfo *hints)
{
	if (!info)
		maxinfo = 0;

	int ecode = 0;

	struct addrinfo ai_hints = { .ai_family = AF_UNSPEC };
	if (hints) {
		switch (hints->domain) {
		case 0: break;
		case IO_SOCK_IPV4: ai_hints.ai_family = AF_INET; break;
		case IO_SOCK_IPV6: ai_hints.ai_family = AF_INET6; break;
		default: ecode = EAI_FAMILY; break;
		}

		switch (hints->type) {
		case 0: break;
		case IO_SOCK_STREAM: ai_hints.ai_socktype = SOCK_STREAM; break;
		case IO_SOCK_DGRAM: ai_hints.ai_socktype = SOCK_DGRAM; break;
		default: ecode = EAI_SOCKTYPE; break;
		}
	}

	struct addrinfo *res = NULL;
	if (!ecode)
		ecode = getaddrinfo(nodename, servname, &ai_hints, &res);
	if (ecode) {
		switch (ecode) {
		case EAI_AGAIN: set_errnum(ERRNUM_AI_AGAIN); break;
		case EAI_BADFLAGS: set_errnum(ERRNUM_AI_BADFLAGS); break;
		case EAI_FAIL: set_errnum(ERRNUM_AI_FAIL); break;
		case EAI_FAMILY: set_errnum(ERRNUM_AI_FAMILY); break;
		case EAI_MEMORY: set_errnum(ERRNUM_AI_MEMORY); break;
		case EAI_NONAME: set_errnum(ERRNUM_AI_NONAME); break;
		case EAI_SERVICE: set_errnum(ERRNUM_AI_SERVICE); break;
		case EAI_SOCKTYPE: set_errnum(ERRNUM_AI_SOCKTYPE); break;
#ifdef EAI_SYSTEM
		case EAI_SYSTEM: break;
#endif
		}
		return -1;
	}
	assert(res);

	int ninfo = 0;
	for (struct addrinfo *ai = res; ai; ai = ai->ai_next) {
		int domain;
		switch (ai->ai_family) {
		case AF_INET: domain = IO_SOCK_IPV4; break;
		case AF_INET6: domain = IO_SOCK_IPV6; break;
		default: continue;
		}

		int type;
		switch (ai->ai_socktype) {
		case SOCK_STREAM: type = IO_SOCK_STREAM; break;
		case SOCK_DGRAM: type = IO_SOCK_DGRAM; break;
		default: continue;
		}

		if (ninfo++ < maxinfo) {
			memset(info, 0, sizeof(*info));
			info->domain = domain;
			info->type = type;
			info->addr.addrlen = ai->ai_addrlen;
			memcpy(&info->addr.addr, ai->ai_addr, ai->ai_addrlen);
			info++;
		}
	}

	freeaddrinfo(res);

	return ninfo;
}

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

#if _WIN32

static int
ba2str(const BTH_ADDR *ba, char *str)
{
	return sprintf(str, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
			(int)((*ba >> 40) & 0xff), (int)((*ba >> 32) & 0xff),
			(int)((*ba >> 24) & 0xff), (int)((*ba >> 16) & 0xff),
			((int)(*ba >> 8) & 0xff), (int)(*ba & 0xff));
}

static int
str2ba(const char *str, BTH_ADDR *ba)
{
	if (bachk(str) < 0) {
		*ba = 0;
		return -1;
	}

	for (int i = 5; i >= 0; i--, str += 3)
		*ba |= (BTH_ADDR)strtol(str, NULL, 16) << (i * 8);

	return 0;
}

static int
bachk(const char *str)
{
	assert(str);

	if (strlen(str) != 17)
		return -1;

	while (*str) {
		if (!isxdigit(*str++))
			return -1;
		if (!isxdigit(*str++))
			return -1;
		if (!*str)
			break;
		if (*str++ != ':')
			return -1;
	}

	return 0;
}

#endif // _WIN32

#endif // !LELY_NO_STDIO
