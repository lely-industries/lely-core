/**@file
 * This header file is part of the I/O library; it contains the network address
 * declarations.
 *
 * @copyright 2016-2018 Lely Industries N.V.
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

#ifndef LELY_IO_ADDR_H_
#define LELY_IO_ADDR_H_

#include <lely/io/io.h>

#include <stdint.h>

/// An opaque network address type.
struct __io_addr {
	/// The size (in bytes) of #addr.
	int addrlen;
	/// The network address.
	union {
		char __size[128];
		long __align;
	} addr;
};

/// The static initializer for #io_addr_t.
#define IO_ADDR_INIT \
	{ \
		0, \
		{ \
			{ \
				0 \
			} \
		} \
	}

/**
 * The maximum number of bytes required to hold the text representation of a
 * Bluetooth device address, including the terminating null byte.
 */
#define IO_ADDR_BTH_STRLEN 18

/**
 * The maximum number of bytes required to hold the text representation of an
 * IPv4 internet address, including the terminating null byte.
 */
#define IO_ADDR_IPV4_STRLEN 16

/**
 * The maximum number of bytes required to hold the text representation of an
 * IPv6 internet address, including the terminating null byte.
 */
#define IO_ADDR_IPV6_STRLEN 46

/**
 * The maximum number of bytes required to hold the text representation of a
 * UNIX domain socket path name, including the terminating null byte.
 */
#define IO_ADDR_UNIX_STRLEN 108

/// A network address info structure.
struct io_addrinfo {
	/**
	 * The domain of the socket (only #IO_SOCK_IPV4 and #IO_SOCK_IPV6 are
	 * supported).
	 */
	int domain;
	/// The type of the socket (either #IO_SOCK_STREAM or #IO_SOCK_DGRAM).
	int type;
	/// The network address.
	io_addr_t addr;
};

/// The static initializer for struct #io_addrinfo.
#define IO_ADDRINFO_INIT \
	{ \
		0, 0, IO_ADDR_INIT \
	}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compares two network addresses.
 *
 * @returns an integer greater than, equal to, or less than 0 if the address at
 * <b>p1</b> is greater than, equal to, or less than the address at <b>p2</b>.
 */
int io_addr_cmp(const void *p1, const void *p2);

/**
 * Obtains an RFCOMM Bluetooth device address and port number from a network
 * address.
 *
 * @param addr a pointer to a network address.
 * @param ba   the address of a string containing at least
 *             #IO_ADDR_BTH_STRLEN characters (can be NULL). On success, if
 *             <b>ba</b> is not NULL, *<b>ba</b> contains the text
 *             representation of the Bluetooth address.
 * @param port the address of a port number (can be NULL). On success, if
 *             <b>port</b> is not NULL, *<b>port</b> contains the port number or
 *             channel.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_addr_set_rfcomm_a()
 */
int io_addr_get_rfcomm_a(const io_addr_t *addr, char *ba, int *port);

/**
 * Initializes a network address from an RFCOMM Bluetooth device address and
 * port number.
 *
 * @param addr a pointer to the network address to be initialized.
 * @param ba   the text representation of a Bluetooth address. If <b>ba</b> is
 *             NULL or an empty string, the wildcard address (00:00:00:00:00:00)
 *             is used.
 * @param port the port number or channel. If <b>port</b> is 0, io_socket_bind()
 *             will dynamically assign a port number.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_addr_get_rfcomm_a()
 */
int io_addr_set_rfcomm_a(io_addr_t *addr, const char *ba, int port);

/**
 * Obtains an RFCOMM Bluetooth device address and port number from a network
 * address.
 *
 * @param addr a pointer to a network address.
 * @param ba   the address of an array containing at least 6 four bytes (can be
 *             NULL). On success, if <b>ba</b> is not NULL, *<b>ba</b> contains
 *             the Bluetooth address in network byte order.
 * @param port the address of a port number (can be NULL). On success, if
 *             <b>port</b> is not NULL, *<b>port</b> contains the port number or
 *             channel.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_addr_set_rfcomm_n()
 */
int io_addr_get_rfcomm_n(const io_addr_t *addr, uint8_t ba[6], int *port);

/**
 * Initializes a network address from an RFCOMM Bluetooth device address and
 * port number.
 *
 * @param addr a pointer to the network address to be initialized.
 * @param ba   a pointer to a Bluetooth address in network byte order. If
 *             <b>ba</b> is, the wildcard address (00:00:00:00:00:00) is used.
 * @param port the port number or channel. If <b>port</b> is 0, io_socket_bind()
 *             will dynamically assign a port number.
 *
 * @see io_addr_get_rfcomm_n()
 */
