#include <lely/co/dcf.h>
#include <lely/co/obj.h>
#include <lely/co/sync.h>

#include "test.h"

#define NUM_TEST	8

void sync_ind(co_sync_t *sync, co_unsigned8_t cnt, void *data);

int
main(void)
{
	tap_plan(NUM_TEST);

	can_net_t *net = can_net_create();
	tap_assert(net);
	struct co_test test;
	co_test_init(&test, net);

	co_dev_t *dev = co_dev_create_from_dcf_file(TEST_SRCDIR "/sync.dcf");
	tap_assert(dev);
	co_sync_t *sync = co_sync_create(net, dev);
	tap_assert(sync);

	co_sync_set_ind(sync, &sync_ind, &test);

	for (int i = 0; i < NUM_TEST; i++)
		co_test_wait(&test);

	co_sync_destroy(sync);
	co_dev_destroy(dev);

	co_test_fini(&test);
	can_net_destroy(net);

	return 0;
}

void
sync_ind(co_sync_t *sync, co_unsigned8_t cnt, void *data)
{
	__unused_var(sync);
	struct co_test *test = data;

	tap_pass("received SYNC [%d]", cnt);

	co_test_done(test);
}

