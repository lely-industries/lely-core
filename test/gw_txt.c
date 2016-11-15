#include <lely/co/dcf.h>
#include <lely/co/gw_txt.h>
#include <lely/co/nmt.h>

#include "test.h"

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
	co_test_init(&test, net);

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

