/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * network interface functions.
 *
 * @see lely/io/if.h
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

#include <lely/io/if.h>
#include <lely/io/sock.h>
#include <lely/util/errnum.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#if _WIN32
#ifdef _MSC_VER
#pragma comment(lib, "iphlpapi.lib")
#endif
// clang-format off
#include <wincrypt.h>
#include <iphlpapi.h>
// clang-format on
#elif defined(__linux__) && defined(HAVE_IFADDRS_H)
#include <ifaddrs.h>
#endif

#if _WIN32 || (defined(__linux__) && defined(HAVE_IFADDRS_H))
static void io_addr_set(io_addr_t *addr, const struct sockaddr *address);
#endif

#if _WIN32
static NETIO_STATUS WINAPI ConvertLengthToIpv6Mask(
		ULONG MaskLength, u_char Mask[16]);
#endif

#if _WIN32 || (defined(__linux__) && defined(HAVE_IFADDRS_H))

int
io_get_ifinfo(int maxinfo, struct io_ifinfo *info)
{
	if (!info)
		maxinfo = 0;

#if _WIN32
	ULONG Flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST
			| GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_PREFIX
			| GAA_FLAG_SKIP_FRIENDLY_NAME
			| GAA_FLAG_INCLUDE_ALL_INTERFACES;
	DWORD Size = 0;
	DWORD dwErrCode = GetAdaptersAddresses(
			AF_UNSPEC, Flags, NULL, NULL, &Size);
	if (dwErrCode != ERROR_BUFFER_OVERFLOW) {
		SetLastError(dwErrCode);
		return -1;
	}

	PIP_ADAPTER_ADDRESSES pAdapterAddresses = malloc(Size);
	if (!pAdapterAddresses)
		return -1;

	dwErrCode = GetAdaptersAddresses(
			AF_UNSPEC, Flags, NULL, pAdapterAddresses, &Size);
	if (dwErrCode != ERROR_SUCCESS) {
		free(pAdapterAddresses);
		SetLastError(dwErrCode);
		return -1;
	}

	int ninfo = 0;
	for (PIP_ADAPTER_ADDRESSES paa = pAdapterAddresses; paa;
			paa = paa->Next) {
		// Skip interfaces with invalid indices.
		unsigned int index =
				paa->IfIndex ? paa->IfIndex : paa->Ipv6IfIndex;
		if (!index)
			continue;

		// Copy the status.
		int flags = 0;
		if (paa->OperStatus == IfOperStatusUp)
			flags |= IO_IF_UP;
		if (paa->IfType == IF_TYPE_PPP)
			flags |= IO_IF_POINTTOPOINT;
		else if (paa->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
			flags |= IO_IF_LOOPBACK;
		else
			flags |= IO_IF_BROADCAST;
		if (!(paa->Flags & IP_ADAPTER_NO_MULTICAST))
			flags |= IO_IF_MULTICAST;

		// Every unicast address represents a network interface.
		for (PIP_ADAPTER_UNICAST_ADDRESS paua =
						paa->FirstUnicastAddress;
				paua; paua = paua->Next) {
			LPSOCKADDR lpSockaddr = paua->Address.lpSockaddr;
			if (!lpSockaddr)
				continue;

			// We only support IPv4 and IPv6.
			int domain;
			switch (lpSockaddr->sa_family) {
			case AF_INET: domain = IO_SOCK_IPV4; break;
			case AF_INET6: domain = IO_SOCK_IPV6; break;
			default: continue;
			}

			if (++ninfo > maxinfo)
				continue;

			memset(info, 0, sizeof(*info));
			info->addr.addrlen = 0;
			info->netmask.addrlen = 0;
			info->broadaddr.addrlen = 0;

			// Copy the index and obtain the interface name.
			info->index = index;
			if_indextoname(info->index, info->name);

			info->domain = domain;

			info->flags = flags;

			// Copy the interface address.
			io_addr_set(&info->addr, lpSockaddr);

			if (domain == IO_SOCK_IPV4) {
				// Construct the netmask from the prefix length.
				ULONG Mask;
				ConvertLengthToIpv4Mask(
						paua->OnLinkPrefixLength,
						&Mask);
				io_addr_set_ipv4_n(&info->netmask,
						(uint8_t *)&Mask, 0);

				if (info->flags & IO_IF_BROADCAST) {
					// Obtain the broadcast address from the
					// interface address and the netmask.
					ULONG BCast = ((struct sockaddr_in *)lpSockaddr)
									->sin_addr
									.s_addr
							| ~Mask;
					io_addr_set_ipv4_n(&info->broadaddr,
							(uint8_t *)&BCast, 0);
				}
			} else if (domain == IO_SOCK_IPV6) {
				// Construct the netmask from the prefix length.
				u_char Mask[16];
				ConvertLengthToIpv6Mask(
						paua->OnLinkPrefixLength, Mask);
				io_addr_set_ipv6_n(&info->netmask, Mask, 0);

				// IPv6 does not support broadcast.
				info->flags &= ~IO_IF_BROADCAST;
			}

			info++;
		}
	}

	free(pAdapterAddresses);
#else
	struct ifaddrs *res = NULL;
	if (getifaddrs(&res) == -1)
		return -1;

	int ninfo = 0;
	for (struct ifaddrs *ifa = res; ifa; ifa = ifa->ifa_next) {
		// Obtain the domain from the interface address.
		int domain = 0;
		if (ifa->ifa_addr) {
			switch (ifa->ifa_addr->sa_family) {
			case AF_INET: domain = IO_SOCK_IPV4; break;
			case AF_INET6: domain = IO_SOCK_IPV4; break;
			}
		}
		// Skip network interfaces with unknown domains.
		if (!domain)
			continue;

		if (++ninfo > maxinfo)
			continue;

		memset(info, 0, sizeof(*info));
		info->addr.addrlen = 0;
		info->netmask.addrlen = 0;
		info->broadaddr.addrlen = 0;

		// Obtain the interface index and copy the name.
		info->index = if_nametoindex(ifa->ifa_name);
		strncpy(info->name, ifa->ifa_name, IO_IF_NAME_STRLEN - 1);

		info->domain = domain;

		// Copy the status.
		info->flags = 0;
		if (ifa->ifa_flags & IFF_UP)
			info->flags |= IO_IF_UP;
		if (ifa->ifa_flags & IFF_BROADCAST)
			info->flags |= IO_IF_BROADCAST;
		if (ifa->ifa_flags & IFF_LOOPBACK)
			info->flags |= IO_IF_LOOPBACK;
		if (ifa->ifa_flags & IFF_POINTOPOINT)
			info->flags |= IO_IF_POINTTOPOINT;
		if (ifa->ifa_flags & IFF_MULTICAST)
			info->flags |= IO_IF_MULTICAST;

		// Copy the interface address.
		if (ifa->ifa_addr)
			io_addr_set(&info->addr, ifa->ifa_addr);

		// Copy the netmask.
		if (ifa->ifa_netmask)
			io_addr_set(&info->netmask, ifa->ifa_netmask);

		// Copy the broadcast or point-to-point destination address.
		if ((ifa->ifa_flags & IFF_BROADCAST) && ifa->ifa_broadaddr)
			io_addr_set(&info->broadaddr, ifa->ifa_broadaddr);
		else if ((ifa->ifa_flags & IFF_POINTOPOINT) && ifa->ifa_dstaddr)
			io_addr_set(&info->broadaddr, ifa->ifa_dstaddr);

		info++;
	}

	freeifaddrs(res);
#endif

	return ninfo;
}

static void
io_addr_set(io_addr_t *addr, const struct sockaddr *address)
{
	assert(addr);
	assert(address);

	switch (address->sa_family) {
#if defined(__linux__) && defined(HAVE_LINUX_CAN_H)
	case AF_CAN: addr->addrlen = sizeof(struct sockaddr_can); break;
#endif
	case AF_INET: addr->addrlen = sizeof(struct sockaddr_in); break;
	case AF_INET6: addr->addrlen = sizeof(struct sockaddr_in6); break;
#if _POSIX_C_SOURCE >= 200112L
	case AF_UNIX: addr->addrlen = sizeof(struct sockaddr_un); break;
#endif
	default: addr->addrlen = 0; break;
	}
	memcpy(&addr->addr, address, addr->addrlen);
}

#endif // _WIN32 || (__linux__ && HAVE_IFADDRS_H)

#if _WIN32
static NETIO_STATUS WINAPI
ConvertLengthToIpv6Mask(ULONG MaskLength, u_char Mask[16])
{
	if (MaskLength > 128) {
		for (int i = 0; i < 16; i++)
			Mask[i] = 0;
		return ERROR_INVALID_PARAMETER;
	}

	for (LONG i = MaskLength, j = 0; i > 0; i -= 8, j++)
		Mask[j] = i >= 8 ? 0xff : ((0xff << (8 - i)) & 0xff);
	return NO_ERROR;
}
#endif

#endif // !LELY_NO_STDIO