void io_addr_set_rfcomm_n(io_addr_t *addr, const uint8_t ba[6], int port);

/**
 * Initializes a network address with the local Bluetooth (RFCOMM) device
 * address (FF:FF:FF:00:00:00) and a port number.
 *
 * @param addr a pointer to the network address to be initialized.
 * @param port the port number or PSM (Protocol Service Multiplexer). If
 *             <b>port</b> is 0, io_socket_bind() will dynamically assign a port
 *             number.
 */
void io_addr_set_rfcomm_local(io_addr_t *addr, int port);

/**
 * Obtains an IPv4 address and port number from a network address.
 *
 * @param addr a pointer to a network address.
 * @param ip   the address of a string containing at least #IO_ADDR_IPV4_STRLEN
 *             characters (can be NULL). On success, if <b>ip</b> is not NULL,
 *             *<b>ip</b> contains the text representation of the IPv4 address.
 * @param port the address of a port number (can be NULL). On success, if
 *             <b>port</b> is not NULL, *<b>port</b> contains the port number.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_addr_set_ipv4_a()
 */
int io_addr_get_ipv4_a(const io_addr_t *addr, char *ip, int *port);

/**
 * Initializes a network address from an IPv4 address and port number.
 *
 * @param addr a pointer to the network address to be initialized.
 * @param ip   the text representation of an IPv4 address. If <b>ip</b> is NULL
 *             or an empty string, the wildcard address (INADDR_ANY) is used.
 * @param port the port number.
 *
 * @see io_addr_get_ipv4_a()
 */
int io_addr_set_ipv4_a(io_addr_t *addr, const char *ip, int port);

/**
 * Obtains an IPv4 address and port number from a network address.
 *
 * @param addr a pointer to a network address.
 * @param ip   the address of an array containing at least 4 four bytes (can be
 *             NULL). On success, if <b>ip</b> is not NULL, *<b>ip</b> contains
 *             the IPv4 address in network byte order.
 * @param port the address of a port number (can be NULL). On success, if
 *             <b>port</b> is not NULL, *<b>port</b> contains the port number.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_addr_set_ipv4_n()
 */
int io_addr_get_ipv4_n(const io_addr_t *addr, uint8_t ip[4], int *port);

/**
 * Initializes a network address from an IPv4 address and port number.
 *
 * @param addr a pointer to the network address to be initialized.
 * @param ip   a pointer to an IPv4 address in network byte order. If <b>ip</b>
 *             is NULL, the wildcard address (INADDR_ANY) is used.
 * @param port the port number.
 *
 * @see io_addr_get_ipv4_n()
 */
void io_addr_set_ipv4_n(io_addr_t *addr, const uint8_t ip[4], int port);

/**
 * Initializes a network address with the IPv4 loopback address and a port
 * number.
 *
 * @param addr a pointer to the network address to be initialized.
 * @param port the port number.
 */
void io_addr_set_ipv4_loopback(io_addr_t *addr, int port);

/**
 * Initializes a network address with the IPv4 broadcast address and a port
 * number.
 *
 * @param addr a pointer to the network address to be initialized.
 * @param port the port number.
 */
void io_addr_set_ipv4_broadcast(io_addr_t *addr, int port);

/**
 * Obtains an IPv6 address and port number from a network address.
 *
 * @param addr a pointer to a network address.
 * @param ip   the address of a string containing at least #IO_ADDR_IPV6_STRLEN
 *             characters (can be NULL). On success, if <b>ip</b> is not NULL,
 *             *<b>ip</b> contains the text representation of the IPv6 address.
 * @param port the address of a port number (can be NULL). On success, if
 *             <b>port</b> is not NULL, *<b>port</b> contains the port number.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_addr_set_ipv6_a()
 */
int io_addr_get_ipv6_a(const io_addr_t *addr, char *ip, int *port);

/**
 * Initializes a network address from an IPv6 address and port number.
 *
 * @param addr a pointer to the network address to be initialized.
 * @param ip   the text representation of an IPv6 address. If <b>ip</b> is NULL
 *             or an empty string, the wildcard address (in6addr_any) is used.
 * @param port the port number.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_addr_get_ipv6_a()
 */
int io_addr_set_ipv6_a(io_addr_t *addr, const char *ip, int port);

