/**@file
 * This file contains the CANopen cat daemon. It provides an implementation of
 * object 1026 (OS prompt) by connecting the StdIn, StdOut and StdErr
 * sub-objects to `stdin`, `stdout` and `stderr` of a user-specified process.
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
#include <lely/co/nmt.h>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>
#include <lely/co/tpdo.h>
#include <lely/co/val.h>
#include <lely/io/can.h>
#include <lely/io/pipe.h>
#include <lely/io/poll.h>
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
	"Arguments: [options...] <CAN interface> command\n" \
	"Options:\n" \
	"  -D, --no-daemon       Do not run as daemon\n" \
	"  -f <filename>, --file=<filename>\n" \
	"                        Use <filename> as the EDS/DCF file instead of\n" \
	"                        " COCATD_DCF ".\n" \
	"  -h, --help            Display this information\n" \
	"  -n <node-ID> --node=<node-ID>\n" \
	"                        Use <node-ID> as the CANopen node-ID"
// clang-format on

#define POLL_TIMEOUT 100

int daemon_init(int argc, char *argv[]);
void daemon_main();
void daemon_fini();
void daemon_handler(int sig, void *handle);

int can_send(const struct can_msg *msg, void *data);
int can_next(const struct timespec *tp, void *data);
int can_timer(const struct timespec *tp, void *data);

void can_err(io_handle_t handle, int *pst, co_nmt_t *nmt);

co_unsigned32_t co_1026_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);
co_unsigned32_t co_1026_up_ind(
		const co_sub_t *sub, struct co_sdo_req *req, void *data);

#define FLAG_HELP 0x01
#define FLAG_NO_DAEMON 0x02

int flags;
io_poll_t *poll;
io_handle_t hcan = IO_HANDLE_ERROR;
can_net_t *net;
co_dev_t *dev;
co_nmt_t *nmt;
io_handle_t hin = IO_HANDLE_ERROR;
io_handle_t hout = IO_HANDLE_ERROR;
io_handle_t herr = IO_HANDLE_ERROR;

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
				? EXIT_FAILURE : EXIT_SUCCESS;
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

	const char *filename = COCATD_DCF;
	co_unsigned8_t id = 0;
	const char *ifname = NULL;
	const char *command = NULL;

	opterr = 0;
	optind = 1;
	int optpos = 0;
	while (optind < argc) {
		char *arg = argv[optind];
		if (*arg != '-') {
			optind++;
			switch (optpos++) {
			case 0: ifname = arg; break;
			case 1: command = arg; break;
			default:
				diag(DIAG_ERROR, 0, "extra argument %s", arg);
				break;
			}
		} else if (*++arg == '-') {
			optind++;
			if (!*++arg)
				break;
			if (!strcmp(arg, "no-daemon")) {
			} else if (!strncmp(arg, "file=", 5)) {
				filename = arg + 5;
			} else if (!strcmp(arg, "help")) {
			} else if (!strncmp(arg, "node=", 5)) {
				id = strtoul(arg + 5, NULL, 0);
			} else {
				diag(DIAG_ERROR, 0, "illegal option -- %s",
						arg);
			}
		} else {
			int c = getopt(argc, argv, ":Df:hn:");
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
			case 'D': break;
			case 'f': filename = optarg; break;
			case 'h': break;
			case 'n': id = strtoul(optarg, NULL, 0); break;
			}
		}
	}
	for (char *arg = argv[optind]; optind < argc; arg = argv[++optind]) {
		switch (optpos++) {
		case 0: ifname = arg; break;
		case 1: command = arg; break;
		default: diag(DIAG_ERROR, 0, "extra argument %s", arg); break;
		}
	}

	if (optpos < 1 || !ifname) {
		diag(DIAG_ERROR, 0, "no CAN interface specified");
		goto error_arg;
	}

	if (optpos < 2 || !command) {
		diag(DIAG_ERROR, 0, "no command specified");
		goto error_arg;
	}

	poll = io_poll_create();
	if (!poll) {
		diag(DIAG_ERROR, get_errc(),
				"unable to create I/O polling interface");
		goto error_create_poll;
	}

	hcan = io_open_can(ifname);
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
	can_net_set_time(net, &now);

	dev = co_dev_create_from_dcf_file(filename);
	if (!dev)
		goto error_create_dev;
	if (id)
		co_dev_set_id(dev, id);

	nmt = co_nmt_create(net, dev);
	if (!nmt) {
		diag(DIAG_ERROR, get_errc(), "unable to create NMT service");
		goto error_create_nmt;
	}

	io_handle_t hvin[2] = { IO_HANDLE_ERROR, IO_HANDLE_ERROR };
	if (io_open_pipe(hvin) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to open pipe");
		goto error_open_hvin;
	}

	io_handle_t hvout[2] = { IO_HANDLE_ERROR, IO_HANDLE_ERROR };
	if (io_open_pipe(hvout) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to open pipe");
		goto error_open_hvout;
	}

	io_handle_t hverr[2] = { IO_HANDLE_ERROR, IO_HANDLE_ERROR };
	if (io_open_pipe(hverr) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to open pipe");
		goto error_open_hverr;
	}

	pid_t pid = fork();
	if (pid == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to fork process");
		goto error_fork;
	}

	if (!pid) {
		hin = hvin[0];
		io_close(hvin[1]);

		io_close(hvout[0]);
		hout = hvout[1];

		io_close(hverr[0]);
		herr = hverr[1];

		fclose(stdin);
		fclose(stdout);
		fclose(stderr);

		int result;

		do
			result = dup2(io_get_fd(hin), STDIN_FILENO);
		while (result == -1 && errno == EINTR);
		if (result == -1)
			goto error_dup_hin;
		io_close(hin);

		do
			result = dup2(io_get_fd(hout), STDOUT_FILENO);
		while (result == -1 && errno == EINTR);
		if (result == -1)
			goto error_dup_hout;
		io_close(hout);

		do
			result = dup2(io_get_fd(herr), STDERR_FILENO);
		while (result == -1 && errno == EINTR);
		if (result == -1)
			goto error_dup_herr;
		io_close(herr);

		int stat_val = system(command);
		if (!WIFEXITED(stat_val))
			goto error_system;

		// Propagate the exit status from the command.
		exit(WEXITSTATUS(stat_val));

	error_dup_hin:
		io_close(hin);
	error_dup_hout:
		io_close(hout);
	error_dup_herr:
		io_close(herr);
	error_system:
		exit(EXIT_FAILURE);
	}

	io_close(hvin[0]);
	hvin[0] = IO_HANDLE_ERROR;
	hin = hvin[1];

	if (io_set_flags(hin, IO_FLAG_NONBLOCK) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to configure pipe");
		goto error_set_flags_hin;
	}

	hout = hvout[0];
	io_close(hvout[1]);
	hvout[1] = IO_HANDLE_ERROR;

	if (io_set_flags(hout, IO_FLAG_NONBLOCK) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to configure pipe");
		goto error_set_flags_hout;
	}

	herr = hverr[0];
	io_close(hverr[1]);
	hverr[1] = IO_HANDLE_ERROR;

	if (io_set_flags(herr, IO_FLAG_NONBLOCK) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to configure pipe");
		goto error_set_flags_herr;
	}

	co_sub_t *sub_1026_01 = co_dev_find_sub(dev, 0x1026, 0x01);
	if (!sub_1026_01) {
		diag(DIAG_ERROR, 0,
				"sub-object 1026:01 not found in object dictionary");
		goto error_find_sub_1026_01;
	}
	co_sub_set_dn_ind(sub_1026_01, &co_1026_dn_ind, (void *)hin);

	co_sub_t *sub_1026_02 = co_dev_find_sub(dev, 0x1026, 0x02);
	if (!sub_1026_02) {
		diag(DIAG_ERROR, 0,
				"sub-object 1026:02 not found in object dictionary");
		goto error_find_sub_1026_02;
	}
	co_sub_set_up_ind(sub_1026_02, &co_1026_up_ind, (void *)hout);

	co_sub_t *sub_1026_03 = co_dev_find_sub(dev, 0x1026, 0x03);
	if (!sub_1026_03) {
		diag(DIAG_ERROR, 0,
				"sub-object 1026:01 not found in object dictionary");
		goto error_find_sub_1026_03;
	}
	co_sub_set_up_ind(sub_1026_03, &co_1026_up_ind, (void *)herr);

	co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);

	daemon_set_handler(&daemon_handler, poll);

	return 0;

error_find_sub_1026_03:
error_find_sub_1026_02:
error_find_sub_1026_01:
error_set_flags_herr:
error_set_flags_hout:
error_set_flags_hin:
error_fork:
	io_close(hverr[1]);
	io_close(hverr[0]);
error_open_hverr:
	io_close(hvout[1]);
	io_close(hvout[0]);
error_open_hvout:
	io_close(hvin[1]);
	io_close(hvin[0]);
error_open_hvin:
	co_nmt_destroy(nmt);
	nmt = NULL;
error_create_nmt:
	co_dev_destroy(dev);
	dev = NULL;
error_create_dev:
	can_net_destroy(net);
	net = NULL;
error_create_net:
error_watch_can:
error_set_flags_can:
	io_close(hcan);
	hcan = IO_HANDLE_ERROR;
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
	can_net_set_next_func(net, &can_next, &next);

	int watch_out = 1;
	can_timer_t *timer_out = can_timer_create();
	if (!timer_out) {
		diag(DIAG_ERROR, get_errc(), "unable to create timer");
		goto error_create_timer_out;
	}
	can_timer_set_func(timer_out, &can_timer, &watch_out);

	int watch_err = 1;
	can_timer_t *timer_err = can_timer_create();
	if (!timer_err) {
		diag(DIAG_ERROR, get_errc(), "unable to create timer");
		goto error_create_timer_err;
	}
	can_timer_set_func(timer_err, &can_timer, &watch_err);

	struct io_event event;
	int st = CAN_STATE_ACTIVE;

	for (;;) {
		// Only watch hout when PDO 1 is active.
		co_tpdo_t *pdo_out = co_nmt_get_tpdo(nmt, 1);
		if (watch_out && pdo_out) {
			event.events = IO_EVENT_READ;
			event.u.handle = hout;
			watch_out = !!io_poll_watch(poll, hout, &event, 0);
		}
		// Only watch herr when PDO 1 is active.
		co_tpdo_t *pdo_err = co_nmt_get_tpdo(nmt, 2);
		if (watch_err && pdo_err) {
			event.events = IO_EVENT_READ;
			event.u.handle = herr;
			watch_err = !!io_poll_watch(poll, herr, &event, 0);
		}
		// Limit the timeout to sensible values.
		int64_t timeout = timespec_diff_msec(&next, &now);
		timeout = MAX(0, MIN(POLL_TIMEOUT, timeout));
		// Wait for a single I/O event.
		int n = io_poll_wait(poll, 1, &event, timeout);
		// Update the current time and the next timeout. Fix the timeout
		// if it lies in the past.
		timespec_get(&now, TIME_UTC);
		can_net_set_time(net, &now);
		if (timespec_cmp(&next, &now) < 0) {
			next = now;
			timespec_add_msec(&next, POLL_TIMEOUT);
		}
		// Handle I/O events after updating the current time, so any
		// callbacks can obtain the correct time from the CAN network.
		if (n != 1)
			continue;
		if (event.events == IO_EVENT_SIGNAL) {
			switch (event.u.sig) {
			case DAEMON_STOP: goto done;
			case DAEMON_PAUSE:
				io_poll_watch(poll, hcan, NULL, 0);

				io_poll_watch(poll, hout, NULL, 0);
				can_timer_stop(timer_out);
				watch_out = 0;

				io_poll_watch(poll, herr, NULL, 0);
				can_timer_stop(timer_err);
				watch_err = 0;

				daemon_status(DAEMON_PAUSE);
				break;
			case DAEMON_CONTINUE:
				event.events = IO_EVENT_READ;
				event.u.handle = hcan;
				io_poll_watch(poll, hcan, &event, 1);

				watch_out = 1;
				watch_err = 1;

				daemon_status(DAEMON_CONTINUE);
				break;
			}
		} else if (event.events & IO_EVENT_READ) {
			if (event.u.handle == hcan) {
				int result;
				struct can_msg msg = CAN_MSG_INIT;
				while ((result = io_can_read(hcan, &msg)) == 1)
					can_net_recv(net, &msg);
				// Treat the reception of an error frame, or any
				// error other than an empty receive buffer, as
				// an error event.
				// clang-format off
				if (!result || (result == -1
						&& get_errnum() != ERRNUM_AGAIN
						&& get_errnum() != ERRNUM_WOULDBLOCK))
					// clang-format on
					event.events |= IO_EVENT_ERROR;
			} else if (event.u.handle == hout) {
				co_tpdo_event(pdo_out);
				// Wait for the inhibit time to elapse before
				// watching hout.
				struct timespec start = { 0, 0 };
				co_tpdo_get_next(pdo_out, &start);
				can_timer_start(timer_out, net, &start, NULL);
			} else if (event.u.handle == herr) {
				co_tpdo_event(pdo_err);
				// Wait for the inhibit time to elapse before
				// watching herr.
				struct timespec start = { 0, 0 };
				co_tpdo_get_next(pdo_err, &start);
				can_timer_start(timer_err, net, &start, NULL);
			}
		}
		// clang-format off
		if (event.u.handle == hcan && (st == CAN_STATE_BUSOFF
				|| (event.events & IO_EVENT_ERROR)))
			// clang-format on
			can_err(hcan, &st, nmt);
	}

done:
	can_timer_destroy(timer_err);
error_create_timer_err:
	can_timer_destroy(timer_out);
error_create_timer_out:
	can_net_set_next_func(net, NULL, NULL);
}

void
daemon_fini()
{
	io_close(herr);
	herr = IO_HANDLE_ERROR;

	io_close(hout);
	hout = IO_HANDLE_ERROR;

	io_close(hin);
	hin = IO_HANDLE_ERROR;

	co_nmt_destroy(nmt);
	nmt = NULL;

	co_dev_destroy(dev);
	dev = NULL;

	can_net_destroy(net);
	net = NULL;

	io_close(hcan);
	hcan = IO_HANDLE_ERROR;

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

int
can_send(const struct can_msg *msg, void *data)
{
	io_handle_t handle = (io_handle_t)data;

	return io_can_write(handle, msg) == 1 ? 0 : -1;
}

int
can_next(const struct timespec *tp, void *data)
{
	assert(tp);
	struct timespec *next = data;
	assert(next);

	*next = *tp;

	return 0;
}

int
can_timer(const struct timespec *tp, void *data)
{
	(void)tp;
	int *pwatch = data;
	assert(pwatch);

	*pwatch = 1;

	return 0;
}

void
can_err(io_handle_t handle, int *pst, co_nmt_t *nmt)
{
	assert(pst);
	assert(nmt);

	int st = io_can_get_state(handle);
	if (st != *pst) {
		if (*pst == CAN_STATE_BUSOFF)
			// Recovered from bus off.
			co_nmt_on_err(nmt, 0x8140, 0x10, NULL);
		else if (st == CAN_STATE_PASSIVE)
			// CAN in error passive mode.
			co_nmt_on_err(nmt, 0x8120, 0x10, NULL);
		*pst = st;
	}
}

co_unsigned32_t
co_1026_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	(void)sub;
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1026);
	assert(co_sub_get_subidx(sub) == 0x01);
	assert(co_sub_get_type(sub) == CO_DEFTYPE_UNSIGNED8);
	assert(req);
	io_handle_t handle = (io_handle_t)data;

	co_unsigned8_t val = 0;

	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, CO_DEFTYPE_UNSIGNED8, &val, &ac) == -1)
		return ac;

	if (io_write(handle, &val, 1) != 1)
		return CO_SDO_AC_DATA;

	return 0;
}

co_unsigned32_t
co_1026_up_ind(const co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	(void)sub;
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1026);
	assert(co_sub_get_subidx(sub) == 0x02
			|| co_sub_get_subidx(sub) == 0x03);
	assert(co_sub_get_type(sub) == CO_DEFTYPE_UNSIGNED8);
	assert(req);
	io_handle_t handle = (io_handle_t)data;

	co_unsigned8_t val = 0;
	if (io_read(handle, &val, 1) != 1)
		return CO_SDO_AC_NO_DATA;

	co_unsigned32_t ac = 0;
	co_sdo_req_up_val(req, CO_DEFTYPE_UNSIGNED8, &val, &ac);
	return ac;
}
