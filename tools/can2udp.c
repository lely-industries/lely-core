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

#include <lely/util/diag.h>
#include <lely/util/time.h>
#include <lely/can/socket.h>
#include <lely/co/wtm.h>

#include <assert.h>
#include <fcntl.h>
#include <linux/can.h>
#include <net/if.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#define CMD_USAGE \
	"Arguments: [options] <CAN interface> <address> <port>\n" \
	"Options:\n" \
	"  -4, --ipv4            Use IPv4 for receiving UDP frames (default)\n" \
	"  -6, --ipv6            Use IPv6 for receiving UDP frames \n" \
	"  -b, --broadcast       Send broadcast messages (IPv4 only)\n" \
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

#define FLAG_BROADCAST	0x01
#define FLAG_FLUSH	0x02
#define FLAG_VERBOSE	0x04

int flags = 0;

void cmd_diag_handler(void *handle, enum diag_severity severity, errc_t errc,
		const char *format, va_list ap);

int open_can(const char *ifname);
int open_send(const char *address, const char *port, int flags);
int open_recv(int domain, const char *port);

int wtm_recv(co_wtm_t *wtm, uint8_t nif, const struct timespec *tp,
		const struct can_msg *msg, void *data);
int wtm_send(co_wtm_t *wtm, const void *buf, size_t nbytes, void *data);

int
main(int argc, char *argv[])
{
	const char *cmd = cmdname(argv[0]);
	diag_set_handler(&cmd_diag_handler, (void *)cmd);

	int recv_domain = AF_INET;
	int nif = 1;
	int keep = 10000;
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
					recv_domain = AF_INET;
				} else if (!strcmp(opt, "ipv6")) {
					recv_domain = AF_INET6;
				} else if (!strcmp(opt, "broadcast")) {
					flags |= FLAG_BROADCAST;
				} else if (!strcmp(opt, "flush")) {
					flags |= FLAG_FLUSH;
				} else if (!strcmp(opt, "help")) {
					diag(DIAG_INFO, 0, CMD_USAGE);
					goto error_arg;
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
						recv_domain = AF_INET;
						break;
					case '6':
						recv_domain = AF_INET6;
						break;
					case 'b':
						flags |= FLAG_BROADCAST;
						break;
					case 'f':
						flags |= FLAG_FLUSH;
						break;
					case 'h':
						diag(DIAG_INFO, 0, CMD_USAGE);
						goto error_arg;
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

	int epfd = epoll_create1(EPOLL_CLOEXEC);
	if (__unlikely(epfd == -1)) {
		diag(DIAG_ERROR, get_errc(), "unable top create epoll instance");
		goto error_open_ep;
	}
	struct epoll_event ev = { 0, { NULL } };

	int canfd = open_can(ifname);
	if (__unlikely(canfd == -1)) {
		diag(DIAG_ERROR, get_errc(), "%s is not a suitable CAN device",
				ifname);
		goto error_open_can;
	}
	ev.events = EPOLLIN;
	ev.data.fd = canfd;
	if (__unlikely(epoll_ctl(epfd, EPOLL_CTL_ADD, canfd, &ev) == -1)) {
		diag(DIAG_ERROR, get_errc(), "unable to register file descriptor with epoll");
		goto error_add_can;
	}

	int sendfd = open_send(address, send_port, flags);
	if (__unlikely(sendfd == -1))
		goto error_open_send;

	int recvfd = -1;
	if (recv_port) {
		recvfd = open_recv(recv_domain, recv_port);
		if (__unlikely(recvfd == -1)) {
			diag(DIAG_ERROR, get_errc(), "unable to bind to port %s",
					recv_port);
			goto error_open_recv;
		}
		ev.events = EPOLLIN;
		ev.data.fd = recvfd;
		if (__unlikely(epoll_ctl(epfd, EPOLL_CTL_ADD, recvfd, &ev)
				== -1)) {
			diag(DIAG_ERROR, get_errc(), "unable to register file descriptor with epoll");
			goto error_add_recv;
		}
	}

	co_wtm_t *wtm = co_wtm_create();
	if (__unlikely(!wtm)) {
		diag(DIAG_ERROR, get_errc(), "unable to create WTM interface");
		goto error_create_wtm;
	}
	if (__unlikely(co_wtm_set_nif(wtm, nif) == -1)) {
		diag(DIAG_ERROR, get_errc(), "invalid WTM interface indicator: %d",
				nif);
		goto error_set_nif;
	}
	co_wtm_set_recv_func(wtm, &wtm_recv, (void *)(intptr_t)canfd);
	co_wtm_set_send_func(wtm, &wtm_send, (void *)(intptr_t)sendfd);

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
		int n = epoll_wait(epfd, &ev, 1, timeout);
		// Update the clock. This might trigger timer-overrun messages.
		timespec_get(&now, TIME_UTC);
		co_wtm_set_time(wtm, 1, &now);
		// Ignore non-input events.
		if (n != 1 || !(ev.events & EPOLLIN))
			continue;
		if (ev.data.fd == canfd) {
			// Read a single CAN frame.
			struct can_frame frame;
			ssize_t result;
			do result = recv(canfd, &frame, sizeof(frame), 0);
			while (__unlikely(result == -1 && errno == EINTR));
			if (__unlikely(result != (ssize_t)sizeof(frame)))
				continue;
			// Process the frame.
			struct can_msg msg = CAN_MSG_INIT;
			can_frame2can_msg(&frame, &msg);
			co_wtm_send(wtm, 1, &msg);
			if (flags & FLAG_FLUSH)
				co_wtm_flush(wtm);
			// Print the frame in verbose mode.
			if (flags & FLAG_VERBOSE) {
				char s[60] = { 0 };
				snprintf_can_msg(s, sizeof(s), &msg);
				printf("[%ld.%09ld] > %s\n", now.tv_sec,
						now.tv_nsec, s);
			};
		} else if (ev.data.fd == recvfd) {
			// Read a single generic frame.
			char buf[CO_WTM_MAX_LEN];
			ssize_t result;
			do result = recv(recvfd, buf, sizeof(buf), 0);
			while (__unlikely(result == -1 && errno == EINTR));
			if (__unlikely(result == -1))
				continue;
			// Process the frame.
			co_wtm_recv(wtm, buf, result);
		}
	}

	co_wtm_destroy(wtm);
	if (recvfd != -1)
		close(recvfd);
	close(sendfd);
	close(canfd);
	close(epfd);

	return EXIT_SUCCESS;

