/**@file
 * This is the internal header file of the ioctl network device configuration
 * functions.
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#ifndef LELY_IO2_INTERN_LINUX_IOCTL_H_
#define LELY_IO2_INTERN_LINUX_IOCTL_H_

#include "io.h"

#ifdef __linux__

#include <assert.h>
#include <string.h>

#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sys/ioctl.h>

#ifndef LELY_IO_IFREQ_DOMAIN
#define LELY_IO_IFREQ_DOMAIN AF_UNIX
#endif

#ifndef LELY_IO_IFREQ_TYPE
#define LELY_IO_IFREQ_TYPE SOCK_DGRAM
#endif

#ifndef LELY_IO_IFREQ_PROTOCOL
#define LELY_IO_IFREQ_PROTOCOL 0
#endif

struct ifr_handle {
	int fd;
	struct ifreq ifr;
};

static int ifr_open(struct ifr_handle *ifh, const char *name);
static int ifr_close(struct ifr_handle *ifh);

static int ifr_get_flags(const char *name);
static int ifr_set_flags(const char *name, int *flags, int mask);

static inline int
ifr_open(struct ifr_handle *ifh, const char *name)
{
	assert(ifh);
	assert(name);

	ifh->fd = socket(LELY_IO_IFREQ_DOMAIN,
			LELY_IO_IFREQ_TYPE | SOCK_CLOEXEC,
			LELY_IO_IFREQ_PROTOCOL);
	if (ifh->fd == -1)
		return -1;

	memset(&ifh->ifr, 0, sizeof(ifh->ifr));
	if (!memccpy(ifh->ifr.ifr_name, name, '\0', IFNAMSIZ))
		ifh->ifr.ifr_name[IFNAMSIZ - 1] = '\0';

	return 0;
}

static inline int
ifr_close(struct ifr_handle *ifh)
{
	assert(ifh);

	int fd = ifh->fd;
	ifh->fd = -1;
	return close(fd);
}

static inline int
ifr_get_flags(const char *name)
{
	struct ifr_handle ifh;
	if (ifr_open(&ifh, name) == -1)
		return -1;

	int result = ioctl(ifh.fd, SIOCGIFFLAGS, &ifh.ifr) == -1
			? -1
			: ifh.ifr.ifr_flags;
	ifr_close(&ifh);
	return result;
}

static inline int
ifr_set_flags(const char *name, int *flags, int mask)
{
	assert(flags);

	struct ifr_handle ifh;
	if (ifr_open(&ifh, name) == -1)
		return -1;

	int result = ioctl(ifh.fd, SIOCGIFFLAGS, &ifh.ifr);
	if (!result) {
		if ((ifh.ifr.ifr_flags ^ *flags) & mask) {
			ifh.ifr.ifr_flags &= ~mask;
			ifh.ifr.ifr_flags |= *flags & mask;
			result = ioctl(ifh.fd, SIOCSIFFLAGS, &ifh.ifr);
		}
		if (!result)
			*flags = ifh.ifr.ifr_flags;
	}
	ifr_close(&ifh);
	return result;
}

#endif // __linux__

#endif // !LELY_IO2_INTERN_LINUX_IOCTL_H_
