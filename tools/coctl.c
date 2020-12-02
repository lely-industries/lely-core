/**@file
 * This file contains the CANopen control tool (a CiA 309-3 gateway).
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

#include <lely/can/err.h>
#include <lely/co/dcf.h>
#include <lely/co/gw_txt.h>
#include <lely/co/nmt.h>
#include <lely/io/can.h>
#include <lely/io/poll.h>
#include <lely/libc/stdio.h>
#include <lely/libc/threads.h>
#include <lely/libc/unistd.h>
#include <lely/util/diag.h>
#include <lely/util/lex.h>
#include <lely/util/time.h>

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#if _WIN32
#include <io.h>
#endif

struct co_net {
	const char *can_path;
	const char *dcf_path;
	io_handle_t handle;
	int st;
	can_net_t *net;
	co_dev_t *dev;
	co_nmt_t *nmt;
};

// clang-format off
#define HELP \
	"Arguments: [options...] [<CAN interface> <EDS/DCF filename>]...\n" \
	"Options:\n" \
	"  -e, --exit            Exit on error\n" \
	"  -h, --help            Display this information\n" \
	"  -i <ms>, --inhibit=<ms>\n" \
	"                        Wait at least <ms> milliseconds between requests\n" \
	"                        (default: 100)\n" \
	"  -m, --monitor         Do not exit on EOF (monitor mode)\n" \
	"  -W, --no-wait         Do not wait for the previous command to complete\n" \
	"                        before accepting the next one"
// clang-format on

#define FLAG_EXIT 0x01
#define FLAG_HELP 0x02
#define FLAG_MONITOR 0x04
#define FLAG_NO_WAIT 0x08

#define INHIBIT 100

#define POLL_TIMEOUT 10

int can_send(const struct can_msg *msg, void *data);

int gw_send(const struct co_gw_srv *srv, void *data);
void gw_rate(co_unsigned16_t id, co_unsigned16_t rate, void *data);
int gw_txt_recv(const char *txt, void *data);
int gw_txt_send(const struct co_gw_req *req, void *data);

void sig_done(int sig);
int io_thrd_start(void *arg);

void co_net_err(struct co_net *net);

struct co_net net[CO_GW_NUM_NET];
io_poll_t *poll;

int flags;
int inhibit = INHIBIT;

mtx_t wait_mtx;
cnd_t wait_cond;
int wait;
int status;

mtx_t recv_mtx;
char *recv_buf;

mtx_t send_mtx;
cnd_t send_cond;
char *send_buf;

sig_atomic_t done;

int
main(int argc, char *argv[])
{
	argv[0] = (char *)cmdname(argv[0]);
	diag_set_handler(&cmd_diag_handler, argv[0]);

	for (co_unsigned16_t id = 0; id < CO_GW_NUM_NET; id++)
		net[id].handle = IO_HANDLE_ERROR;
	co_unsigned16_t num_net = 0;

	opterr = 0;
	optind = 1;
	while (optind < argc) {
		char *arg = argv[optind];
		if (*arg != '-') {
			optind++;
			if (num_net < CO_GW_NUM_NET) {
				if (!net[num_net].can_path)
					net[num_net].can_path = arg;
				else
					net[num_net++].dcf_path = arg;
			} else {
				diag(DIAG_ERROR, 0,
						"at most %d CAN networks are supported",
						CO_GW_NUM_NET);
			}
		} else if (*++arg == '-') {
			optind++;
			if (!*++arg)
				break;
			if (!strcmp(arg, "exit")) {
				flags |= FLAG_EXIT;
			} else if (!strcmp(arg, "help")) {
				flags |= FLAG_HELP;
			} else if (!strncmp(arg, "inhibit=", 8)) {
				inhibit = atoi(arg + 8);
			} else if (!strcmp(arg, "monitor")) {
				flags |= FLAG_MONITOR;
			} else if (!strcmp(arg, "no-wait")) {
				flags |= FLAG_NO_WAIT;
			} else {
				diag(DIAG_ERROR, 0, "illegal option -- %s",
						arg);
			}
		} else {
			int c = getopt(argc, argv, ":ehi:mW");
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
			case 'e': flags |= FLAG_EXIT; break;
			case 'h': flags |= FLAG_HELP; break;
			case 'i': inhibit = atoi(optarg); break;
			case 'm': flags |= FLAG_MONITOR; break;
			case 'W': flags |= FLAG_NO_WAIT; break;
			}
		}
	}
	for (; optind < argc; optind++) {
		if (num_net < CO_GW_NUM_NET) {
			if (!net[num_net].can_path)
				net[num_net].can_path = argv[optind];
			else
				net[num_net++].dcf_path = argv[optind];
		} else {
			diag(DIAG_ERROR, 0,
					"at most %d CAN networks are supported",
					CO_GW_NUM_NET);
		}
	}

	if (flags & FLAG_HELP) {
		diag(DIAG_INFO, 0, "%s", HELP);
		return EXIT_SUCCESS;
	}

	if (!num_net) {
		diag(DIAG_ERROR, 0, "no CANopen networks specified");
		goto error_arg;
	}

	poll = io_poll_create();
	if (!poll) {
		diag(DIAG_ERROR, get_errc(),
				"unable to create I/O polling interface");
		goto error_create_poll;
	}
	struct io_event event = IO_EVENT_INIT;

	struct timespec now = { 0, 0 };

	for (co_unsigned16_t id = 1; id <= num_net; id++) {
		if (!net[id - 1].can_path || !net[id - 1].dcf_path)
			break;
		// Create a non-blocking CAN device handle.
		net[id - 1].handle = io_open_can(net[id - 1].can_path);
		if (net[id - 1].handle == IO_HANDLE_ERROR) {
			diag(DIAG_ERROR, get_errc(),
					"%s is not a suitable CAN device",
					net[id - 1].can_path);
			goto error_net;
		}
		if (io_set_flags(net[id - 1].handle, IO_FLAG_NONBLOCK) == -1) {
			diag(DIAG_ERROR, get_errc(),
					"unable to set CAN device flags");
			goto error_net;
		}
		// Watch the CAN device for incoming frames.
		event.events = IO_EVENT_READ;
		event.u.data = &net[id - 1];
		if (io_poll_watch(poll, net[id - 1].handle, &event, 1) == -1) {
			diag(DIAG_ERROR, get_errc(),
					"unable to watch CAN device");
			goto error_net;
		}
		net[id - 1].st = io_can_get_state(net[id - 1].handle);
		// Create a CAN network object.
		net[id - 1].net = can_net_create();
		if (!net[id - 1].net) {
			diag(DIAG_ERROR, get_errc(),
					"unable to create CAN network interface");
			goto error_net;
		}
		can_net_set_send_func(net[id - 1].net, &can_send,
				(void *)net[id - 1].handle);
		// Set the current network time.
		timespec_get(&now, TIME_UTC);
		can_net_set_time(net[id - 1].net, &now);
		// Load the EDS/DCF from file.
		net[id - 1].dev = co_dev_create_from_dcf_file(
				net[id - 1].dcf_path);
		if (!net[id - 1].dev)
			goto error_net;
		// Create the NMT service.
		net[id - 1].nmt =
				co_nmt_create(net[id - 1].net, net[id - 1].dev);
		if (!net[id - 1].nmt) {
			diag(DIAG_ERROR, get_errc(),
					"unable to create NMT service");
		}
	}

	co_gw_t *gw = co_gw_create();
	if (!gw) {
		diag(DIAG_ERROR, 0, "unable to create gateway");
		goto error_create_gw;
	}

	for (co_unsigned16_t id = 1; id <= num_net; id++) {
		if (!net[id - 1].nmt)
			break;
		if (co_gw_init_net(gw, id, net[id - 1].nmt) == -1) {
			diag(DIAG_ERROR, get_errc(),
					"unable to initialize CANopen network");
		}
	}

	co_gw_txt_t *gw_txt = co_gw_txt_create();
	if (!gw_txt) {
		diag(DIAG_ERROR, 0, "unable to create gateway");
		goto error_create_gw_txt;
	}

	co_gw_set_send_func(gw, &gw_send, gw_txt);
	co_gw_set_rate_func(gw, &gw_rate, NULL);
	co_gw_txt_set_recv_func(gw_txt, &gw_txt_recv, NULL);
	co_gw_txt_set_send_func(gw_txt, &gw_txt_send, gw);

	mtx_init(&recv_mtx, mtx_plain);
	cnd_init(&wait_cond);
	wait = 0;
	status = EXIT_SUCCESS;

	mtx_init(&recv_mtx, mtx_plain);
	recv_buf = NULL;

	mtx_init(&send_mtx, mtx_plain);
	cnd_init(&send_cond);
	send_buf = NULL;

	thrd_t thr;
	if (thrd_create(&thr, &io_thrd_start, gw_txt) != thrd_success) {
		diag(DIAG_ERROR, 0, "unable to create thread");
		goto error_create_thr;
	}

	// Determine whether stdin is an interactive terminal.
	int errsv = errno;
#if _WIN32
	int tty = _isatty(_fileno(stdin));
#elif _POSIX_C_SOURCE >= 200112L
	int tty = isatty(fileno(stdin));
#else
	int tty = 1;
#endif
	errno = errsv;
	int eof = 0;

	char *line = NULL;
	size_t n = 0;
	co_unsigned32_t seq = 1;
	char *cmd = NULL;
	while (!done) {
		if (!(flags & FLAG_NO_WAIT) || (flags & FLAG_EXIT)) {
			mtx_lock(&wait_mtx);
			if (!(flags & FLAG_NO_WAIT)) {
				// Wait for all pending requests to complete.
				while (wait && !((flags & FLAG_EXIT) && status))
					cnd_wait(&wait_cond, &wait_mtx);
			}
			if ((flags & FLAG_EXIT) && status) {
				mtx_unlock(&wait_mtx);
				break;
			}
			mtx_unlock(&wait_mtx);
		}
		// Do not print a confirmation or indication while the user is
		// entering a command.
		if (!tty || !cmd) {
			mtx_lock(&send_mtx);
			// In monitor mode, wait for the next indication.
			if (eof && !send_buf)
				cnd_wait(&send_cond, &send_mtx);
			char *buf = send_buf;
			send_buf = NULL;
			mtx_unlock(&send_mtx);
			if (buf) {
				printf("%s", buf);
				free(buf);
			}
		}
		// In monitor mode, keep printing indications on EOF, but do not
		// try to parse any more commands.
		if (eof)
			continue;
		// Only print the sequence number (or continuation ellipsis) on
		// an interactive terminal.
		if (tty) {
			if (cmd)
				printf("... ");
			else
				printf("[%u] ", seq);
			fflush(stdout);
		}
		// Keep reading lines until end-of-file.
		if (getline(&line, &n, stdin) == -1) {
			if (tty)
				fputc('\n', stdout);
			if (ferror(stdin)) {
				diag(DIAG_ERROR, errno2c(errno),
						"error reading from stdin");
				// Abort on a stream error.
				break;
			}
			if (feof(stdin)) {
				// Disable the interactive terminal on EOF.
				tty = 0;
				eof = 1;
				if (flags & FLAG_MONITOR) {
					// In monitor mode, we exit on receipt
					// of SIGINT or SIGTERM instead of EOF.
					signal(SIGINT, &sig_done);
					signal(SIGTERM, &sig_done);
#if _POSIX_C_SOURCE >= 200112L
					signal(SIGQUIT, &sig_done);
#endif
				} else {
					done = 1;
				}
			}
			// Skip parsing on error.
			continue;
		}
		// Ignore empty lines and comments.
		char *cp = line;
		cp += lex_ctype(&isblank, cp, NULL, NULL);
		cp += lex_line_comment("#", cp, NULL, NULL);
		if (lex_break(cp, NULL, NULL))
			continue;
		// If a line ends with a continuation character, buffer it.
		cp = line + strlen(line);
		while (isspace(*--cp))
			;
		if (*cp == '\\') {
			*cp = '\0';
			if (cmd) {
				char *tmp;
				if (asprintf(&tmp, "%s%s", cmd, line) == -1)
					break;
				free(cmd);
				cmd = tmp;
			} else {
				if (asprintf(&cmd, "[%u] %s", seq, line)
						== -1) {
					cmd = NULL;
					break;
				}
			}
		} else {
			if (!(flags & FLAG_NO_WAIT)) {
				mtx_lock(&wait_mtx);
				wait = 1;
				mtx_unlock(&wait_mtx);
			}

			// Append the command to the receive buffer.
			mtx_lock(&recv_mtx);
			int result;
			if (recv_buf) {
				char *tmp;
				if (cmd)
					result = asprintf(&tmp, "%s%s%s",
							recv_buf, cmd, line);
				else
					result = asprintf(&tmp, "%s[%u] %s",
							recv_buf, seq, line);
				if (result < 0) {
					mtx_unlock(&recv_mtx);
					break;
				}
				free(recv_buf);
				recv_buf = tmp;
			} else {
				if (cmd)
					result = asprintf(&recv_buf, "%s%s",
							cmd, line);
				else
					result = asprintf(&recv_buf, "[%u] %s",
							seq, line);
				if (result < 0) {
					recv_buf = NULL;
					mtx_unlock(&recv_mtx);
					break;
				}
			}
			mtx_unlock(&recv_mtx);

			seq++;
			free(cmd);
			cmd = NULL;

			// Signal the I/O thread and wait for one poll cycle.
			io_poll_signal(poll, 0);
			const struct timespec duration = { POLL_TIMEOUT / 1000,
				(POLL_TIMEOUT % 1000) * 1000000 };
			thrd_sleep(&duration, NULL);
		}
	}
	free(cmd);
	free(line);

	io_poll_signal(poll, 1);
	thrd_join(thr, NULL);

	free(send_buf);
	cnd_destroy(&send_cond);
	mtx_destroy(&send_mtx);

	free(recv_buf);
	mtx_destroy(&recv_mtx);

	cnd_destroy(&wait_cond);
	mtx_destroy(&wait_mtx);

	co_gw_txt_destroy(gw_txt);
	co_gw_destroy(gw);

	while (num_net--) {
		co_nmt_destroy(net[num_net].nmt);
		co_dev_destroy(net[num_net].dev);
		can_net_destroy(net[num_net].net);
		io_close(net[num_net].handle);
	}

	io_poll_destroy(poll);

	return status;

error_create_thr:
	free(send_buf);
	cnd_destroy(&send_cond);
	mtx_destroy(&send_mtx);
	free(recv_buf);
	mtx_destroy(&recv_mtx);
	cnd_destroy(&wait_cond);
	mtx_destroy(&wait_mtx);
	co_gw_txt_destroy(gw_txt);
error_create_gw_txt:
	co_gw_destroy(gw);
error_create_gw:
error_net:
	while (num_net--) {
		co_nmt_destroy(net[num_net].nmt);
		co_dev_destroy(net[num_net].dev);
		can_net_destroy(net[num_net].net);
		io_close(net[num_net].handle);
	}
	io_poll_destroy(poll);
error_create_poll:
error_arg:
	return EXIT_FAILURE;
}

int
can_send(const struct can_msg *msg, void *data)
{
	io_handle_t handle = (io_handle_t)data;

	return io_can_write(handle, msg) == 1 ? 0 : -1;
}

int
gw_send(const struct co_gw_srv *srv, void *data)
{
	co_gw_txt_t *gw = data;
	assert(gw);

	return co_gw_txt_recv(gw, srv);
}

void
gw_rate(co_unsigned16_t id, co_unsigned16_t rate, void *data)
{
	assert(id && id <= CO_GW_NUM_NET);
	uint32_t bitrate = rate * 1000;
	(void)data;

	if (!net[id - 1].handle || !bitrate)
		return;

	if (io_can_set_bitrate(net[id - 1].handle, bitrate) == -1)
		diag(DIAG_ERROR, 0, "unable to set bitrate of %s to %u bit/s",
				net[id - 1].can_path, bitrate);
}

int
gw_txt_recv(const char *txt, void *data)
{
	(void)data;

	mtx_lock(&send_mtx);
	if (send_buf) {
		char *buf = NULL;
		if (asprintf(&buf, "%s%s\n", send_buf, txt) == -1) {
			mtx_unlock(&send_mtx);
			return -1;
		}
		free(send_buf);
		send_buf = buf;
	} else if (asprintf(&send_buf, "%s\n", txt) == -1) {
		mtx_unlock(&send_mtx);
		return -1;
	}
	cnd_signal(&send_cond);
	mtx_unlock(&send_mtx);

	return 0;
}

int
gw_txt_send(const struct co_gw_req *req, void *data)
{
	co_gw_t *gw_txt = data;
	assert(gw_txt);

	return co_gw_recv(gw_txt, req);
}

void
sig_done(int sig)
{
	(void)sig;

	done = 1;

	// Wake up the wait for pending requests.
	mtx_lock(&wait_mtx);
	wait = 0;
	cnd_signal(&wait_cond);
	mtx_unlock(&wait_mtx);

	// Wake up the wait for indications in monitor mode.
	mtx_lock(&send_mtx);
	cnd_signal(&send_cond);
	mtx_unlock(&send_mtx);
}

int
io_thrd_start(void *arg)
{
	co_gw_txt_t *gw = arg;
	assert(gw);

	struct timespec last = { 0, 0 };
	char *buf = NULL;
	char *end = NULL;
	char *cp = NULL;
	struct floc at = { "<stdin>", 1, 1 };
	for (;;) {
		if (flags & FLAG_EXIT) {
			int iec = co_gw_txt_iec(gw);
			if (iec) {
				mtx_lock(&wait_mtx);
				status = iec;
				if (!(flags & FLAG_NO_WAIT)) {
					wait = 0;
					cnd_signal(&wait_cond);
				}
				mtx_unlock(&wait_mtx);
			}
		}
		if (!buf) {
			mtx_lock(&recv_mtx);
			buf = recv_buf;
			recv_buf = NULL;
			mtx_unlock(&recv_mtx);
			if (buf) {
				end = buf + strlen(buf) + 1;
				cp = buf;
			} else if (!(flags & FLAG_NO_WAIT)) {
				// Signal the main thread if no pending request
				// remain.
				mtx_lock(&wait_mtx);
				if (wait) {
					wait = !!co_gw_txt_pending(gw);
					if (!wait)
						cnd_signal(&wait_cond);
				}
				mtx_unlock(&wait_mtx);
			}
		}
		// Update the CAN network time.
		struct timespec now = { 0, 0 };
		timespec_get(&now, TIME_UTC);
		for (co_unsigned16_t id = 1; id <= CO_GW_NUM_NET; id++) {
			if (!net[id - 1].net)
				break;
			can_net_set_time(net[id - 1].net, &now);
			timespec_get(&now, TIME_UTC);
		}
		// Process user requests.
		// clang-format off
		if (buf && (!inhibit || timespec_diff_msec(&now, &last)
				>= inhibit)) {
			// clang-format on
			size_t chars = co_gw_txt_send(gw, cp, end, &at);
			if (chars) {
				cp += chars;
				timespec_get(&last, TIME_UTC);
			} else {
				free(buf);
				buf = end = cp = NULL;
			}
		}
		// Process incoming CAN frames.
		struct io_event event = IO_EVENT_INIT;
		int n = io_poll_wait(poll, 1, &event, POLL_TIMEOUT);
		if (n != 1)
			continue;
		if (event.events == IO_EVENT_SIGNAL) {
			if (event.u.sig)
				break;
		} else if (event.events & IO_EVENT_READ) {
			struct co_net *net = event.u.data;
			assert(net);
			int result;
			struct can_msg msg = CAN_MSG_INIT;
			while ((result = io_can_read(net->handle, &msg)) == 1)
				can_net_recv(net->net, &msg);
			// Treat the reception of an error frame, or any error
			// other than an empty receive buffer, as an error
			// event.
			// clang-format off
			if (!result || (result == -1
					&& get_errnum() != ERRNUM_AGAIN
					&& get_errnum() != ERRNUM_WOULDBLOCK))
				// clang-format on
				event.events |= IO_EVENT_ERROR;
		}
		if (net->st == CAN_STATE_BUSOFF
				|| (event.events & IO_EVENT_ERROR))
			co_net_err(event.u.data);
	}
	free(buf);

	return 0;
}

void
co_net_err(struct co_net *net)
{
	assert(net);

	int st = io_can_get_state(net->handle);
	if (st != net->st) {
		if (net->st == CAN_STATE_BUSOFF)
			// Recovered from bus off.
			co_nmt_on_err(net->nmt, 0x8140, 0x10, NULL);
		else if (st == CAN_STATE_PASSIVE)
			// CAN in error passive mode.
			co_nmt_on_err(net->nmt, 0x8120, 0x10, NULL);
		net->st = st;
	}
}
