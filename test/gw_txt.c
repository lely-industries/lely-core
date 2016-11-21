#include <lely/util/diag.h>
#include <lely/co/dcf.h>
#include <lely/co/gw_txt.h>
#include <lely/co/nmt.h>

#include "test.h"

#define TEST_WAIT	10
#define TEST_STEP	2

static const char *cmds[] = {
	"[1] set command_timeout 1000",
	"[2] set command_size 65536",
	"[3] set network 1",
	"[4] info version",
	"[5] boot_up_indication Disable",
	"[6] set id 1",
	"[7] init 0",
	"[8] set heartbeat 50",
	"[9] set node 2",
	"[10] disable guarding",
	"[11] enable heartbeat 100",
	"[12] reset node",
	"[13] reset comm",
	"[14] preop",
	"[15] stop",
	"[16] start",
	"[18] set tpdo 1 0x202 sync1 2 0x2000 0 u32 0x2001 0 u32",
	"[17] set rpdo 1 0x182 sync0 2 0x2002 0 u32 0x2003 0 u32",
	"[19] write pdo 1 2 0x1234 0x5678",
	"[20] read pdo 1",
};

int gw_send(const struct co_gw_srv *srv, void *data);
int gw_txt_recv(const char *txt, void *data);
int gw_txt_send(const struct co_gw_req *req, void *data);

int
main(void)
{
	tap_plan(2);

	can_net_t *net = can_net_create();
	tap_assert(net);

	struct co_test test;
	co_test_init(&test, net, TEST_WAIT);

	co_dev_t *mdev = co_dev_create_from_dcf_file(
			TEST_SRCDIR "/gw_txt-master.dcf");
	tap_assert(mdev);

	co_nmt_t *master = co_nmt_create(net, mdev);
	tap_assert(master);

	co_dev_t *sdev = co_dev_create_from_dcf_file(
			TEST_SRCDIR "/gw_txt-slave.dcf");
	tap_assert(sdev);

	co_nmt_t *slave = co_nmt_create(net, sdev);
	tap_assert(slave);

	co_gw_t *gw = co_gw_create();
	tap_assert(gw);
	tap_test(!co_gw_init_net(gw, 1, master), "initialize CANopen network");

	co_gw_txt_t *gw_txt = co_gw_txt_create();
	tap_assert(gw_txt);

	co_gw_set_send_func(gw, &gw_send, gw_txt);
	co_gw_txt_set_recv_func(gw_txt, &gw_txt_recv, &test);
	co_gw_txt_set_send_func(gw_txt, &gw_txt_send, gw);

	tap_test(!co_nmt_cs_ind(slave, CO_NMT_CS_RESET_NODE), "reset slave");
	co_test_step(&test);

	int ncmd = sizeof(cmds) / sizeof(*cmds);
	for (int i = 0; i < ncmd; i++) {
		tap_diag("%s", cmds[i]);
		struct floc at = { "gw_txt", i + 1, 1 };
		co_gw_txt_send(gw_txt, cmds[i], NULL, &at);
		do co_test_step(&test);
		while (co_gw_txt_pending(gw_txt));
	}
	for (int i = 0; i < TEST_STEP; i++)
		co_test_step(&test);

	co_gw_txt_destroy(gw_txt);
	co_gw_destroy(gw);

	co_nmt_destroy(slave);
	co_dev_destroy(sdev);

	co_nmt_destroy(master);
	co_dev_destroy(mdev);

	co_test_fini(&test);
	can_net_destroy(net);

	return 0;
}

int
gw_send(const struct co_gw_srv *srv, void *data)
{
	co_gw_txt_t *gw_txt = data;
	tap_assert(gw_txt);

	return co_gw_txt_recv(gw_txt, srv);
}

int
gw_txt_recv(const char *txt, void *data)
{
	struct co_test *test = data;
	tap_assert(test);

	tap_diag("%s", txt);

	return 0;
}

int
gw_txt_send(const struct co_gw_req *req, void *data)
{
	co_gw_t *gw = data;
	tap_assert(gw);

	return co_gw_recv(gw, req);
}

