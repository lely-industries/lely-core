#include "co-test.h"
#include <lely/co/dcf.h>
#include <lely/co/gw_txt.h>
#include <lely/co/nmt.h>

#define TEST_WAIT 1
#define TEST_STEP 20

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
	"[10] lss_ident_nonconf",
	"[11] _lss_slowscan 0x360 0 0 0 0 0",
	"[12] lss_switch_glob 0",
	"[13] _lss_fastscan 0 0xffff0000 0 0xffffffff 0 0xffffffff 0 0xffffffff",
	"[14] lss_switch_glob 0",
	"[15] lss_identity 0x360 0 0 0 0 0",
	"[16] lss_switch_sel 0x360 0 0 0",
	"[17] lss_get_node",
	"[18] lss_set_node 2",
	"[19] lss_conf_bitrate 0 9",
	"[20] lss_activate_bitrate 100",
	"[21] lss_store",
	"[22] lss_switch_glob 0",
	"[23] 0 reset comm",
	"[30] set node 2",
	"[31] disable guarding",
	"[32] enable heartbeat 100",
	"[33] reset node",
	"[34] reset comm",
	"[35] preop",
	"[36] stop",
	"[37] start",
	// Read the identity object of the gateway itself.
	"[40] 1 r 0x1018 0 u8",
	"[41] 1 r 0x1018 1 u32",
	"[42] 1 r 0x1018 2 u32",
	"[43] 1 r 0x1018 3 u32",
	"[44] 1 r 0x1018 4 u32",
	// Read, write and read again a value of every single data type.
	"[50] r 0x2001 0 b",
	"[51] w 0x2001 0 b 1",
	"[52] r 0x2001 0 b",
	"[53] r 0x2002 0 i8",
	"[54] w 0x2002 0 i8 -128",
	"[55] r 0x2002 0 i8",
	"[56] r 0x2003 0 i16",
	"[57] w 0x2003 0 i16 -32768",
	"[58] r 0x2003 0 i16",
	"[59] r 0x2004 0 i32",
	"[60] w 0x2004 0 i32 -2147483648",
	"[61] r 0x2004 0 i32",
	"[62] r 0x2005 0 u8",
	"[63] w 0x2005 0 u8 255",
	"[64] r 0x2005 0 u8",
	"[65] r 0x2006 0 u16",
	"[66] w 0x2006 0 u16 65535",
	"[67] r 0x2006 0 u16",
	"[68] r 0x2007 0 u32",
	"[69] w 0x2007 0 u32 4294967295",
	"[70] r 0x2007 0 u32",
	"[71] r 0x2008 0 r32",
	"[72] w 0x2008 0 r32 3.14159274101257324219",
	"[73] r 0x2008 0 r32",
	"[74] r 0x2009 0 vs",
	"[75] w 0x2009 0 vs \"Hello, \"\"World\"\"!\"",
	"[76] r 0x2009 0 vs",
	"[77] r 0x200A 0 os",
	"[78] w 0x200A 0 os ASNFZ4mrze8=",
	"[79] r 0x200A 0 os",
	"[80] r 0x200B 0 us",
	"[81] w 0x200B 0 us SABlAGwAbABvACwAIABXAG8AcgBsAGQAIQA=",
	"[82] r 0x200B 0 us",
	"[83] r 0x200C 0 t",
	"[84] w 0x200C 0 t 365 43200000",
	"[85] r 0x200C 0 t",
	"[86] r 0x200D 0 td",
	"[87] w 0x200D 0 td 365 43200000",
	"[88] r 0x200D 0 td",
	"[89] r 0x200F 0 d",
	"[90] w 0x200F 0 d ASNFZ4mrze8=",
	"[91] r 0x200F 0 d",
	"[92] r 0x2010 0 i24",
	"[93] w 0x2010 0 i24 -8388608",
	"[94] r 0x2010 0 i24",
	"[95] r 0x2011 0 r64",
	"[96] w 0x2011 0 r64 3.14159274101257324219",
	"[97] r 0x2011 0 r64",
	"[98] r 0x2012 0 i40",
	"[99] w 0x2012 0 i40 -549755813888",
	"[100] r 0x2012 0 i40",
	"[101] r 0x2013 0 i48",
	"[102] w 0x2013 0 i48 -140737488355328",
	"[103] r 0x2013 0 i48",
	"[104] r 0x2014 0 i56",
	"[105] w 0x2014 0 i56 -36028797018963968",
	"[106] r 0x2014 0 i56",
	"[107] r 0x2015 0 i64",
	"[108] w 0x2015 0 i64 -9223372036854775808",
	"[109] r 0x2015 0 i64",
	"[110] r 0x2016 0 u24",
	"[111] w 0x2016 0 u24 16777215",
	"[112] r 0x2016 0 u24",
	"[113] r 0x2018 0 u40",
	"[114] w 0x2018 0 u40 1099511627775",
	"[115] r 0x2018 0 u40",
	"[116] r 0x2019 0 u48",
	"[117] w 0x2019 0 u48 281474976710655",
	"[118] r 0x2019 0 u48",
	"[119] r 0x201A 0 u56",
	"[120] w 0x201A 0 u56 72057594037927935",
	"[121] r 0x201A 0 u56",
	"[122] r 0x201B 0 u64",
	"[123] w 0x201B 0 u64 18446744073709551615",
	"[124] r 0x201B 0 u64",
	// Configure the slave RPDO.
	"[130] w 0x1400 1 u32 0x80000202",
	"[131] w 0x1600 0 u8 0",
	"[132] w 0x1600 1 u32 0x20050008",
	"[133] w 0x1600 2 u32 0x20060010",
	"[134] w 0x1600 3 u32 0x20070020",
	"[135] w 0x1600 0 u8 3",
	"[136] w 0x1400 1 u32 0x202",
	// Configure the slave TPDO.
	"[137] w 0x1800 1 u32 0x80000182",
	"[138] w 0x1800 2 u8 0x01",
	"[139] w 0x1A00 0 u8 0",
	"[140] w 0x1A00 1 u32 0x20050008",
	"[141] w 0x1A00 2 u32 0x20060010",
	"[142] w 0x1A00 3 u32 0x20070020",
	"[143] w 0x1A00 0 u8 3",
	"[144] w 0x1800 1 u32 0x182",
	// Configure the gateway RPDO and TPDO.
	"[145] set rpdo 1 0x182 sync0 3 0x2003 0 u8 0x2004 0 u16 0x2005 0 u32",
	"[146] set tpdo 1 0x202 sync1 3 0x2000 0 u8 0x2001 0 u16 0x2002 0 u32",
	// Activate the SYNC producer.
	"[147] 1 w 0x1005 0 u32 0x40000080",
	"[148] 1 w 0x1006 0 u32 10000",
	"[149] write pdo 1 3 0x12 0x3456 0x789ABCDE",
	"[150] read pdo 1",
};

int gw_send(const struct co_gw_srv *srv, void *data);
int gw_txt_recv(const char *txt, void *data);
int gw_txt_send(const struct co_gw_req *req, void *data);

int
main(void)
{
	tap_plan(2);

#if !LELY_NO_STDIO && !LELY_NO_DIAG
	diag_set_handler(&co_test_diag_handler, NULL);
	diag_at_set_handler(&co_test_diag_at_handler, NULL);
#endif

	can_net_t *net = can_net_create();
	tap_assert(net);

	struct co_test test;
	co_test_init(&test, net, TEST_WAIT);

	co_dev_t *mdev = co_dev_create_from_dcf_file(
			TEST_SRCDIR "/co-gw_txt-master.dcf");
	tap_assert(mdev);

	co_nmt_t *master = co_nmt_create(net, mdev);
	tap_assert(master);

	co_dev_t *sdev = co_dev_create_from_dcf_file(
			TEST_SRCDIR "/co-gw_txt-slave.dcf");
	tap_assert(sdev);

	co_nmt_t *slave = co_nmt_create(net, sdev);
	tap_assert(slave);

#if LELY_NO_CO_LSS
	co_nmt_set_id(slave, 0x02);
#endif

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
			do
				co_test_step(&test);
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
