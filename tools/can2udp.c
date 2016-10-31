/*!\file
 * This file contains the CAN to UDP forwarding tool.
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

#include <lely/util/daemon.h>
#include <lely/util/diag.h>
#include <lely/util/time.h>
#include <lely/io/addr.h>
#include <lely/io/can.h>
#include <lely/io/poll.h>
#include <lely/io/sock.h>
#include <lely/co/wtm.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USAGE \
	"Arguments: [options] <CAN interface> <address> <port>\n" \
	"Options:\n" \
	"  -4, --ipv4            Use IPv4 for receiving UDP frames (default)\n" \
	"  -6, --ipv6            Use IPv6 for receiving UDP frames \n" \
	"  -b, --broadcast       Send broadcast messages (IPv4 only)\n" \
	"  -D, --no-daemon       Do not run as daemon\n" \
	"  -f, --flush           Flush the send buffer after every received CAN frame\n" \
	"  -h, --help            Display this information\n" \
	"  -i <n>, --interface=<n>\n" \
	"                        Uses WTM interface indicator <n> (in the range [1..127],\n" \
	"                        default: 1)\n" \
	"  -k <ms>, --keep-alive=<ms>\n" \
	"                        Sends a keep-alive message every <ms> milliseconds\n" \
	"                        (default: 10000)\n" \
	"  -p <local port>, --port=<local port>\n" \
	"                        Receive UDP frames on <local port>\n" \
	"  -v, --verbose         Print sent and received CAN frames\n"

void usage(const char *cmdname);

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

#define FLAG_BROADCAST	0x01
#define FLAG_FLUSH	0x02
#define FLAG_NO_DAEMON	0x04
#define FLAG_VERBOSE	0x08

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
	const char *cmd = cmdname(argv[0]);
	diag_set_handler(&cmd_diag_handler, (void *)cmd);

	for (int i = 1; i < argc; i++) {
		char *opt = argv[i];
		if (*opt == '-') {
			opt++;
			if (*opt == '-') {
				opt++;
				if (!strcmp(opt, "help")) {
					usage(cmd);
					return EXIT_SUCCESS;
				} else if (!strcmp(opt, "no-daemon")) {
					flags |= FLAG_NO_DAEMON;
				}
			} else {
				for (; *opt; opt++) {
					switch (*opt) {
					case 'D':
						flags |= FLAG_NO_DAEMON;
						break;
					case 'h':
						usage(cmd);
						return EXIT_SUCCESS;
					}
				}
			}
		}
	}

	if (flags & FLAG_NO_DAEMON) {
		if (__unlikely(daemon_init(argc, argv)))
			return EXIT_FAILURE;
		daemon_main();
		daemon_fini();
		return EXIT_SUCCESS;
	} else {
		return daemon_start(cmd, &daemon_init, &daemon_main,
				&daemon_fini, argc, argv)
				? EXIT_FAILURE : EXIT_SUCCESS;
	}
}

void
usage(const char *cmdname)
{
	fprintf(stderr, "%s: %s", cmdname, USAGE);
}

int
daemon_init(int argc, char *argv[])
{
	if (__unlikely(lely_io_init() == -1)) {
		diag(DIAG_ERROR, get_errc(), "unable to initialize I/O library");
		goto error_io_init;
	}

	int recv_domain = IO_SOCK_IPV4;
	int nif = 1;
	const char *recv_port = NULL;
	const char *ifname = NULL;
	const char *address = NULL;
	const char *send_port = NULL;
	for (int i = 1; i < argc; i++) {
		char *opt = argv[i];
		if (*opt == '-') {
			opt++;
			if (*opt == '-') {
				opt++;
				if (!strcmp(opt, "ipv4")) {
					recv_domain = IO_SOCK_IPV4;
				} else if (!strcmp(opt, "ipv6")) {
					recv_domain = IO_SOCK_IPV6;
				} else if (!strcmp(opt, "broadcast")) {
					flags |= FLAG_BROADCAST;
				} else if (!strcmp(opt, "no-daemon")) {
				} else if (!strcmp(opt, "flush")) {
					flags |= FLAG_FLUSH;
				} else if (!strcmp(opt, "help")) {
				} else if (!strcmp(opt, "interface=")) {
					nif = atoi(opt + 10);
				} else if (!strcmp(opt, "keep-alive=")) {
					keep = atoi(opt + 11);
				} else if (!strcmp(opt, "port=")) {
					recv_port = opt + 5;
				} else if (!strcmp(opt, "verbose")) {
					flags |= FLAG_VERBOSE;
				} else {
					diag(DIAG_ERROR, 0, "invalid option '--%s'",
							opt);
					goto error_arg;
				}
			} else {
				for (; *opt; opt++) {
					switch (*opt) {
					case '4':
						recv_domain = IO_SOCK_IPV4;
						break;
					case '6':
						recv_domain = IO_SOCK_IPV6;
						break;
					case 'b':
						flags |= FLAG_BROADCAST;
						break;
					case 'D': break;
					case 'f':
						flags |= FLAG_FLUSH;
						break;
					case 'h': break;
					case 'i':
						if (__unlikely(++i >= argc)) {
							diag(DIAG_ERROR, 0, "option '-%c' requires an argument",
									*opt);
							goto error_arg;
						}
						nif = atoi(argv[i]);
						break;
					case 'k':
						if (__unlikely(++i >= argc)) {
							diag(DIAG_ERROR, 0, "option '-%c' requires an argument",
									*opt);
							goto error_arg;
						}
						keep = atoi(argv[i]);
						break;
					case 'p':
						if (__unlikely(++i >= argc)) {
							diag(DIAG_ERROR, 0, "option '-%c' requires an argument",
									*opt);
							goto error_arg;
						}
						recv_port = argv[i];
						break;
					case 'v':
						flags |= FLAG_VERBOSE;
						break;
					default:
						diag(DIAG_ERROR, 0, "invalid option '-%c'",
								*opt);
						goto error_arg;
					}
				}
			}
		} else {
			if (!ifname)
				ifname = opt;
			else if (!address)
				address = opt;
			else if (!send_port)
				send_port = opt;
		}
	}

	if (__unlikely(!ifname)) {
		diag(DIAG_ERROR, 0, "no CAN interface specified");
		goto error_arg;
	}

	if (__unlikely(!address)) {
		diag(DIAG_ERROR, 0, "no address specified");
		goto error_arg;
	}

	if (__unlikely(!send_port)) {
		diag(DIAG_ERROR, 0, "no port specified");
		goto error_arg;
	}

	poll = io_poll_create();
	if (__unlikely(!poll)) {
		diag(DIAG_ERROR, get_errc(), "unable to create I/O polling interface");
		goto error_create_poll;
	}
	struct io_event event = IO_EVENT_INIT;

	can_handle = open_can(ifname);
	if (__unlikely(can_handle == IO_HANDLE_ERROR)) {
		diag(DIAG_ERROR, get_errc(), "%s is not a suitable CAN device",
				ifname);
		goto error_open_can;
	}
	event.events = IO_EVENT_READ;
	event.u.handle = can_handle;
	if (__unlikely(io_poll_watch(poll, can_handle, &event, 1) == -1)) {
		diag(DIAG_ERROR, get_errc(), "unable to watch %s", ifname);
		goto error_watch_can;
	}

	send_handle = open_send(address, send_port, flags);
	if (__unlikely(send_handle == IO_HANDLE_ERROR)) {
		diag(DIAG_ERROR, get_errc(), "unable to connect to [%s]:%s",
				address, send_port);
		goto error_open_send;
	}

	if (recv_port) {
		recv_handle = open_recv(recv_domain, recv_port);
		if (__unlikely(recv_handle == IO_HANDLE_ERROR)) {
			diag(DIAG_ERROR, get_errc(), "unable to bind to port %s",
					recv_port);
			goto error_open_recv;
		}
		event.events = IO_EVENT_READ;
		event.u.handle = recv_handle;
		if (__unlikely(io_poll_watch(poll, recv_handle, &event, 1)
				== -1)) {
			diag(DIAG_ERROR, get_errc(), "unable to watch port %s",
					recv_port);
			goto error_watch_recv;
		}
	}

	wtm = co_wtm_create();
	if (__unlikely(!wtm)) {
		diag(DIAG_ERROR, get_errc(), "unable to create WTM interface");
		goto error_create_wtm;
	}
	if (__unlikely(co_wtm_set_nif(wtm, nif) == -1)) {
		diag(DIAG_ERROR, get_errc(), "invalid WTM interface indicator: %d",
				nif);
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
		while (keep > 0 && (timeout = timespec_diff_msec(&next, &now))
				<= 0) {
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
		} else if ((event.events & IO_EVENT_READ)
				&& event.u.handle == can_handle) {
			// Read a single CAN frame.
			struct can_msg msg = CAN_MSG_INIT;
			int n = io_can_read(can_handle, &msg);
			if (__unlikely(n != 1)) {
				// If an error occurred or an error frame was
				// received, update the diagnostic parameters of
				// the CAN interface.
				int st = io_can_get_state(can_handle);
				int err = 0xf;
				io_can_get_error(can_handle, &err);
				co_wtm_set_diag_can(wtm, 1, st, err, 0xff,
						0xffff, 0xffff, 0xffff);
				continue;
			}
			// Print the frame in verbose mode.
			if (flags & FLAG_VERBOSE) {
				char s[60] = { 0 };
				snprintf_can_msg(s, sizeof(s), &msg);
				printf("[% 10ld.%09ld] > %s\n", now.tv_sec,
						now.tv_nsec, s);
			}
			// Process the frame.
			co_wtm_send(wtm, 1, &msg);
			if (flags & FLAG_FLUSH)
				co_wtm_flush(wtm);
		} else if ((event.events & IO_EVENT_READ)
				&& event.u.handle == recv_handle) {
			// Read a single generic frame.
			char buf[CO_WTM_MAX_LEN];
			ssize_t result = io_read(recv_handle, buf, sizeof(buf));
			if (__unlikely(result == -1))
				continue;
			// Process the frame.
			co_wtm_recv(wtm, buf, result);
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

	errc_t errc = 0;

	io_handle_t handle = io_open_can(ifname);
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		errc = get_errc();
		goto error_open_can;
	}

	if (__unlikely(io_set_flags(handle, IO_FLAG_NONBLOCK) == -1)) {
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

	errc_t errc = 0;

	struct io_addrinfo hints = {
		.type = IO_SOCK_DGRAM
	};
	struct io_addrinfo info;
	if (__unlikely(io_get_addrinfo(1, &info, address, port, &hints) == -1)) {
		errc = get_errc();
		goto error_get_addrinfo;
	}

	io_handle_t handle = io_open_socket(info.domain, info.type);
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		errc = get_errc();
		goto error_open_socket;
	}

	if (__unlikely(info.domain == IO_SOCK_IPV4 && (flags & FLAG_BROADCAST)
			&& io_sock_set_broadcast(handle, 1) == -1)) {
		errc = get_errc();
		goto error_set_broadcast;
	}

	if (__unlikely(io_connect(handle, &info.addr) == -1)) {
		errc = get_errc();
		goto error_connect;
	}

	if (__unlikely(io_set_flags(handle, IO_FLAG_NONBLOCK) == -1)) {
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
	errc_t errc = 0;

	io_handle_t handle = io_open_socket(domain, IO_SOCK_DGRAM);
	if (__unlikely(!handle)) {
		errc = get_errc();
		goto error_open_socket;
	}

	if (__unlikely(io_sock_set_reuseaddr(handle, 1) == -1)) {
		errc = get_errc();
		goto error_set_reuseaddr;
	}

	io_addr_t addr;
	if (domain == IO_SOCK_IPV6)
		io_addr_set_ipv6_n(&addr, NULL, atoi(port));
	else
		io_addr_set_ipv4_n(&addr, NULL, atoi(port));
	if (__unlikely(io_sock_bind(handle, &addr) == -1)) {
		errc = get_errc();
		goto error_bind;
	}

	if (__unlikely(io_set_flags(handle, IO_FLAG_NONBLOCK) == -1)) {
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
	__unused_var(wtm);
	assert(msg);
	io_handle_t handle = data;
	assert(handle != IO_HANDLE_ERROR);

	if (nif != 1)
		return 0;

	// Print the frame in verbose mode.
	if (flags & FLAG_VERBOSE) {
		char s[60] = { 0 };
		snprintf_can_msg(s, sizeof(s), msg);
		printf("[% 10ld.%09ld] < %s\n", tp->tv_sec, tp->tv_nsec, s);
	}

	// Send the received frame to the CAN bus.
	return io_can_write(handle, msg) == 1 ? 0 : -1;
}

int
wtm_send(co_wtm_t *wtm, const void *buf, size_t nbytes, void *data)
{
	__unused_var(wtm);
	io_handle_t handle = data;
	assert(handle != IO_HANDLE_ERROR);

	return io_write(handle, buf, nbytes) == (ssize_t)nbytes ? 0 : -1;
}

