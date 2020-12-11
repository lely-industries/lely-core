#include "co-test.h"
#include <lely/co/dcf.h>
#if !LELY_NO_CO_LSS
#include <lely/co/lss.h>
#endif
#include <lely/co/nmt.h>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>

#define NMT_TIMEOUT 1000

void cs_ind(co_nmt_t *nmt, co_unsigned8_t cs, void *data);
void hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason,
		void *data);
void st_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, void *data);
#if !LELY_NO_CO_LSS
void lss_req(co_nmt_t *nmt, co_lss_t *lss, void *data);
#endif
void boot_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char ec,
		void *data);

#if !LELY_NO_CO_LSS
void err_ind(co_lss_t *lss, co_unsigned8_t cs, co_unsigned8_t err,
		co_unsigned8_t spec, void *data);
void lssid_ind(co_lss_t *lss, co_unsigned8_t cs, co_unsigned32_t id,
		void *data);
void nid_ind(co_lss_t *lss, co_unsigned8_t cs, co_unsigned8_t id, void *data);
void scan_ind(co_lss_t *lss, co_unsigned8_t cs, const struct co_id *id,
		void *data);
#endif

co_unsigned32_t co_1f51_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

co_dev_t *mdev;
co_nmt_t *master;

int
main(void)
{
#if LELY_NO_CO_LSS
	tap_plan(3);
#else
	tap_plan(3 + 17);
#endif

#if !LELY_NO_STDIO && !LELY_NO_DIAG
	diag_set_handler(&co_test_diag_handler, NULL);
	diag_at_set_handler(&co_test_diag_at_handler, NULL);
#endif

	can_net_t *net = can_net_create();
	tap_assert(net);

	struct co_test test;
	co_test_init(&test, net, 0);

	mdev = co_dev_create_from_dcf_file("co-nmt-master.dcf");
	tap_assert(mdev);

	master = co_nmt_create(net, mdev);
	tap_assert(master);
	co_nmt_set_cs_ind(master, &cs_ind, mdev);
	co_nmt_set_hb_ind(master, &hb_ind, NULL);
	co_nmt_set_st_ind(master, &st_ind, NULL);
#if !LELY_NO_CO_LSS
	co_nmt_set_lss_req(master, &lss_req, &test);
#endif
	co_nmt_set_boot_ind(master, &boot_ind, &test);
	co_nmt_set_timeout(master, NMT_TIMEOUT);

	co_dev_t *sdev = co_dev_create_from_dcf_file(
			TEST_SRCDIR "/co-nmt-slave.dcf");
	tap_assert(sdev);
	co_obj_t *obj_1f51 = co_dev_find_obj(sdev, 0x1f51);
	if (obj_1f51)
		co_obj_set_dn_ind(obj_1f51, &co_1f51_dn_ind, sdev);

	co_nmt_t *slave = co_nmt_create(net, sdev);
	tap_assert(slave);
	co_nmt_set_cs_ind(slave, &cs_ind, sdev);

	tap_test(!co_nmt_cs_ind(slave, CO_NMT_CS_RESET_NODE), "reset slave");
	co_test_step(&test);

	tap_test(!co_nmt_cs_ind(master, CO_NMT_CS_RESET_NODE), "reset master");
	co_test_wait(&test);

	co_nmt_destroy(slave);
	co_dev_destroy(sdev);

	co_nmt_destroy(master);
	co_dev_destroy(mdev);

	co_test_fini(&test);
	can_net_destroy(net);

	return 0;
}

void
cs_ind(co_nmt_t *nmt, co_unsigned8_t cs, void *data)
{
	(void)nmt;
	co_dev_t *dev = data;
	tap_assert(dev);

	tap_diag("node %d received command 0x%02x", co_dev_get_id(dev), cs);
}

void
hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason, void *data)
{
	(void)nmt;
	(void)data;

	// clang-format off
	tap_diag("heartbeat %s %s for node %d",
			reason == CO_NMT_EC_TIMEOUT
					? "timeout" : "state change",
			state == CO_NMT_EC_OCCURRED ? "occurred" : "resolved",
			id);
	// clang-format on
}

void
st_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, void *data)
{
	tap_assert(nmt);
	(void)data;

	tap_diag("state %02x reported for node %d", st, id);

	if (co_nmt_is_master(nmt) && st == CO_NMT_ST_BOOTUP)
		co_nmt_boot_req(nmt, id, 100);
}

