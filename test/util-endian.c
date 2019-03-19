#include "test.h"
#include <lely/util/endian.h>

#define VALUE UINT64_C(0x0123456789abcdef)
#define BVAL_BE \
	{ \
		0x00, 0x3c, 0xda, 0xb8, 0x96, 0x74, 0x52, 0x00 \
	}
#define BVAL_LE \
	{ \
		0x00, 0xdc, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x02 \
	}

int
main(void)
{
	tap_plan(3);

	unsigned char buf[sizeof(uint64_t)] = { 0 };

	stle_u64(buf, VALUE);
	tap_test(ldle_u64(buf) == VALUE);

	unsigned char tmp[sizeof(buf)] = { 0 };
	bcpybe(tmp, 10, buf, 6, 48);
	tap_test(!memcmp(tmp, (unsigned char[])BVAL_BE, sizeof(tmp)));
	bcpyle(tmp, 10, buf, 6, 48);
	tap_test(!memcmp(tmp, (unsigned char[])BVAL_LE, sizeof(tmp)));

	return 0;
}
