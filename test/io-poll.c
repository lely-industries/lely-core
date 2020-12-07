#include "test.h"
#if _WIN32
#include <lely/io/sock.h>
#else
#include <lely/io/pipe.h>
#endif
#include <lely/io/poll.h>

#define TIMEOUT 1000

int
main(void)
{
	tap_plan(16);

	tap_assert(!lely_io_init());

	io_poll_t *poll = io_poll_create();
	tap_assert(poll);

	io_handle_t pipe[2];
#if _WIN32
	tap_test(!io_open_socketpair(IO_SOCK_IPV4, IO_SOCK_STREAM, pipe));
#else
	tap_test(!io_open_pipe(pipe));
#endif
	tap_test(!io_set_flags(pipe[0], IO_FLAG_NONBLOCK));
	tap_test(!io_set_flags(pipe[1], IO_FLAG_NONBLOCK));

	struct io_event event = IO_EVENT_INIT;

	int sig = 13;
	tap_test(!io_poll_signal(poll, sig));
	tap_test(io_poll_wait(poll, 1, &event, TIMEOUT) == 1);
	tap_test(event.events == IO_EVENT_SIGNAL);
	tap_test(event.u.sig == sig);

	struct io_event revent = { .events = IO_EVENT_READ,
		.u.handle = pipe[0] };
	tap_assert(!io_poll_watch(poll, pipe[0], &revent, 0));

	struct io_event sevent = { .events = IO_EVENT_WRITE,
		.u.handle = pipe[1] };
	tap_assert(!io_poll_watch(poll, pipe[1], &sevent, 0));

	tap_test(io_poll_wait(poll, 1, &event, TIMEOUT) == 1);
	tap_test(event.events == IO_EVENT_WRITE);
	tap_test(event.u.handle == pipe[1]);

	int sval = 13;
	tap_test(io_write(pipe[1], &sval, sizeof(sval)) == sizeof(sval));

	tap_test(io_poll_wait(poll, 1, &event, TIMEOUT) == 1);
	tap_test(event.events == IO_EVENT_READ);
	tap_test(event.u.handle == pipe[0]);

	int rval = 0;
	tap_test(io_read(pipe[0], &rval, sizeof(rval)) == sizeof(rval));

	tap_test(rval == sval);

	io_close(pipe[1]);
	io_close(pipe[0]);
	io_poll_destroy(poll);

	lely_io_fini();

	return 0;
}
