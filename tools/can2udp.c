/**@file
 * This file contains the CAN to UDP forwarding tool.
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

#include <lely/co/wtm.h>
#include <lely/io/addr.h>
#include <lely/io/can.h>
#include <lely/io/poll.h>
#include <lely/io/sock.h>
#include <lely/libc/unistd.h>
#include <lely/util/daemon.h>
#include <lely/util/diag.h>
#include <lely/util/time.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
#define HELP \
	"Arguments: [options...] <CAN interface> address port\n" \
	"Options:\n" \
	"  -4, --ipv4            Use IPv4 for receiving UDP frames (default)\n" \
	"  -6, --ipv6            Use IPv6 for receiving UDP frames\n" \
	"  -b, --broadcast       Send broadcast messages (IPv4 only)\n" \
	"  -D, --no-daemon       Do not run as daemon\n" \
	"  -f, --flush           Flush the send buffer after every received CAN frame\n" \
	"  -h, --help            Display this information\n" \
	"  -i <n>, --interface=<n>\n" \
	"                        Use WTM interface indicator <n> (in the range [1..127],\n" \
	"                        default: 1)\n" \
	"  -k <ms>, --keep-alive=<ms>\n" \
	"                        Sends a keep-alive message every <ms> milliseconds\n" \
	"                        (default: 10000)\n" \
	"  -p <local port>, --port=<local port>\n" \
	"                        Receive UDP frames on <local port>\n" \
	"  -v, --verbose         Print sent and received CAN frames"
// clang-format on

int daemon_init(int argc, char *argv[]);
void daemon_main();
void daemon_fini();
void daemon_handler(int sig, void *handle);

io_handle_t open_can(const char *ifname);
io_handle_t open_send(const char *address, const char *port, int flags);
io_handle_t open_recv(int domain, const char *port);

int wtm_recv(co_wtm_t *wtm, uint8_t nif, const struct timespec *tp,
		const struct can_msg *msg, void *data);
int wtm_send(co_wtm_t *wtm, const void *buf, size_t nbytes, void *data);

#define FLAG_BROADCAST 0x01
#define FLAG_FLUSH 0x02
#define FLAG_HELP 0x04
#define FLAG_NO_DAEMON 0x08
#define FLAG_VERBOSE 0x10

int flags;
int keep = 10000;
io_poll_t *poll;
io_handle_t can_handle = IO_HANDLE_ERROR;
io_handle_t send_handle = IO_HANDLE_ERROR;
io_handle_t recv_handle = IO_HANDLE_ERROR;
co_wtm_t *wtm;

int
main(int argc, char *argv[])
{
	argv[0] = (char *)cmdname(argv[0]);
	diag_set_handler(&cmd_diag_handler, argv[0]);

	opterr = 0;
	optind = 1;
	while (optind < argc) {
		char *arg = argv[optind];
		if (*arg != '-') {
			optind++;
		} else if (*++arg == '-') {
			optind++;
			if (!*++arg)
				break;
			if (!strcmp(arg, "help")) {
				flags |= FLAG_HELP;
			} else if (!strcmp(arg, "no-daemon")) {
				flags |= FLAG_NO_DAEMON;
			}
		} else {
			int c = getopt(argc, argv, ":Dh");
			if (c == -1)
				break;
			switch (c) {
			case ':':
			case '?': break;
			case 'D': flags |= FLAG_NO_DAEMON; break;
			case 'h': flags |= FLAG_HELP; break;
			}
		}
	}

	if (flags & FLAG_HELP) {
		diag(DIAG_INFO, 0, "%s", HELP);
		return EXIT_SUCCESS;
	}

	if (flags & FLAG_NO_DAEMON) {
		if (daemon_init(argc, argv))
			return EXIT_FAILURE;
		daemon_main();
		daemon_fini();
		return EXIT_SUCCESS;
	} else {
		// clang-format off
		return daemon_start(argv[0], &daemon_init, &daemon_main,
				&daemon_fini, argc, argv)
				? EXIT_FAILURE
				: EXIT_SUCCESS;
		// clang-format on
	}
}

int
daemon_init(int argc, char *argv[])
{
	if (lely_io_init() == -1) {
		diag(DIAG_ERROR, get_errc(),
				"unable to initialize I/O library");
		goto error_io_init;
	}

	int recv_domain = IO_SOCK_IPV4;
	int nif = 1;
	const char *recv_port = NULL;
	const char *ifname = NULL;
	const char *address = NULL;
	const char *send_port = NULL;

	opterr = 0;
	optind = 1;
	int optpos = 0;
	while (optind < argc) {
		char *arg = argv[optind];
		if (*arg != '-') {
			optind++;
			switch (optpos++) {
			case 0: ifname = arg; break;
			case 1: address = arg; break;
			case 2: send_port = arg; break;
			default:
				diag(DIAG_ERROR, 0, "extra argument %s", arg);
				break;
			}
		} else if (*++arg == '-') {
			optind++;
			if (!*++arg)
				break;
			if (!strcmp(arg, "ipv4")) {
				recv_domain = IO_SOCK_IPV4;
			} else if (!strcmp(arg, "ipv6")) {
				recv_domain = IO_SOCK_IPV6;
			} else if (!strcmp(arg, "broadcast")) {
				flags |= FLAG_BROADCAST;
			} else if (!strcmp(arg, "no-daemon")) {
			} else if (!strcmp(arg, "flush")) {
				flags |= FLAG_FLUSH;
			} else if (!strcmp(arg, "help")) {
			} else if (!strncmp(arg, "interface=", 10)) {
				nif = atoi(arg + 10);
			} else if (!strncmp(arg, "keep-alive=", 11)) {
				keep = atoi(arg + 11);
			} else if (!strncmp(arg, "port=", 5)) {
				recv_port = arg + 5;
			} else if (!strcmp(arg, "verbose")) {
				flags |= FLAG_VERBOSE;
			} else {
				diag(DIAG_ERROR, 0, "illegal option -- %s",
						arg);
			}
		} else {
			int c = getopt(argc, argv, ":46bDfhi:k:p:v");
			if (c == -1)
				break;
			switch (c) {
			case ':':
				diag(DIAG_ERROR, 0,
						"option requires an argument -- %c",
						optopt);
				break;
			case '?':
				diag(DIAG_ERROR, 0, "illegal option -- %c",
						optopt);
				break;
			case '4': recv_domain = IO_SOCK_IPV4; break;
			case '6': recv_domain = IO_SOCK_IPV6; break;
			case 'b': flags |= FLAG_BROADCAST; break;
			case 'D': break;
			case 'f': flags |= FLAG_FLUSH; break;
			case 'h': break;
			case 'i': nif = atoi(optarg); break;
			case 'k': keep = atoi(optarg); break;
			case 'p': recv_port = optarg; break;
			case 'v': flags |= FLAG_VERBOSE; break;
			}
		}
	}
	for (char *arg = argv[optind]; optind < argc; arg = argv[++optind]) {
		switch (optpos++) {
		case 0: ifname = arg; break;
		case 1: address = arg; break;
		case 2: send_port = arg; break;
		default: diag(DIAG_ERROR, 0, "extra argument %s", arg); break;
		}
	}

	if (optpos < 1 || !ifname) {
		diag(DIAG_ERROR, 0, "no CAN interface specified");
		goto error_arg;
	}

	if (optpos < 2 || !address) {
		diag(DIAG_ERROR, 0, "no address specified");
		goto error_arg;
	}

	if (optpos < 3 || !send_port) {
		diag(DIAG_ERROR, 0, "no port specified");
		goto error_arg;
	}

	poll = io_poll_create();
	if (!poll) {
		diag(DIAG_ERROR, get_errc(),
				"unable to create I/O polling interface");
		goto error_create_poll;
	}
	struct io_event event = IO_EVENT_INIT;

	can_handle = open_can(ifname);
	if (can_handle == IO_HANDLE_ERROR) {
		diag(DIAG_ERROR, get_errc(), "%s is not a suitable CAN device",
				ifname);
		goto error_open_can;
	}
	event.events = IO_EVENT_READ;
	event.u.handle = can_handle;
	if (io_poll_watch(poll, can_handle, &event, 1) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to watch %s", ifname);
		goto error_watch_can;
	}

	send_handle = open_send(address, send_port, flags);
	if (send_handle == IO_HANDLE_ERROR) {
		diag(DIAG_ERROR, get_errc(), "unable to connect to [%s]:%s",
				address, send_port);
		goto error_open_send;
	}

	if (recv_port) {
		recv_handle = open_recv(recv_domain, recv_port);
		if (recv_handle == IO_HANDLE_ERROR) {
			diag(DIAG_ERROR, get_errc(),
					"unable to bind to port %s", recv_port);
			goto error_open_recv;
		}
		event.events = IO_EVENT_READ;
		event.u.handle = recv_handle;
		if (io_poll_watch(poll, recv_handle, &event, 1) == -1) {
			diag(DIAG_ERROR, get_errc(), "unable to watch port %s",
					recv_port);
			goto error_watch_recv;
		}
	}

	wtm = co_wtm_create();
	if (!wtm) {
		diag(DIAG_ERROR, get_errc(), "unable to create WTM interface");
		goto error_create_wtm;
	}
	if (co_wtm_set_nif(wtm, nif) == -1) {
		diag(DIAG_ERROR, get_errc(),
				"invalid WTM interface indicator: %d", nif);
		goto error_set_nif;
	}
	co_wtm_set_recv_func(wtm, &wtm_recv, can_handle);
	co_wtm_set_send_func(wtm, &wtm_send, send_handle);

	// Disable verbose output in daemon mode.
	if (!(flags & FLAG_NO_DAEMON))
		flags &= ~FLAG_VERBOSE;

	daemon_set_handler(&daemon_handler, poll);

	return 0;

error_set_nif:
error_create_wtm:
	co_wtm_destroy(wtm);
	wtm = NULL;
error_watch_recv:
	if (recv_handle != IO_HANDLE_ERROR)
		io_close(recv_handle);
	recv_handle = IO_HANDLE_ERROR;
error_open_recv:
	io_close(send_handle);
	send_handle = IO_HANDLE_ERROR;
error_open_send:
error_watch_can:
	io_close(can_handle);
	can_handle = IO_HANDLE_ERROR;
error_open_can:
	io_poll_destroy(poll);
	poll = NULL;
error_create_poll:
error_arg:
	lely_io_fini();
error_io_init:
	return -1;
}

void
daemon_main()
{
	struct timespec now = { 0, 0 };
	timespec_get(&now, TIME_UTC);
	struct timespec next = now;

	for (;;) {
		// Send a keep-alive message if necessary and compute the
		// timeout (number of ms to next keep-alive message).
		int timeout = -1;
		// clang-format off
		while (keep > 0 && (timeout = timespec_diff_msec(&next, &now))
				<= 0) {
			// clang-format on
			co_wtm_send_alive(wtm);
			timespec_add_msec(&next, keep);
		}
		// Wait for input events.
		struct io_event event;
		int n = io_poll_wait(poll, 1, &event, timeout);
		// Update the clock. This might trigger timer-overrun messages.
		timespec_get(&now, TIME_UTC);
		co_wtm_set_time(wtm, 1, &now);
		// Handle events.
		if (n != 1)
			continue;
		if (event.events == IO_EVENT_SIGNAL) {
			switch (event.u.sig) {
			case DAEMON_STOP:
				// Stop the daemon by returning.
				return;
			case DAEMON_PAUSE:
				// Pause the daemon by unregistering the device
				// handles so we only responds to signals.
				io_poll_watch(poll, can_handle, NULL, 0);
				if (recv_handle != IO_HANDLE_ERROR)
					io_poll_watch(poll, recv_handle, NULL,
							0);
				daemon_status(DAEMON_PAUSE);
				break;
			case DAEMON_CONTINUE:
				// Continue the daemon by re-registering the
				// device handles.
				event.events = IO_EVENT_READ;
				event.u.handle = can_handle;
				io_poll_watch(poll, can_handle, &event, 1);
				if (recv_handle != IO_HANDLE_ERROR) {
					event.u.handle = recv_handle;
					io_poll_watch(poll, recv_handle, &event,
							1);
				}
				daemon_status(DAEMON_CONTINUE);
				break;
			}
		} else if (event.u.handle == can_handle
				&& (event.events & IO_EVENT_READ)) {
			int result;
			struct can_msg msg = CAN_MSG_INIT;
			while ((result = io_can_read(can_handle, &msg)) == 1) {
				// Print the frame in verbose mode.
				if (flags & FLAG_VERBOSE) {
					char s[60] = { 0 };
					snprintf_can_msg(s, sizeof(s), &msg);
					printf("[%10ld.%09ld] > %s\n",
							now.tv_sec, now.tv_nsec,
							s);
				}
				// Process the frame.
				co_wtm_send(wtm, 1, &msg);
				if (flags & FLAG_FLUSH)
					co_wtm_flush(wtm);
			}
			// Treat the reception of an error frame, or any error
			// other than an empty receive buffer, as an error
			// event.
			// clang-format off
			if (!result || (result == -1
					&& get_errnum() != ERRNUM_AGAIN
					&& get_errnum() != ERRNUM_WOULDBLOCK))
				// clang-format on
				event.events |= IO_EVENT_ERROR;
		} else if (event.u.handle == recv_handle
				&& (event.events & IO_EVENT_READ)) {
			// Read a single generic frame.
			char buf[CO_WTM_MAX_LEN];
			ssize_t result = io_read(recv_handle, buf, sizeof(buf));
			if (result == -1)
				continue;
			// Process the frame.
			co_wtm_recv(wtm, buf, result);
		}
		if (event.u.handle == can_handle
				&& (event.events & IO_EVENT_ERROR)) {
			// If an error occurred or an error frame was received,
			// update the diagnostic parameters of the CAN
			// interface.
			int st = io_can_get_state(can_handle);
			int err = 0xf;
			io_can_get_error(can_handle, &err);
			co_wtm_set_diag_can(wtm, 1, st, err, 0xff, 0xffff,
					0xffff, 0xffff);
		}
	}
}

void
daemon_fini()
{
	co_wtm_destroy(wtm);
	wtm = NULL;

	if (recv_handle != IO_HANDLE_ERROR)
		io_close(recv_handle);
	recv_handle = IO_HANDLE_ERROR;

	io_close(send_handle);
	send_handle = IO_HANDLE_ERROR;

	io_close(can_handle);
	can_handle = IO_HANDLE_ERROR;

	io_poll_destroy(poll);
	poll = NULL;

	lely_io_fini();
}

void
daemon_handler(int sig, void *handle)
{
	io_poll_t *poll = handle;
	assert(poll);

	io_poll_signal(poll, sig);
}

io_handle_t
open_can(const char *ifname)
{
	assert(ifname);

	int errc = 0;

	io_handle_t handle = io_open_can(ifname);
	if (handle == IO_HANDLE_ERROR) {
		errc = get_errc();
		goto error_open_can;
	}

	if (io_set_flags(handle, IO_FLAG_NONBLOCK) == -1) {
		errc = get_errc();
		goto error_set_flags;
	}

	return handle;

error_set_flags:
	io_close(handle);
error_open_can:
	set_errc(errc);
	return IO_HANDLE_ERROR;
}

io_handle_t
open_send(const char *address, const char *port, int flags)
{
	assert(address);

	int errc = 0;

	struct io_addrinfo hints = { .type = IO_SOCK_DGRAM };
	struct io_addrinfo info;
	// clang-format off
	if (io_get_addrinfo(1, &info, address, port, &hints) == -1) {
		// clang-format on
		errc = get_errc();
		goto error_get_addrinfo;
	}

	io_handle_t handle = io_open_socket(info.domain, info.type);
	if (handle == IO_HANDLE_ERROR) {
		errc = get_errc();
		goto error_open_socket;
	}

	if (info.domain == IO_SOCK_IPV4 && (flags & FLAG_BROADCAST)
			&& io_sock_set_broadcast(handle, 1) == -1) {
		errc = get_errc();
		goto error_set_broadcast;
	}

	if (io_connect(handle, &info.addr) == -1) {
		errc = get_errc();
		goto error_connect;
	}

	if (io_set_flags(handle, IO_FLAG_NONBLOCK) == -1) {
		errc = get_errc();
		goto error_set_flags;
	}

	return handle;

error_set_flags:
error_connect:
error_set_broadcast:
	io_close(handle);
error_open_socket:
error_get_addrinfo:
	set_errc(errc);
	return IO_HANDLE_ERROR;
}

io_handle_t
open_recv(int domain, const char *port)
{
	int errc = 0;

	io_handle_t handle = io_open_socket(domain, IO_SOCK_DGRAM);
	if (!handle) {
		errc = get_errc();
		goto error_open_socket;
	}

	if (io_sock_set_reuseaddr(handle, 1) == -1) {
		errc = get_errc();
		goto error_set_reuseaddr;
	}

	io_addr_t addr;
	if (domain == IO_SOCK_IPV6)
		io_addr_set_ipv6_n(&addr, NULL, atoi(port));
	else
		io_addr_set_ipv4_n(&addr, NULL, atoi(port));
	if (io_sock_bind(handle, &addr) == -1) {
		errc = get_errc();
		goto error_bind;
	}

	if (io_set_flags(handle, IO_FLAG_NONBLOCK) == -1) {
		errc = get_errc();
		goto error_set_flags;
	}

	return handle;

error_set_flags:
error_bind:
error_set_reuseaddr:
	io_close(handle);
error_open_socket:
	set_errc(errc);
	return IO_HANDLE_ERROR;
}

int
wtm_recv(co_wtm_t *wtm, uint8_t nif, const struct timespec *tp,
		const struct can_msg *msg, void *data)
{
	(void)wtm;
	assert(msg);
	io_handle_t handle = data;
	assert(handle != IO_HANDLE_ERROR);

	if (nif != 1)
		return 0;

	// Print the frame in verbose mode.
	if (flags & FLAG_VERBOSE) {
		char s[60] = { 0 };
		snprintf_can_msg(s, sizeof(s), msg);
		printf("[%10ld.%09ld] < %s\n", tp->tv_sec, tp->tv_nsec, s);
	}

	// Send the received frame to the CAN bus.
	return io_can_write(handle, msg) == 1 ? 0 : -1;
}

int
wtm_send(co_wtm_t *wtm, const void *buf, size_t nbytes, void *data)
{
	(void)wtm;
	io_handle_t handle = data;
	assert(handle != IO_HANDLE_ERROR);

	return io_write(handle, buf, nbytes) == (ssize_t)nbytes ? 0 : -1;
}
