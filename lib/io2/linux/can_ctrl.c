/**@file
 * This file is part of the I/O library; it contains the CAN controller
 * mplementation for Linux.
 *
 * @see lely/io2/linux/can.h
 *
 * @copyright 2015-2021 Lely Industries N.V.
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

#if !LELY_NO_STDIO && defined(__linux__)

#include <lely/io2/linux/can.h>
#include <lely/util/util.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <net/if.h>

#include "can_attr.h"
#define LELY_IO_IFREQ_DOMAIN AF_CAN
#define LELY_IO_IFREQ_TYPE SOCK_RAW
#define LELY_IO_IFREQ_PROTOCOL CAN_RAW
#include "if.h"
#include "ifreq.h"

#ifndef LELY_IO_CAN_TXLEN
/// The default SocketCAN transmit queue length (in number of CAN frames).
#define LELY_IO_CAN_TXLEN 128
#endif

static int io_can_ctrl_impl_stop(io_can_ctrl_t *ctrl);
static int io_can_ctrl_impl_stopped(const io_can_ctrl_t *ctrl);
static int io_can_ctrl_impl_restart(io_can_ctrl_t *ctrl);
static int io_can_ctrl_impl_get_bitrate(
		const io_can_ctrl_t *ctrl, int *pnominal, int *pdata);
static int io_can_ctrl_impl_set_bitrate(
		io_can_ctrl_t *ctrl, int nominal, int data);
static int io_can_ctrl_impl_get_state(const io_can_ctrl_t *ctrl);

// clang-format off
static const struct io_can_ctrl_vtbl io_can_ctrl_impl_vtbl = {
	&io_can_ctrl_impl_stop,
	&io_can_ctrl_impl_stopped,
	&io_can_ctrl_impl_restart,
	&io_can_ctrl_impl_get_bitrate,
	&io_can_ctrl_impl_set_bitrate,
	&io_can_ctrl_impl_get_state
};
// clang-format on

struct io_can_ctrl_impl {
	const struct io_can_ctrl_vtbl *ctrl_vptr;
	unsigned int index;
	char name[IF_NAMESIZE];
	int flags;
};

static inline struct io_can_ctrl_impl *io_can_ctrl_impl_from_ctrl(
		const io_can_ctrl_t *ctrl);

void *
io_can_ctrl_alloc(void)
{
	struct io_can_ctrl_impl *impl = malloc(sizeof(*impl));
	if (!impl)
		return NULL;
	// Suppress a GCC maybe-uninitialized warning.
	impl->ctrl_vptr = NULL;
	// cppcheck-suppress memleak symbolName=impl
	return &impl->ctrl_vptr;
}

void
io_can_ctrl_free(void *ptr)
{
	if (ptr)
		free(io_can_ctrl_impl_from_ctrl(ptr));
}

io_can_ctrl_t *
io_can_ctrl_init(io_can_ctrl_t *ctrl, unsigned int index, size_t txlen)
{
	struct io_can_ctrl_impl *impl = io_can_ctrl_impl_from_ctrl(ctrl);

	if (!txlen)
		txlen = LELY_IO_CAN_TXLEN;

	impl->ctrl_vptr = &io_can_ctrl_impl_vtbl;

	impl->index = index;

	memset(impl->name, 0, IF_NAMESIZE);
	if (!if_indextoname(impl->index, impl->name))
		return NULL;

	struct io_can_attr attr = IO_CAN_ATTR_INIT;
	// Some CAN network interfaces, such as the serial line CAN interface
	// provided by the SLCAN driver, do not provide the CAN bus attributes.
	// This is not an error.
	int errsv = errno;
	if (io_can_attr_get(&attr, impl->index) == -1 && errno != ENOTSUP)
		return NULL;
	errno = errsv;

	impl->flags = attr.flags;

	if (io_if_set_txqlen(ARPHRD_CAN, impl->index, txlen) == -1)
		return NULL;

	return ctrl;
}

void
io_can_ctrl_fini(io_can_ctrl_t *ctrl)
{
	(void)ctrl;
}

io_can_ctrl_t *
io_can_ctrl_create_from_name(const char *name, size_t txlen)
{
	unsigned int index = if_nametoindex(name);
	if (!index)
		return 0;
	return io_can_ctrl_create_from_index(index, txlen);
}

io_can_ctrl_t *
io_can_ctrl_create_from_index(unsigned int index, size_t txlen)
{
	int errsv = 0;

	io_can_ctrl_t *ctrl = io_can_ctrl_alloc();
	if (!ctrl) {
		errsv = errno;
		goto error_alloc;
	}

	io_can_ctrl_t *tmp = io_can_ctrl_init(ctrl, index, txlen);
	if (!tmp) {
		errsv = errno;
		goto error_init;
	}
	ctrl = tmp;

	return ctrl;

error_init:
	io_can_ctrl_free((void *)ctrl);
error_alloc:
	errno = errsv;
	return NULL;
}

void
io_can_ctrl_destroy(io_can_ctrl_t *ctrl)
{
	if (ctrl) {
		io_can_ctrl_fini(ctrl);
		io_can_ctrl_free((void *)ctrl);
	}
}

const char *
io_can_ctrl_get_name(const io_can_ctrl_t *ctrl)
{
	const struct io_can_ctrl_impl *impl = io_can_ctrl_impl_from_ctrl(ctrl);

	return impl->name;
}

unsigned int
io_can_ctrl_get_index(const io_can_ctrl_t *ctrl)
{
	const struct io_can_ctrl_impl *impl = io_can_ctrl_impl_from_ctrl(ctrl);

	return impl->index;
}

int
io_can_ctrl_get_flags(const io_can_ctrl_t *ctrl)
{
	const struct io_can_ctrl_impl *impl = io_can_ctrl_impl_from_ctrl(ctrl);

	return impl->flags;
}

static int
io_can_ctrl_impl_stop(io_can_ctrl_t *ctrl)
{
	struct io_can_ctrl_impl *impl = io_can_ctrl_impl_from_ctrl(ctrl);

	int flags = 0;
	return ifr_set_flags(impl->name, &flags, IFF_UP);
}

static int
io_can_ctrl_impl_stopped(const io_can_ctrl_t *ctrl)
{
	const struct io_can_ctrl_impl *impl = io_can_ctrl_impl_from_ctrl(ctrl);

	int flags = ifr_get_flags(impl->name);
	return flags == -1 ? -1 : !(flags & IFF_UP);
}

static int
io_can_ctrl_impl_restart(io_can_ctrl_t *ctrl)
{
	struct io_can_ctrl_impl *impl = io_can_ctrl_impl_from_ctrl(ctrl);

	int flags = IFF_UP;
	return ifr_set_flags(impl->name, &flags, IFF_UP);
}

static int
io_can_ctrl_impl_get_bitrate(
		const io_can_ctrl_t *ctrl, int *pnominal, int *pdata)
{
	const struct io_can_ctrl_impl *impl = io_can_ctrl_impl_from_ctrl(ctrl);

	struct io_can_attr attr = IO_CAN_ATTR_INIT;
	if (io_can_attr_get(&attr, impl->index) == -1)
		return -1;

	if (pnominal)
		*pnominal = attr.nominal;

	if (pdata)
#if LELY_NO_CANFD
		*pdata = 0;
#else
		*pdata = attr.data;
#endif

	return 0;
}

static int
io_can_ctrl_impl_set_bitrate(io_can_ctrl_t *ctrl, int nominal, int data)
{
	struct io_can_ctrl_impl *impl = io_can_ctrl_impl_from_ctrl(ctrl);

	char buf[128];
	struct rtattr *linkinfo = (struct rtattr *)buf;
	*linkinfo = (struct rtattr){ .rta_len = RTA_LENGTH(0),
		.rta_type = IFLA_LINKINFO };

	struct rtattr *info_kind = RTA_TAIL(linkinfo);
	*info_kind = (struct rtattr){ .rta_len = RTA_LENGTH(strlen("can")),
		.rta_type = IFLA_INFO_KIND };
	strcpy(RTA_DATA(info_kind), "can");

	linkinfo->rta_len += RTA_ALIGN(info_kind->rta_len);

	struct rtattr *info_data = RTA_TAIL(info_kind);
	*info_data = (struct rtattr){ .rta_len = RTA_LENGTH(0),
		.rta_type = IFLA_INFO_DATA };
	struct rtattr *rta = RTA_DATA(info_data);

	*rta = (struct rtattr){ .rta_len = RTA_LENGTH(
						sizeof(struct can_bittiming)),
		.rta_type = IFLA_CAN_BITTIMING };
	*(struct can_bittiming *)RTA_DATA(rta) =
			(struct can_bittiming){ .bitrate = nominal };
	info_data->rta_len += RTA_ALIGN(rta->rta_len);
	rta = RTA_TAIL(rta);

#if LELY_NO_CANFD
	(void)data;
#else
	if (impl->flags & IO_CAN_BUS_FLAG_BRS) {
		*rta = (struct rtattr){ .rta_len = RTA_LENGTH(sizeof(
							struct can_bittiming)),
			.rta_type = IFLA_CAN_DATA_BITTIMING };
		*(struct can_bittiming *)RTA_DATA(rta) =
				(struct can_bittiming){ .bitrate = data };
		info_data->rta_len += RTA_ALIGN(rta->rta_len);
		// rta = RTA_TAIL(rta); // Not needed if rta is not used below.
	}
#endif

	linkinfo->rta_len += RTA_ALIGN(info_data->rta_len);

	int errsv = 0;

	int flags = 0;
	if (ifr_set_flags(impl->name, &flags, IFF_UP) == -1) {
		errsv = errno;
		goto error_ifr_set_flags;
	}

	struct rtnl_handle rth = { -1, 0, 0 };
	if (rtnl_open(&rth) == -1) {
		errsv = errno;
		goto error_rtnl_open;
	}

	// clang-format off
	if (rtnl_send_newlink_request(&rth, AF_UNSPEC, ARPHRD_CAN, impl->index,
			flags, linkinfo, RTA_ALIGN(linkinfo->rta_len)) == -1) {
		// clang-format on
		errsv = errno;
		goto error_rtnl_send_newlink_request;
	}

	if (rtnl_recv_ack(&rth) == -1) {
		errsv = errno;
		goto error_rtnl_recv_ack;
	}

	rtnl_close(&rth);
	return 0;

error_rtnl_recv_ack:
error_rtnl_send_newlink_request:
	rtnl_close(&rth);
error_rtnl_open:
error_ifr_set_flags:
	errno = errsv;
	return -1;
}

static int
io_can_ctrl_impl_get_state(const io_can_ctrl_t *ctrl)
{
	const struct io_can_ctrl_impl *impl = io_can_ctrl_impl_from_ctrl(ctrl);

	int flags = ifr_get_flags(impl->name);
	if (flags == -1)
		return -1;
	if (!(flags & IFF_UP))
		return CAN_STATE_STOPPED;

	struct io_can_attr attr = IO_CAN_ATTR_INIT;
	if (io_can_attr_get(&attr, impl->index) == -1)
		return -1;

	return attr.state;
}

static inline struct io_can_ctrl_impl *
io_can_ctrl_impl_from_ctrl(const io_can_ctrl_t *ctrl)
{
	assert(ctrl);

	return structof(ctrl, struct io_can_ctrl_impl, ctrl_vptr);
}

#endif // !LELY_NO_STDIO && __linux__
