/**@file
 * This file contains the CANopen cat tool.
 *
 * @copyright 2017-2020 Lely Industries N.V.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lely/can/net.h>
#include <lely/co/pdo.h>
#include <lely/io/can.h>
#include <lely/io/poll.h>
#include <lely/libc/threads.h>
#include <lely/libc/unistd.h>
#include <lely/util/diag.h>
#include <lely/util/time.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
#define HELP \
	"Arguments: [options...] [<CAN interface> [StdIn PDO COB-ID] [StdOut PDO COB-ID]\n" \
	"           [StdErr PDO COB-ID]\n" \
	"Options:\n" \
	"  -h, --help            Display this information\n" \
	"  -i <ms>, --inhibit=<ms>\n" \
	"                        Wait at least <ms> milliseconds between PDOs\n" \
	"                        (default: 1)"
// clang-format on

#define FLAG_HELP 0x01

#define INHIBIT 1

#define POLL_TIMEOUT 100

int can_recv(const struct can_msg *msg, void *data);
int can_send(const struct can_msg *msg, void *data);

int io_thrd_start(void *arg);

can_net_t *net;

int
main(int argc, char *argv[])
{
	argv[0] = (char *)cmdname(argv[0]);
	diag_set_handler(&cmd_diag_handler, argv[0]);

	int flags = 0;
	int inhibit = INHIBIT;
	const char *ifname = 0;
	uint32_t cobid_in = CO_PDO_COBID_VALID;
	uint32_t cobid_out = CO_PDO_COBID_VALID;
	uint32_t cobid_err = CO_PDO_COBID_VALID;

	opterr = 0;
	optind = 1;
	int optpos = 0;
	while (optind < argc) {
		char *arg = argv[optind];
		if (*arg != '-') {
			optind++;
			switch (optpos++) {
			case 0: ifname = arg; break;
			case 1: cobid_in = strtoul(arg, NULL, 0); break;
			case 2: cobid_out = strtoul(arg, NULL, 0); break;
			case 3: cobid_err = strtoul(arg, NULL, 0); break;
			default:
				diag(DIAG_ERROR, 0, "extra argument %s", arg);
				break;
			}
		} else if (*++arg == '-') {
			optind++;
			if (!*++arg)
				break;
			if (!strcmp(arg, "help")) {
				flags |= FLAG_HELP;
			} else if (!strncmp(arg, "inhibit=", 8)) {
				inhibit = atoi(arg + 8);
			} else {
				diag(DIAG_ERROR, 0, "illegal option -- %s",
						arg);
			}
		} else {
			int c = getopt(argc, argv, ":hi:");
			if (c == -1)
				break;
			switch (c) {
			case ':':
			case '?': break;
			case 'h': flags |= FLAG_HELP; break;
			case 'i': inhibit = atoi(optarg); break;
			}
		}
	}
	for (char *arg = argv[optind]; optind < argc; arg = argv[++optind]) {
		switch (optpos++) {
		case 0: ifname = arg; break;
		case 1: cobid_in = strtoul(arg, NULL, 0); break;
		case 2: cobid_out = strtoul(arg, NULL, 0); break;
		case 3: cobid_err = strtoul(arg, NULL, 0); break;
		default: diag(DIAG_ERROR, 0, "extra argument %s", arg); break;
		}
	}

	if (flags & FLAG_HELP) {
		diag(DIAG_INFO, 0, "%s", HELP);
		return EXIT_SUCCESS;
	}

	if (optpos < 1 || !ifname) {
		diag(DIAG_ERROR, 0, "no CAN interface specified");
		goto error_arg;
	}

	if (lely_io_init() == -1) {
		diag(DIAG_ERROR, get_errc(),
				"unable to initialize I/O library");
		goto error_io_init;
	}

	if (!freopen(NULL, "rb", stdin)) {
		diag(DIAG_ERROR, get_errc(), "unable to reopen stdin");
		goto error_freopen;
	}

	if (!freopen(NULL, "wb", stdout)) {
		diag(DIAG_ERROR, get_errc(), "unable to reopen stdout");
		goto error_freopen;
	}

	if (!freopen(NULL, "wb", stderr)) {
		diag(DIAG_ERROR, get_errc(), "unable to reopen stderr");
		goto error_freopen;
	}

	io_poll_t *poll = io_poll_create();
	if (!poll) {
		diag(DIAG_ERROR, get_errc(),
				"unable to create I/O polling interface");
		goto error_create_poll;
	}

	io_handle_t hcan = io_open_can(ifname);
	if (hcan == IO_HANDLE_ERROR) {
		diag(DIAG_ERROR, get_errc(), "%s is not a suitable CAN device",
				ifname);
		goto error_open_can;
	}

	if (io_set_flags(hcan, IO_FLAG_NONBLOCK) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to configure %s", ifname);
		goto error_set_flags_can;
	}

	struct io_event event = IO_EVENT_INIT;
	event.events = IO_EVENT_READ;
	event.u.handle = hcan;
	if (io_poll_watch(poll, hcan, &event, 1) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to watch %s", ifname);
		goto error_watch_can;
	}

	net = can_net_create();
	if (!net) {
		diag(DIAG_ERROR, get_errc(), "unable to create CAN network");
		goto error_create_net;
	}
	can_net_set_send_func(net, &can_send, hcan);

	struct timespec now = { 0, 0 };
	timespec_get(&now, TIME_UTC);
	struct timespec next = now;

	can_recv_t *recv_out = NULL;
	if (!(cobid_out & CO_PDO_COBID_VALID)) {
		recv_out = can_recv_create();
		if (!recv_out) {
			diag(DIAG_ERROR, get_errc(),
					"unable to create CAN frame receiver");
			goto error_create_recv_out;
		}
		can_recv_set_func(recv_out, &can_recv, stdout);

		uint32_t id = cobid_out;
		uint8_t flags = 0;
		if (id & CO_PDO_COBID_FRAME) {
			id &= CAN_MASK_EID;
			flags |= CAN_FLAG_IDE;
		} else {
			id &= CAN_MASK_BID;
		}
		can_recv_start(recv_out, net, id, flags);
	}

	can_recv_t *recv_err = NULL;
	if (!(cobid_err & CO_PDO_COBID_VALID)) {
		recv_err = can_recv_create();
		if (!recv_err) {
			diag(DIAG_ERROR, get_errc(),
					"unable to create CAN frame receiver");
			goto error_create_recv_err;
		}
		can_recv_set_func(recv_err, &can_recv, stderr);

		uint32_t id = cobid_err;
		uint8_t flags = 0;
		if (id & CO_PDO_COBID_FRAME) {
			id &= CAN_MASK_EID;
			flags |= CAN_FLAG_IDE;
		} else {
			id &= CAN_MASK_BID;
		}
		can_recv_start(recv_err, net, id, flags);
	}

	thrd_t thr;
	if (thrd_create(&thr, &io_thrd_start, poll) != thrd_success) {
		diag(DIAG_ERROR, 0, "unable to create thread");
		goto error_create_thr;
	}

	int c;
	while ((c = fgetc(stdin)) != EOF) {
		if (cobid_in & CO_PDO_COBID_VALID)
			continue;

		// Wait until the inhibit time has passed.
		if (inhibit > 0) {
			struct timespec req = next;
			timespec_sub(&req, &now);
			nanosleep(&req, NULL);
		}

		struct can_msg msg = CAN_MSG_INIT;
		msg.id = cobid_in;
		if (msg.id & CO_PDO_COBID_FRAME) {
			msg.id &= CAN_MASK_EID;
			msg.flags |= CAN_FLAG_IDE;
		} else {
			msg.id &= CAN_MASK_BID;
		}
		msg.len = 1;
		msg.data[0] = (unsigned char)c;
		can_net_send(net, &msg);

		if (inhibit > 0) {
			timespec_get(&now, TIME_UTC);
			next = now;
			timespec_add_msec(&next, inhibit);
		}

		fflush(stderr);
		fflush(stdout);
	}

	io_poll_signal(poll, 1);
	thrd_join(thr, NULL);

	can_recv_destroy(recv_err);
	can_recv_destroy(recv_out);

	can_net_destroy(net);
	io_close(hcan);

	io_poll_destroy(poll);

	lely_io_fini();

	return EXIT_SUCCESS;

error_create_thr:
	can_recv_destroy(recv_err);
error_create_recv_err:
	can_recv_destroy(recv_out);
error_create_recv_out:
	can_net_destroy(net);
error_create_net:
error_watch_can:
error_set_flags_can:
	io_close(hcan);
error_open_can:
	io_poll_destroy(poll);
error_create_poll:
error_freopen:
	lely_io_fini();
error_io_init:
error_arg:
	return EXIT_FAILURE;
}

int
can_recv(const struct can_msg *msg, void *data)
{
	assert(msg);
	FILE *stream = data;
	assert(stream);

	// Ignore remote frames.
	if (msg->flags & CAN_FLAG_RTR)
		return 0;

#if !LELY_NO_CANFD
	// Ignore CAN FD format frames.
	if (msg->flags & CAN_FLAG_EDL)
		return 0;
#endif

	// Only accept single-byte PDOs.
	if (msg->len != 1)
		return 0;

	return fputc(msg->data[0], stream) != EOF ? 0 : -1;
}

int
can_send(const struct can_msg *msg, void *data)
{
	io_handle_t handle = (io_handle_t)data;

	return io_can_write(handle, msg) == 1 ? 0 : -1;
}

int
io_thrd_start(void *arg)
{
	io_poll_t *poll = arg;
	assert(poll);

	for (;;) {
		struct io_event event = IO_EVENT_INIT;
		int n = io_poll_wait(poll, 1, &event, POLL_TIMEOUT);
		if (n != 1)
			continue;
		if (event.events == IO_EVENT_SIGNAL) {
			if (event.u.sig)
				break;
		} else if (event.events & IO_EVENT_READ) {
			struct can_msg msg = CAN_MSG_INIT;
			while (io_can_read(event.u.handle, &msg) == 1)
				can_net_recv(net, &msg);
		}
	}

	return 0;
}
