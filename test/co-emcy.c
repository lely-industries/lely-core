#include "co-test.h"
#include <lely/co/dcf.h>
#include <lely/co/emcy.h>

void emcy_ind(co_emcy_t *emcy, co_unsigned8_t id, co_unsigned16_t ec,
		co_unsigned8_t er, co_unsigned8_t msef[5], void *data);

int
main(void)
{
	tap_plan(10);

#if !LELY_NO_STDIO && !LELY_NO_DIAG
	diag_set_handler(&co_test_diag_handler, NULL);
	diag_at_set_handler(&co_test_diag_at_handler, NULL);
#endif

	can_net_t *net = can_net_create();
	tap_assert(net);
	struct co_test test;
	co_test_init(&test, net, 0);

	co_dev_t *dev = co_dev_create_from_dcf_file(TEST_SRCDIR "/co-emcy.dcf");
	tap_assert(dev);
	co_emcy_t *emcy = co_emcy_create(net, dev);
	tap_assert(emcy);

	co_emcy_set_ind(emcy, &emcy_ind, &test);

	co_emcy_push(emcy, 0x1000, 0x00, NULL);
	co_test_wait(&test);
	co_emcy_push(emcy, 0x2000, 0x02, NULL);
	co_test_wait(&test);
	co_emcy_push(emcy, 0x3000, 0x04, NULL);
	co_test_wait(&test);
	co_emcy_push(emcy, 0x4000, 0x08, NULL);
	co_test_wait(&test);
	co_emcy_push(emcy, 0x8100, 0x10, NULL);
	co_test_wait(&test);

	co_emcy_pop(emcy, NULL, NULL);
	co_test_wait(&test);
	co_emcy_pop(emcy, NULL, NULL);
	co_test_wait(&test);
	co_emcy_pop(emcy, NULL, NULL);
	co_test_wait(&test);
	co_emcy_pop(emcy, NULL, NULL);
	co_test_wait(&test);
	co_emcy_pop(emcy, NULL, NULL);
	co_test_wait(&test);

	co_emcy_destroy(emcy);
	co_dev_destroy(dev);

	co_test_fini(&test);
	can_net_destroy(net);

	return 0;
}

void
emcy_ind(co_emcy_t *emcy, co_unsigned8_t id, co_unsigned16_t ec,
		co_unsigned8_t er, co_unsigned8_t msef[5], void *data)
{
	(void)emcy;
	(void)msef;
	struct co_test *test = data;

	tap_pass("received EMCY [%d: %04X (%02X)]", id, ec, er);

	co_test_done(test);
}
