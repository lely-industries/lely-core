#include "co-test.h"
#include <lely/co/csdo.h>
#include <lely/co/dcf.h>
#include <lely/co/ssdo.h>
#include <lely/co/val.h>

// A value small enough for a single CAN frame.
#define EXP_VALUE "42"

// A value too large for a single CAN frame.
#define SEG_VALUE "Hello, world!"

// A value too large for a single (127 * 7 bytes) block.
#define BLK_VALUE \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"

void dn_con(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, void *data);
void up_con(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, const void *ptr, size_t n, void *data);

int
main(void)
{
	tap_plan(12);

#if !LELY_NO_STDIO && !LELY_NO_DIAG
	diag_set_handler(&co_test_diag_handler, NULL);
	diag_at_set_handler(&co_test_diag_at_handler, NULL);
#endif

	can_net_t *net = can_net_create();
	tap_assert(net);
	struct co_test test;
	co_test_init(&test, net, 0);

	co_dev_t *sdev = co_dev_create_from_dcf_file(
			TEST_SRCDIR "/co-sdo-server.dcf");
	tap_assert(sdev);
	co_ssdo_t *ssdo = co_ssdo_create(net, sdev, 1);
	tap_assert(ssdo);

	co_dev_t *cdev = co_dev_create_from_dcf_file(
			TEST_SRCDIR "/co-sdo-client.dcf");
	tap_assert(cdev);
	co_csdo_t *csdo = co_csdo_create(net, cdev, 1);
	tap_assert(csdo);

	// clang-format off
	tap_test(!co_csdo_dn_req(csdo, 0x2000, 0x00, EXP_VALUE,
			strlen(EXP_VALUE), &dn_con, &test),
			"expedited SDO download");
	// clang-format on
	co_test_wait(&test);

	tap_test(!co_csdo_up_req(csdo, 0x2000, 0x00, &up_con, &test),
			"expedited SDO upload");
	co_test_wait(&test);

	// clang-format off
	tap_test(!co_csdo_dn_req(csdo, 0x2000, 0x00, SEG_VALUE,
			strlen(SEG_VALUE), &dn_con, &test),
			"segmented SDO download");
	// clang-format on
	co_test_wait(&test);

	tap_test(!co_csdo_up_req(csdo, 0x2000, 0x00, &up_con, &test),
			"segmented SDO upload");
	co_test_wait(&test);

	// clang-format off
	tap_test(!co_csdo_blk_dn_req(csdo, 0x2000, 0x00, BLK_VALUE,
			strlen(BLK_VALUE), &dn_con, &test),
			"SDO block download");
	// clang-format on
	co_test_wait(&test);

	tap_test(!co_csdo_blk_up_req(csdo, 0x2000, 0x00, 0, &up_con, &test),
			"SDO block upload");
	co_test_wait(&test);

	co_csdo_destroy(csdo);
	co_dev_destroy(cdev);

	co_ssdo_destroy(ssdo);
	co_dev_destroy(sdev);

	co_test_fini(&test);
	can_net_destroy(net);

	return 0;
}

void
dn_con(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, void *data)
{
	(void)sdo;
	struct co_test *test = data;

	if (!ac)
		tap_pass("value sent");
	else
		tap_fail("received abort code %08X for SDO %Xsub%X: %s", ac,
				idx, subidx, co_sdo_ac2str(ac));

	co_test_done(test);
}

void
up_con(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, const void *ptr, size_t n, void *data)
{
	(void)sdo;
	struct co_test *test = data;

	if (!ac) {
		union co_val val;
		co_val_init(CO_DEFTYPE_VISIBLE_STRING, &val);
		// clang-format off
		if (!n || co_val_read(CO_DEFTYPE_VISIBLE_STRING, &val, ptr,
				(const uint_least8_t *)ptr + n) == n)
			// clang-format on
			tap_pass("value received\n%s", val.vs);
		else
			tap_fail("unable to read value");
		co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
	} else {
		tap_fail("received abort code %08X for SDO %Xsub%X: %s", ac,
				idx, subidx, co_sdo_ac2str(ac));
	}

	co_test_done(test);
}