error_set_nif:
	co_wtm_destroy(wtm);
error_create_wtm:
error_add_recv:
	if (recvfd != -1)
		close(recvfd);
error_open_recv:
	close(sendfd);
error_open_send:
error_add_can:
	close(canfd);
error_open_can:
	close(epfd);
error_open_ep:
error_arg:
	return EXIT_FAILURE;
}

void
cmd_diag_handler(void *handle, enum diag_severity severity, errc_t errc,
		const char *format, va_list ap)
{
	if (handle)
		fprintf(stderr, "%s: ", (const char *)handle);
	default_diag_handler(handle, severity, errc, format, ap);
}

int
open_can(const char *ifname)
{
	assert(ifname);

	int errsv = 0;

	int s = socket(PF_CAN, SOCK_RAW | SOCK_CLOEXEC, CAN_RAW);
	if (__unlikely(s == -1)) {
		errsv = errno;
		goto error_socket;
	}

	struct ifreq ifr;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = '\0';
	if (__unlikely(ioctl(s, SIOCGIFINDEX, &ifr) == -1)) {
		errsv = errno;
		goto error_ioctl;
	}

	struct sockaddr_can addr;
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if (__unlikely(bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1)) {
		errsv = errno;
		goto error_bind;
	}

	int flags = fcntl(s, F_GETFL, 0);
	if (__unlikely(flags == -1 || fcntl(s, F_SETFL,
			flags | O_NONBLOCK) == -1)) {
		errsv = errno;
		goto error_fcntl;
	}

	return s;

error_fcntl:
error_bind:
error_ioctl:
	close(s);
error_socket:
	errno = errsv;
	return -1;
}