#if !LELY_NO_CO_LSS
void
lss_req(co_nmt_t *nmt, co_lss_t *lss, void *data)
{
	tap_assert(nmt);
	tap_assert(lss);
	struct co_test *test = data;
	tap_assert(test);

	struct co_id lo = { 4, co_dev_get_val_u32(mdev, 0x1f85, 0x02),
		co_dev_get_val_u32(mdev, 0x1f86, 0x02), 0, 0 };
	struct co_id hi = { 4, co_dev_get_val_u32(mdev, 0x1f85, 0x02),
		co_dev_get_val_u32(mdev, 0x1f86, 0x02), UINT32_MAX,
		UINT32_MAX };
	tap_test(!co_lss_slowscan_req(lss, &lo, &hi, &scan_ind, test),
			"LSS slowscan");
	co_test_wait(test);

	tap_test(!co_lss_switch_req(lss, 0), "switch state global");
	co_test_step(test);

	struct co_id id = { 4, co_dev_get_val_u32(mdev, 0x1f85, 0x02),
		co_dev_get_val_u32(mdev, 0x1f86, 0x02),
		co_dev_get_val_u32(mdev, 0x1f87, 0x02),
		co_dev_get_val_u32(mdev, 0x1f88, 0x02) };
	struct co_id mask = { 4, UINT32_MAX, UINT32_MAX, 0, 0 };
	tap_test(!co_lss_fastscan_req(lss, &id, &mask, &scan_ind, test),
			"LSS fastscan");
	co_test_wait(test);

	tap_test(!co_lss_get_vendor_id_req(lss, &lssid_ind, test),
			"inquire identity vendor-ID");
	co_test_wait(test);

	tap_test(!co_lss_get_product_code_req(lss, &lssid_ind, test),
			"inquire identity product-code");
	co_test_wait(test);

	tap_test(!co_lss_get_revision_req(lss, &lssid_ind, test),
			"inquire identity revision-number");
	co_test_wait(test);

	tap_test(!co_lss_get_serial_nr_req(lss, &lssid_ind, test),
			"inquire identity serial-nunber");
	co_test_wait(test);

	tap_test(!co_lss_get_id_req(lss, &nid_ind, test), "inquire node-ID");
	co_test_wait(test);

	tap_test(!co_lss_set_id_req(lss, 0x02, &err_ind, test),
			"configure node-ID");
	co_test_wait(test);

	tap_test(!co_lss_switch_req(lss, 0), "switch state global");
	co_test_step(test);

	co_nmt_lss_con(nmt);

	co_nmt_cs_req(master, CO_NMT_CS_RESET_NODE, 0);
}
#endif // !LELY_NO_CO_LSS

void
boot_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char ec,
		void *data)
{
	(void)nmt;
	(void)st;
	struct co_test *test = data;
	tap_assert(test);

	tap_test(!ec, "error status %c reported for node %d", ec ? ec : '0',
			id);

	co_test_done(test);
}

#if !LELY_NO_CO_LSS

void
err_ind(co_lss_t *lss, co_unsigned8_t cs, co_unsigned8_t err,
		co_unsigned8_t spec, void *data)
{
	(void)lss;
	(void)cs;
	struct co_test *test = data;
	tap_assert(test);

	if (err == 0xff)
		tap_diag("received implementation-specific error code 0x%02X",
				spec);
	else if (err)
		tap_diag("received error code 0x%02X", err);

	co_test_done(test);
}

void
lssid_ind(co_lss_t *lss, co_unsigned8_t cs, co_unsigned32_t id, void *data)
{
	(void)lss;
	struct co_test *test = data;
	tap_assert(test);

	switch (cs) {
	case 0x5a: tap_pass("received vendor-ID 0x%08X", id); break;
	case 0x5b: tap_pass("received product-code 0x%08X", id); break;
	case 0x5c: tap_pass("received revision-number 0x%08X", id); break;
	case 0x5d: tap_pass("received serial-number 0x%08X", id); break;
	default: tap_fail("unknown command specifier: %02X", cs);
	}

	co_test_done(test);
}

void
nid_ind(co_lss_t *lss, co_unsigned8_t cs, co_unsigned8_t id, void *data)
{
	(void)lss;
	(void)cs;
	struct co_test *test = data;
	tap_assert(test);

	tap_pass("received node-ID %02X", id);

	co_test_done(test);
}

void
scan_ind(co_lss_t *lss, co_unsigned8_t cs, const struct co_id *id, void *data)
{
	(void)lss;
	(void)cs;
	struct co_test *test = data;
	tap_assert(test);

	if (id) {
		tap_pass("slave found");
		tap_diag("received vendor-ID 0x%08X", id->vendor_id);
		tap_diag("received product-code 0x%08X", id->product_code);
		tap_diag("received revision-number 0x%08X", id->revision);
		tap_diag("received serial-number 0x%08X", id->serial_nr);
	} else {
		tap_fail("slave not found");
	}

	co_test_done(test);
}

#endif // !LELY_NO_CO_LSS

co_unsigned32_t
co_1f51_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	tap_assert(sub);
	tap_assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1f51);
	tap_assert(req);
	co_dev_t *dev = data;
	tap_assert(dev);

	co_unsigned32_t ac = 0;

	co_unsigned16_t type = co_sub_get_type(sub);
	union co_val val;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	co_unsigned8_t subidx = co_sub_get_subidx(sub);
	if (!subidx) {
		ac = CO_SDO_AC_NO_WRITE;
		goto error;
	}

	tap_assert(type == CO_DEFTYPE_UNSIGNED8);
	switch (val.u8) {
	case 0: break;
	case 1: break;
	case 2: break;
	case 3: co_dev_set_val_u32(dev, 0x1f56, subidx, 0x12345678); break;
	default: ac = CO_SDO_AC_PARAM_VAL; break;
	}

	co_sub_dn(sub, &val);
error:
	co_val_fini(type, &val);
	return ac;
}