/**
 * Obtains an IPv6 address and port number from a network address.
 *
 * @param addr a pointer to a network address.
 * @param ip   the address of an array containing at least 16 four bytes (can be
 *             NULL). On success, if <b>ip</b> is not NULL, *<b>ip</b> contains
 *             the IPv6 address in network byte order.
 * @param port the address of a port number (can be NULL). On success, if
 *             <b>port</b> is not NULL, *<b>port</b> contains the port number.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_addr_set_ipv6_a()
 */
int io_addr_get_ipv6_n(const io_addr_t *addr, uint8_t ip[16], int *port);

/**
 * Initializes a network address from an IPv6 address and port number.
 *
 * @param addr a pointer to the network address to be initialized.
 * @param ip   a pointer to an IPv6 address in network byte order. If <b>ip</b>
 *             is NULL, the wildcard address (in6addr_any) is used.
 * @param port the port number.
 *
 * @see io_addr_get_ipv6_a()
 */
void io_addr_set_ipv6_n(io_addr_t *addr, const uint8_t ip[16], int port);

/**
 * Initializes a network address with the IPv6 loopback address and a port
 * number.
 *
 * @param addr a pointer to the network address to be initialized.
 * @param port the port number.
 */
void io_addr_set_ipv6_loopback(io_addr_t *addr, int port);

/**
 * Obtains a UNIX domain socket path name from a network address.
 *
 * @param addr a pointer to a network address.
 * @param path the address of a string containing at least #IO_ADDR_UNIX_STRLEN
 *             characters (can be NULL). On success, if <b>path</b> is not NULL,
 *             *<b>path</b> contains the path name.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_addr_set_unix()
 */
int io_addr_get_unix(const io_addr_t *addr, char *path);

/**
 * Initializes a network address from a UNIX domain socket path name.
 *
 * @param addr a pointer to the network address to be initialized.
 * @param path a pointer to a path name.
 *
 * @see io_addr_get_unix()
 */
void io_addr_set_unix(io_addr_t *addr, const char *path);

/**
 * Obtains the domain of a network address.
 *
 * @returns #IO_SOCK_BTH, #IO_SOCK_IPV4, #IO_SOCK_IPV6 or #IO_SOCK_UNIX, or -1
 * on error. In the latter case, the error number can be obtained with
 * get_errc().
 *
 * @see io_sock_get_domain()
 */
int io_addr_get_domain(const io_addr_t *addr);

/**
 * Obtains the port number of an IPv4 or IPv6 network address.
 *
 * @param addr a pointer to a network address.
 * @param port the address of a port number (can be NULL). On success, if
 *             <b>port</b> is not NULL, *<b>port</b> contains the port number.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_addr_set_port()
 */
int io_addr_get_port(const io_addr_t *addr, int *port);

/**
 * Initializes the port number of an IPv4 or IPv6 network address.
 *
 * @param addr a pointer to the network address to be initialized.
 * @param port the port number.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_addr_set_port()
 */
int io_addr_set_port(io_addr_t *addr, int port);

/// Returns 1 if the network address is a loopback address, and 0 if not.
int io_addr_is_loopback(const io_addr_t *addr);

/// Returns 1 if the network address is a broadcast address, and 0 if not.
int io_addr_is_broadcast(const io_addr_t *addr);

/// Returns 1 if the network address is a multicast address, and 0 if not.
int io_addr_is_multicast(const io_addr_t *addr);

/**
 * Obtains a list of network addresses corresponding to a host and/or service
 * name.
 *
 * @param maxinfo  the maximum number of #io_addrinfo structs to return.
 * @param info     an array of at least <b>maxinfo</b> #io_addrinfo structs (can
 *                 be NULL). On success, *<b>info</b> contains at most
 *                 <b>maxinfo</b> structures describing the network addresses.
 * @param nodename a pointer to a string containing a host (node) name or
 *                 numeric address (can be NULL).
 * @param servname a pointer to a string containing a service name or port
 *                 number (can be NULL).
 * @param hints    a pointer to a network address structure containing hints
 *                 about the domain and type of sockets the caller supports. If
 *                 not NULL, only the <b>domain</b> and <b>type</b> fields of
 *                 *<b>hints</b> are taken into account, if they are non-zero.
 *
 * @returns the total number of addresses (which may be different from
 * <b>maxinfo</b>), or -1 on error. In the latter case, the error number can be
 * obtained with get_errc().
 */
int io_get_addrinfo(int maxinfo, struct io_addrinfo *info, const char *nodename,
		const char *servname, const struct io_addrinfo *hints);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_ADDR_H_
