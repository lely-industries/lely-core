#include "co-test.h"
#include <lely/co/dcf.h>
#include <lely/co/obj.h>
#include <lely/co/time.h>

#define NUM_TEST 8
#define MSEC 100

void time_ind(co_time_t *time, const struct timespec *tp, void *data);

int
main(void)
{
	tap_plan(NUM_TEST);

#if !LELY_NO_STDIO && !LELY_NO_DIAG
	diag_set_handler(&co_test_diag_handler, NULL);
	diag_at_set_handler(&co_test_diag_at_handler, NULL);
#endif

	can_net_t *net = can_net_create();
	tap_assert(net);
	struct co_test test;
	co_test_init(&test, net, 0);

	co_dev_t *dev = co_dev_create_from_dcf_file(TEST_SRCDIR "/co-time.dcf");
	tap_assert(dev);
	co_time_t *time = co_time_create(net, dev);
	tap_assert(time);

	co_time_set_ind(time, &time_ind, &test);

	struct timespec interval = { 0, 1000000 * MSEC };
	co_time_start_prod(time, NULL, &interval);

	for (int i = 0; i < NUM_TEST; i++)
		co_test_wait(&test);

	co_time_destroy(time);
	co_dev_destroy(dev);

	co_test_fini(&test);
	can_net_destroy(net);

	return 0;
}

void
time_ind(co_time_t *time, const struct timespec *tp, void *data)
{
	(void)time;
	tap_assert(tp);
	struct co_test *test = data;

	tap_pass("received TIME [%ld.%09ld]", (long)tp->tv_sec, tp->tv_nsec);

	co_test_done(test);
}
