/*!\file
 * This header file is part of the I/O library; it contains network address
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

#ifndef LELY_IO_ADDR_H
#define LELY_IO_ADDR_H

#include <lely/libc/stdint.h>
#include <lely/io/io.h>

//! An opaque network address type.
struct __io_addr {
	//! The size (in bytes) of #addr.
	int addrlen;
	//! The network address.
	union { char __size[128]; long __align; } addr;
};

//! The static initializer for #io_addr_t.
#define IO_ADDR_INIT	{ 0, { { 0 } } }

/*!
 * The maximum number of bytes required to hold the text representation of an
 * IPv4 internet address, including the terminating null byte.
 */
#define IO_ADDR_IPV4_STRLEN	16

/*!
 * The maximum number of bytes required to hold the text representation of an
 * IPv6 internet address, including the terminating null byte.
 */
#define IO_ADDR_IPV6_STRLEN	46

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Compares two network addresses.
 *
 * \returns an integer greater than, equal to, or less than 0 if the address at
 * \a p1 is greater than, equal to, or less than the address at \a p2.
 */
LELY_IO_EXTERN int __cdecl io_addr_cmp(const void *p1, const void *p2);

/*!
 * Initializes a network address from an IPv4 address and port number.
 *
 * \param addr a pointer to the network address to be initialized.
 * \param ip   the text representation of an IPv4 address. If \a ip is NULL or
 *             an empty string, the wildcard address (INADDR_ANY) is used.
 * \param port the port number.
 *
 * \see io_addr_get_ipv4_a()
 */
LELY_IO_EXTERN int io_addr_set_ipv4_a(io_addr_t *addr, const char *ip,
		int port);

/*!
 * Obtains an IPv4 address and port number from a network address.
 *
 * \param addr a pointer to a network address.
 * \param ip   the address of an array containing at least 4 four bytes (can be
 *             NULL). On success, if \a ip is not NULL, *\a ip contains the IPv4
 *             address in network byte order.
 * \param port the address of a port number (can be NULL). On success, if
 *             \a port is not NULL, *\a port contains the port number.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see io_addr_set_ipv4_n()
 */
LELY_IO_EXTERN int io_addr_get_ipv4_n(const io_addr_t *addr, uint8_t ip[4],
		int *port);

/*!
 * Initializes a network address from an IPv4 address and port number.
 *
 * \param addr a pointer to the network address to be initialized.
 * \param ip   a pointer to an IPv4 address in network byte order. If \a ip is
 *             NULL, the wildcard address (INADDR_ANY) is used.
 * \param port the port number.
 *
 * \see io_addr_get_ipv4_n()
 */
LELY_IO_EXTERN void io_addr_set_ipv4_n(io_addr_t *addr, const uint8_t ip[4],
		int port);

/*!
 * Initializes a network address with the IPv4 loopback address and a port
 * number.
 *
 * \param addr a pointer to the network address to be initialized.
 * \param port the port number.
 */
LELY_IO_EXTERN void io_addr_set_ipv4_loopback(io_addr_t *addr, int port);

/*!
 * Initializes a network address with the IPv4 broadcast address and a port
 * number.
 *
 * \param addr a pointer to the network address to be initialized.
 * \param port the port number.
 */
LELY_IO_EXTERN void io_addr_set_ipv4_broadcast(io_addr_t *addr, int port);

/*!
 * Obtains an IPv6 address and port number from a network address.
 *
 * \param addr a pointer to a network address.
 * \param ip   the address of a string containing at least #IO_ADDR_IPV6_STRLEN
 *             characters (can be NULL). On success, if \a ip is not NULL,
 *             *\a ip contains the text representation of the IPv6 address.
 * \param port the address of a port number (can be NULL). On success, if
 *             \a port is not NULL, *\a port contains the port number.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see io_addr_set_ipv6_a()
 */
LELY_IO_EXTERN int io_addr_get_ipv6_a(const io_addr_t *addr, char *ip,
		int *port);

/*!
 * Initializes a network address from an IPv6 address and port number.
 *
 * \param addr a pointer to the network address to be initialized.
 * \param ip   the text representation of an IPv6 address. If \a ip is NULL or
 *             an empty string, the wildcard address (in6addr_any) is used.
 * \param port the port number.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see io_addr_get_ipv6_a()
 */
LELY_IO_EXTERN int io_addr_set_ipv6_a(io_addr_t *addr, const char *ip,
		int port);

/*!
 * Obtains an IPv6 address and port number from a network address.
 *
 * \param addr a pointer to a network address.
 * \param ip   the address of an array containing at least 16 four bytes (can be
 *             NULL). On success, if \a ip is not NULL, *\a ip contains the IPv6
 *             address in network byte order.
 * \param port the address of a port number (can be NULL). On success, if
 *             \a port is not NULL, *\a port contains the port number.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see io_addr_set_ipv6_a()
 */
LELY_IO_EXTERN int io_addr_get_ipv6_n(const io_addr_t *addr, uint8_t ip[16],
		int *port);

/*!
 * Initializes a network address from an IPv6 address and port number.
 *
 * \param addr a pointer to the network address to be initialized.
 * \param ip   a pointer to an IPv6 address in network byte order. If \a ip is
 *             NULL, the wildcard address (in6addr_any) is used.
 * \param port the port number.
 *
 * \see io_addr_get_ipv6_a()
 */
LELY_IO_EXTERN void io_addr_set_ipv6_n(io_addr_t *addr, const uint8_t ip[16],
		int port);

/*!
 * Initializes a network address with the IPv6 loopback address and a port
 * number.
 *
 * \param addr a pointer to the network address to be initialized.
 * \param port the port number.
 */
LELY_IO_EXTERN void io_addr_set_ipv6_loopback(io_addr_t *addr, int port);

/*!
 * Obtains the port number of an IPv4 or IPv6 network address.
 *
 * \param addr a pointer to a network address.
 * \param port the address of a port number (can be NULL). On success, if
 *             \a port is not NULL, *\a port contains the port number.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see io_addr_set_port()
 */
LELY_IO_EXTERN int io_addr_get_port(const io_addr_t *addr, int *port);

/*!
 * Initializes the port number of an IPv4 or IPv6 network address.
 *
 * \param addr a pointer to the network address to be initialized.
 * \param port the port number.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see io_addr_set_port()
 */
LELY_IO_EXTERN int io_addr_set_port(io_addr_t *addr, int port);

//! Returns 1 if the network address is a loopback address, and 0 if not.
LELY_IO_EXTERN int io_addr_is_loopback(const io_addr_t *addr);

//! Returns 1 if the network address is a broadcast address, and 0 if not.
LELY_IO_EXTERN int io_addr_is_broadcast(const io_addr_t *addr);

//! Returns 1 if the network address is a multicast address, and 0 if not.
LELY_IO_EXTERN int io_addr_is_multicast(const io_addr_t *addr);

#ifdef __cplusplus
}
#endif

#endif

