#include <lely/util/diag.h>
#include <lely/co/dcf.h>
#include <lely/co/gw_txt.h>
#include <lely/co/nmt.h>

#include "test.h"

#define TEST_WAIT	1
#define TEST_STEP	20

static const char *cmds[] = {
	"[1] set command_timeout 1000",
	"[2] set command_size 65536",
	"[3] set network 1",
	"[4] info version",
	"[5] set sdo_timeout 1000",
	"[6] boot_up_indication Disable",
	"[7] set id 1",
	"[8] init 0",
	"[9] set heartbeat 50",
	"[10] set node 2",
	"[20] disable guarding",
	"[21] enable heartbeat 100",
	"[22] reset node",
	"[23] reset comm",
	"[24] preop",
	"[25] stop",
	"[26] start",
	"[30] 1 read 0x1018 0 u8",
	"[31] 1 read 0x1018 1 u32",
	"[32] 1 read 0x1018 2 u32",
	"[33] 1 read 0x1018 3 u32",
	"[34] 1 read 0x1018 4 u32",
	"[40] r 0x2001 0 b",
	"[41] w 0x2001 0 b 1",
	"[42] r 0x2001 0 b",
	"[43] r 0x2002 0 i8",
	"[44] w 0x2002 0 i8 -128",
	"[45] r 0x2002 0 i8",
	"[46] r 0x2003 0 i16",
	"[47] w 0x2003 0 i16 -32768",
	"[48] r 0x2003 0 i16",
	"[49] r 0x2004 0 i32",
	"[50] w 0x2004 0 i32 -2147483648",
	"[51] r 0x2004 0 i32",
	"[52] r 0x2005 0 u8",
	"[53] w 0x2005 0 u8 255",
	"[54] r 0x2005 0 u8",
	"[55] r 0x2006 0 u16",
	"[56] w 0x2006 0 u16 65535",
	"[57] r 0x2006 0 u16",
	"[58] r 0x2007 0 u32",
	"[59] w 0x2007 0 u32 4294967295",
	"[60] r 0x2007 0 u32",
	"[61] r 0x2008 0 r32",
	"[62] w 0x2008 0 r32 3.14159274101257324219",
	"[63] r 0x2008 0 r32",
	"[64] r 0x2009 0 vs",
	"[65] w 0x2009 0 vs \"Hello, \"\"World\"\"!\"",
	"[66] r 0x2009 0 vs",
	"[67] r 0x200A 0 os",
	"[68] w 0x200A 0 os ASNFZ4mrze8=",
	"[69] r 0x200A 0 os",
	"[70] r 0x200B 0 us",
	"[71] w 0x200B 0 us SABlAGwAbABvACwAIABXAG8AcgBsAGQAIQA=",
	"[72] r 0x200B 0 us",
	"[73] r 0x200C 0 t",
	"[74] w 0x200C 0 t 365 43200000",
	"[75] r 0x200C 0 t",
	"[76] r 0x200D 0 td",
	"[77] w 0x200D 0 td 365 43200000",
	"[78] r 0x200D 0 td",
	"[79] r 0x200F 0 d",
	"[80] w 0x200F 0 d ASNFZ4mrze8=",
	"[81] r 0x200F 0 d",
	"[82] r 0x2010 0 i24",
	"[83] w 0x2010 0 i24 -8388608",
	"[84] r 0x2010 0 i24",
	"[85] r 0x2011 0 r64",
	"[86] w 0x2011 0 r64 3.14159274101257324219",
	"[87] r 0x2011 0 r64",
	"[88] r 0x2012 0 i40",
	"[89] w 0x2012 0 i40 -549755813888",
	"[90] r 0x2012 0 i40",
	"[91] r 0x2013 0 i48",
	"[92] w 0x2013 0 i48 -140737488355328",
	"[93] r 0x2013 0 i48",
	"[94] r 0x2014 0 i56",
	"[95] w 0x2014 0 i56 -36028797018963968",
	"[96] r 0x2014 0 i56",
	"[97] r 0x2015 0 i64",
	"[98] w 0x2015 0 i64 -9223372036854775808",
	"[99] r 0x2015 0 i64",
	"[100] r 0x2016 0 u24",
	"[101] w 0x2016 0 u24 16777215",
	"[102] r 0x2016 0 u24",
	"[103] r 0x2018 0 u40",
	"[104] w 0x2018 0 u40 1099511627775",
	"[105] r 0x2018 0 u40",
	"[106] r 0x2019 0 u48",
	"[107] w 0x2019 0 u48 281474976710655",
	"[108] r 0x2019 0 u48",
	"[109] r 0x201A 0 u56",
	"[110] w 0x201A 0 u56 72057594037927935",
	"[111] r 0x201A 0 u56",
	"[112] r 0x201B 0 u64",
	"[113] w 0x201B 0 u64 18446744073709551615",
	"[114] r 0x201B 0 u64",
	"[120] write 0x1400 1 u32 0x80000202",
	"[121] write 0x1600 0 u8 0",
	"[122] write 0x1600 1 u32 0x20050008",
	"[123] write 0x1600 2 u32 0x20060010",
	"[124] write 0x1600 3 u32 0x20070020",
	"[125] write 0x1600 0 u8 3",
	"[126] write 0x1400 1 u32 0x202",
	"[127] write 0x1800 1 u32 0x80000182",
	"[128] write 0x1800 2 u8 0x01",
	"[129] write 0x1A00 0 u8 0",
	"[130] write 0x1A00 1 u32 0x20050008",
	"[131] write 0x1A00 2 u32 0x20060010",
	"[132] write 0x1A00 3 u32 0x20070020",
	"[133] write 0x1A00 0 u8 3",
	"[134] write 0x1800 1 u32 0x182",
	"[135] set tpdo 1 0x202 sync1 3 0x2000 0 u8 0x2001 0 u16 0x2002 0 u32",
	"[136] set rpdo 1 0x182 sync0 3 0x2003 0 u8 0x2004 0 u16 0x2005 0 u32",
	"[137] write pdo 1 3 0x12 0x3456 0x789ABCDE",
	"[138] read pdo 1",
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
		const char *cp = cmds[i];
		size_t chars = 0;
		while ((chars = co_gw_txt_send(gw_txt, cp, NULL, &at))) {
			cp += chars;
			do co_test_step(&test);
			while (co_gw_txt_pending(gw_txt));
		}
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