int
open_send(const char *address, const char *port, int flags)
{
	assert(address);

	int errsv = 0;

	struct addrinfo ai_hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_DGRAM
	};

	struct addrinfo *ai = NULL;
	int ecode = getaddrinfo(address, port, &ai_hints, &ai);
	if (__unlikely(ecode)) {
		diag(DIAG_ERROR, 0, "[%s]:%s: %s",
				address, port, gai_strerror(ecode));
		goto error_getaddrinfo;
	}
	assert(ai);

	int s = socket(ai->ai_family, ai->ai_socktype | SOCK_CLOEXEC, 0);
	if (__unlikely(s == -1)) {
		errsv = errno;
		goto error_socket;
	}

	if (__unlikely(ai->ai_family == AF_INET && (flags & FLAG_BROADCAST)
			&& setsockopt(s, SOL_SOCKET, SO_BROADCAST, &(int){ 1 },
			sizeof(int)) == -1)) {
		errsv = errno;
		goto error_setsockopt;
	}

	if (__unlikely(connect(s, ai->ai_addr, ai->ai_addrlen) == -1)) {
		errsv = errno;
		goto error_connect;
	}

	flags = fcntl(s, F_GETFL, 0);
	if (__unlikely(flags == -1 || fcntl(s, F_SETFL,
			flags | O_NONBLOCK) == -1)) {
		errsv = errno;
		goto error_fcntl;
	}

	freeaddrinfo(ai);

	return s;

error_fcntl:
error_connect:
error_setsockopt:
	close(s);
error_socket:
	freeaddrinfo(ai);
	errno = errsv;
	diag(DIAG_ERROR, get_errc(), "unable to connect");
error_getaddrinfo:
	return -1;
}

int
open_recv(int domain, const char *port)
{
	int errsv = 0;

	int s = socket(domain, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (__unlikely(s == -1)) {
		errsv = errno;
		goto error_socket;
	}

	if (__unlikely(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 },
			sizeof(int)) == -1)) {
		errsv = errno;
		goto error_setsockopt;
	}

	if (domain == AF_INET6) {
		struct sockaddr_in6 addr = {
			.sin6_family = AF_INET6,
			.sin6_port = htons(atoi(port)),
			.sin6_addr = in6addr_any
		};
		if (__unlikely(bind(s, (struct sockaddr *)&addr, sizeof(addr))
				== -1)) {
			errsv = errno;
			goto error_bind;
		}
	} else {
		struct sockaddr_in addr = {
			.sin_family = AF_INET,
			.sin_port = htons(atoi(port)),
			.sin_addr.s_addr = htonl(INADDR_ANY)
		};
		if (__unlikely(bind(s, (struct sockaddr *)&addr, sizeof(addr))
				== -1)) {
			errsv = errno;
			goto error_bind;
		}
	}

	int flags = fcntl(s, F_GETFL, 0);
	if (__unlikely(flags == -1 || fcntl(s, F_SETFL,
			flags | O_NONBLOCK) == -1)) {
		errsv = errno;
		goto error_fcntl;
	}

	return s;

error_fcntl:
error_bind:
error_setsockopt:
	close(s);
error_socket:
	errno = errsv;
	return -1;
}

int
wtm_recv(co_wtm_t *wtm, uint8_t nif, const struct timespec *tp,
		const struct can_msg *msg, void *data)
{
	__unused_var(wtm);
	assert(msg);
	int s = (intptr_t)data;

	if (nif != 1)
		return 0;

	struct can_frame frame;
	can_msg2can_frame(msg, &frame);

	// Send the received frame to the CAN bus.
	ssize_t result;
	do result = send(s, &frame, sizeof(frame), MSG_NOSIGNAL);
	while (__unlikely(result == -1 && errno == EINTR));

	// Print the frame in verbose mode.
	if (flags & FLAG_VERBOSE) {
		char s[60] = { 0 };
		snprintf_can_msg(s, sizeof(s), msg);
		printf("[% 10ld.%09ld] < %s\n", tp->tv_sec, tp->tv_nsec, s);
	}

	return result == (ssize_t)sizeof(frame) ? 0 : -1;
}

int
wtm_send(co_wtm_t *wtm, const void *buf, size_t nbytes, void *data)
{
	__unused_var(wtm);
	int s = (intptr_t)data;

	ssize_t result;
	do result = send(s, buf, nbytes, MSG_NOSIGNAL);
	while (__unlikely(result == -1 && errno == EINTR));

	return result == (ssize_t)nbytes ? 0 : -1;
}

