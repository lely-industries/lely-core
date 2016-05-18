#ifndef LELY_CO_TEST_TEST_H
#define LELY_CO_TEST_TEST_H

#include <lely/tap/tap.h>
#include <lely/can/buf.h>
#include <lely/can/net.h>
#ifndef LELY_CO_NO_WTM
#include <lely/co/wtm.h>
#endif

#ifndef CO_TEST_BUFSIZE
#define CO_TEST_BUFSIZE	256
#endif

struct co_test {
	can_net_t *net;
#ifndef LELY_CO_NO_WTM
	co_wtm_t *wtm;
#endif
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

static int co_test_recv(struct co_test *test, const struct can_msg *msg);
static int co_test_send(const struct can_msg *msg, void *data);
#ifndef LELY_CO_NO_WTM
static int co_test_wtm_recv(co_wtm_t *wtm, uint8_t nif,
		const struct timespec *tp, const struct can_msg *msg,
		void *data);
static int co_test_wtm_send(co_wtm_t *wtm, const void *buf, size_t nbytes,
		void *data);
#endif

static void
co_test_init(struct co_test *test, can_net_t *net)
{
	tap_assert(test);
	tap_assert(net);

	test->net = net;
	can_net_set_send_func(test->net, &co_test_send, test);

	tap_assert(!can_buf_init(&test->buf, CO_TEST_BUFSIZE));

#ifndef LELY_CO_NO_WTM
	test->wtm = co_wtm_create();
	tap_assert(test->wtm);
	co_wtm_set_send_func(test->wtm, &co_test_wtm_send, NULL);
	co_wtm_set_recv_func(test->wtm, &co_test_wtm_recv, test);
#endif

	test->done = 0;

	co_test_step(test);
}

static void
co_test_fini(struct co_test *test)
{
	tap_assert(test);

#ifndef LELY_CO_NO_WTM
	co_wtm_destroy(test->wtm);
#endif

	can_buf_fini(&test->buf);
}

static void
co_test_step(struct co_test *test)
{
	tap_assert(test);

#ifndef LELY_CO_NO_WTM
	co_wtm_flush(test->wtm);
#endif

	struct timespec now = { 0, 0 };
	timespec_get(&now, TIME_UTC);
	can_net_set_time(test->net, &now);

#ifndef LELY_CO_NO_WTM
	co_wtm_flush(test->wtm);
#endif

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
co_test_recv(struct co_test *test, const struct can_msg *msg)
{
	tap_assert(test);

	return can_buf_write(&test->buf, msg, 1) ? 0 : -1;
}

static int
co_test_send(const struct can_msg *msg, void *data)
{
	struct co_test *test = data;
	tap_assert(test);

#ifdef LELY_CO_NO_WTM
	return co_test_recv(test, msg);
#else
	struct timespec now = { 0, 0 };
	can_net_get_time(test->net, &now);
	co_wtm_set_time(test->wtm, 1, &now);
	return co_wtm_send(test->wtm, 1, msg);
#endif
}

#ifndef LELY_CO_NO_WTM

static int
co_test_wtm_recv(co_wtm_t *wtm, uint8_t nif, const struct timespec *tp,
		const struct can_msg *msg, void *data)
{
	__unused_var(wtm);
	__unused_var(nif);
	__unused_var(tp);
	struct co_test *test = data;
	tap_assert(test);

	return co_test_recv(test, msg);
}

static int
co_test_wtm_send(co_wtm_t *wtm, const void *buf, size_t nbytes, void *data)
{
	__unused_var(data);

	co_wtm_recv(wtm, buf, nbytes);

	return 0;
}

#endif

#ifdef __cplusplus
}
#endif

#endif

