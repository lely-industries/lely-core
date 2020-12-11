#ifndef LELY_TEST_INTERN_CO_TEST_H_
#define LELY_TEST_INTERN_CO_TEST_H_

#include "test.h"
#include <lely/can/buf.h>
#include <lely/can/net.h>
#include <lely/util/diag.h>
#if !LELY_NO_CO_WTM
#include <lely/co/wtm.h>
#endif

#include <stdlib.h>

#ifndef CO_TEST_BUFSIZE
#define CO_TEST_BUFSIZE 255
#endif

struct co_test {
	can_net_t *net;
#if !LELY_NO_CO_WTM
	co_wtm_t *wtm;
#endif
	struct can_buf buf;
	int wait;
	int done;
};

#ifdef __cplusplus
extern "C" {
#endif

#if !LELY_NO_STDIO && !LELY_NO_DIAG
static void co_test_diag_handler(void *handle, enum diag_severity severity,
		int errc, const char *format, va_list ap);
static void co_test_diag_at_handler(void *handle, enum diag_severity severity,
		int errc, const struct floc *at, const char *format,
		va_list ap);
#endif // !LELY_NO_STDIO && !LELY_NO_DIAG

static void co_test_init(struct co_test *test, can_net_t *net, int wait);
static void co_test_fini(struct co_test *test);

static void co_test_step(struct co_test *test);

static inline void co_test_wait(struct co_test *test);
static inline void co_test_done(struct co_test *test);

static int co_test_recv(struct co_test *test, const struct can_msg *msg);
static int co_test_send(const struct can_msg *msg, void *data);
#if !LELY_NO_CO_WTM
static int co_test_wtm_recv(co_wtm_t *wtm, uint_least8_t nif,
		const struct timespec *tp, const struct can_msg *msg,
		void *data);
static int co_test_wtm_send(
		co_wtm_t *wtm, const void *buf, size_t nbytes, void *data);
#endif

#if !LELY_NO_STDIO && !LELY_NO_DIAG

static void
co_test_diag_handler(void *handle, enum diag_severity severity, int errc,
		const char *format, va_list ap)
{
	co_test_diag_at_handler(handle, severity, errc, NULL, format, ap);
}

static void
co_test_diag_at_handler(void *handle, enum diag_severity severity, int errc,
		const struct floc *at, const char *format, va_list ap)
{
	(void)handle;

	int errsv = errno;
	char *s = NULL;
	if (vasprintf_diag_at(&s, severity, errc, at, format, ap) >= 0)
		tap_diag("%s", s);
	free(s);
	errno = errsv;

	if (severity == DIAG_FATAL)
		abort();
}

#else // LELY_NO_STDIO || LELY_NO_DIAG

static void
co_test_diag_handler(void *handle, enum diag_severity severity, int errc,
		const char *format, va_list ap)
{
	(void)handle;
	(void)severity;
	(void)errc;
	(void)format;
	(void)ap;
}

static void
co_test_diag_at_handler(void *handle, enum diag_severity severity, int errc,
		const struct floc *at, const char *format, va_list ap)
{
	(void)handle;
	(void)severity;
	(void)errc;
	(void)at;
	(void)format;
	(void)ap;
}

#endif // LELY_NO_STDIO || LELY_NO_DIAG

static void
co_test_init(struct co_test *test, can_net_t *net, int wait)
{
	tap_assert(test);
	tap_assert(net);

	test->net = net;
	can_net_set_send_func(test->net, &co_test_send, test);

	can_buf_init(&test->buf, NULL, 0);
	tap_assert(can_buf_reserve(&test->buf, CO_TEST_BUFSIZE)
			== CO_TEST_BUFSIZE);

#if !LELY_NO_CO_WTM
	test->wtm = co_wtm_create();
	tap_assert(test->wtm);
	co_wtm_set_send_func(test->wtm, &co_test_wtm_send, NULL);
	co_wtm_set_recv_func(test->wtm, &co_test_wtm_recv, test);
#endif

	test->wait = wait;
	test->done = 0;

	co_test_step(test);
}

static void
co_test_fini(struct co_test *test)
{
	tap_assert(test);

#if !LELY_NO_CO_WTM
	co_wtm_destroy(test->wtm);
#endif

	can_buf_fini(&test->buf);
}

static void
co_test_step(struct co_test *test)
{
	tap_assert(test);

#if !LELY_NO_CO_WTM
	co_wtm_flush(test->wtm);
#endif

	struct timespec now = { 0, 0 };
	timespec_get(&now, TIME_UTC);
	can_net_set_time(test->net, &now);

#if !LELY_NO_CO_WTM
	co_wtm_flush(test->wtm);
#endif

	struct can_msg msg;
	while (can_buf_read(&test->buf, &msg, 1)) {
		char s[72] = { 0 };
		snprintf_can_msg(s, sizeof(s), &msg);
		tap_diag("%s", s);

		can_net_recv(test->net, &msg);
	}

	if (test->wait > 0) {
		const struct timespec wait = { test->wait / 1000,
			(test->wait % 1000) * 1000000 };
		nanosleep(&wait, NULL);
	}
}

static inline void
co_test_wait(struct co_test *test)
{
	tap_assert(test);

	do
		co_test_step(test);
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

#if LELY_NO_CO_WTM
	return co_test_recv(test, msg);
#else
	struct timespec now = { 0, 0 };
	can_net_get_time(test->net, &now);
	co_wtm_set_time(test->wtm, 1, &now);
	return co_wtm_send(test->wtm, 1, msg);
#endif
}

#if !LELY_NO_CO_WTM

static int
co_test_wtm_recv(co_wtm_t *wtm, uint_least8_t nif, const struct timespec *tp,
		const struct can_msg *msg, void *data)
{
	(void)wtm;
	(void)nif;
	(void)tp;
	struct co_test *test = data;
	tap_assert(test);

	return co_test_recv(test, msg);
}

static int
co_test_wtm_send(co_wtm_t *wtm, const void *buf, size_t nbytes, void *data)
{
	(void)data;

	co_wtm_recv(wtm, buf, nbytes);

	return 0;
}

#endif

#ifdef __cplusplus
}
#endif

#endif // !LELY_TEST_INTERN_CO_TEST_H_
