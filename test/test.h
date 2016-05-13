#ifndef LELY_CO_TEST_TEST_H
#define LELY_CO_TEST_TEST_H

#include <lely/tap/tap.h>
#include <lely/can/buf.h>
#include <lely/can/net.h>

#ifndef CO_TEST_BUFSIZE
#define CO_TEST_BUFSIZE	256
#endif

struct co_test {
	can_net_t *net;
	struct can_buf buf;
	int done;
};

#ifdef __cplusplus
extern "C" {
#endif

static void co_test_init(struct co_test *test, can_net_t *net);
static void co_test_fini(struct co_test *test);

static void co_test_step(struct co_test *test);

static inline void co_test_wait(struct co_test *test);
static inline void co_test_done(struct co_test *test);

static int co_test_send(const struct can_msg *msg, void *data);

static void
co_test_init(struct co_test *test, can_net_t *net)
{
	tap_assert(test);
	tap_assert(net);

	tap_assert(!can_buf_init(&test->buf, CO_TEST_BUFSIZE));

	test->net = net;
	can_net_set_send_func(test->net, &co_test_send, test);

	test->done = 0;

	co_test_step(test);
}

static void
co_test_fini(struct co_test *test)
{
	tap_assert(test);

	can_buf_fini(&test->buf);
}

static void
co_test_step(struct co_test *test)
{
	tap_assert(test);

	struct timespec now = { 0, 0 };
	timespec_get(&now, TIME_UTC);
	can_net_set_time(test->net, &now);

	struct can_msg msg;
	while (can_buf_read(&test->buf, &msg, 1)) {
		char s[72] = { 0 };
		snprintf_can_msg(s, sizeof(s), &msg);
		tap_diag("%s", s);

		can_net_recv(test->net, &msg);
	}
}

static inline void
co_test_wait(struct co_test *test)
{
	tap_assert(test);

	do co_test_step(test);
	while (!test->done);
	test->done = 0;
}

static inline void
co_test_done(struct co_test *test)
{
	tap_assert(test);

	test->done = 1;
}

static int
co_test_send(const struct can_msg *msg, void *data)
{
	struct co_test *test = data;
	tap_assert(test);

	return can_buf_write(&test->buf, msg, 1) ? 0 : -1;
}

#ifdef __cplusplus
}
#endif

#endif

