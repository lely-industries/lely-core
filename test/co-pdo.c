#include "co-test.h"
#include <lely/co/dcf.h>
#include <lely/co/rpdo.h>
#include <lely/co/tpdo.h>

#define VAL_2000 0x01234567u
#define VAL_2001 0x89abcdefu

int
main(void)
{
	tap_plan(6);

#if !LELY_NO_STDIO && !LELY_NO_DIAG
	diag_set_handler(&co_test_diag_handler, NULL);
	diag_at_set_handler(&co_test_diag_at_handler, NULL);
#endif

	can_net_t *net = can_net_create();
	tap_assert(net);
	struct co_test test;
	co_test_init(&test, net, 0);

	co_dev_t *rdev = co_dev_create_from_dcf_file(
			TEST_SRCDIR "/co-pdo-receive.dcf");
	tap_assert(rdev);
	co_rpdo_t *rpdo = co_rpdo_create(net, rdev, 1);
	tap_assert(rpdo);

	co_dev_t *tdev = co_dev_create_from_dcf_file(
			TEST_SRCDIR "/co-pdo-transmit.dcf");
	tap_assert(tdev);
	co_tpdo_t *tpdo = co_tpdo_create(net, tdev, 1);
	tap_assert(tpdo);

	tap_test(co_dev_set_val_u32(tdev, 0x2000, 0x00, VAL_2000),
			"store object 2000");
	tap_test(co_dev_set_val_u32(tdev, 0x2001, 0x00, VAL_2001),
			"store object 2001");

	tap_test(!co_tpdo_sync(tpdo, 0), "transmit PDOs");
	co_test_step(&test);
	tap_test(!co_rpdo_sync(rpdo, 0), "process received PDOs");

	tap_test(co_dev_get_val_u32(rdev, 0x2000, 0x00) == VAL_2000,
			"check value of object 2000");
	tap_test(co_dev_get_val_u32(rdev, 0x2001, 0x00) == VAL_2001,
			"check value of object 2001");

	co_tpdo_destroy(tpdo);
	co_dev_destroy(tdev);

	co_rpdo_destroy(rpdo);
	co_dev_destroy(rdev);

	co_test_fini(&test);
	can_net_destroy(net);

	return 0;
}
