#include "test.h"
#include <lely/util/frbuf.h>
#include <lely/util/fwbuf.h>

#include <stdio.h>
#include <string.h>

#define FILENAME "util-fbuf.dat"

#define SIZE 1024

#define TXT1 FILENAME
#define POS 32
#define TXT2 "Hello, world!"

void test_frbuf(void);
void test_fwbuf(void);

int
main(void)
{
	tap_plan(18);

	test_fwbuf();
	test_frbuf();

	return 0;
}

void
test_frbuf(void)
{
	frbuf_t *buf = frbuf_create(FILENAME);
	tap_assert(buf);

	char txt1[sizeof(TXT1)] = { '\0' };
	tap_test(frbuf_read(buf, txt1, sizeof(TXT1)) != -1);
	tap_test(!strncmp(txt1, TXT1, sizeof(TXT1)));

	intmax_t pos = frbuf_get_pos(buf);
	tap_test(pos == (intmax_t)sizeof(TXT1));

	size_t size = 0;
	const void *map = frbuf_map(buf, POS, &size);
	tap_assert(map);
	tap_test(size == SIZE - POS);

	char txt2[sizeof(TXT2)] = { '\0' };
	tap_test(frbuf_pread(buf, txt2, sizeof(TXT2), POS) != -1);
	tap_test(!strncmp(txt2, TXT2, sizeof(TXT2)));

	tap_test(!strncmp(map, TXT2, sizeof(TXT2)));

	tap_test(!frbuf_unmap(buf));

	tap_test(frbuf_get_pos(buf) == pos);

	frbuf_destroy(buf);
}

void
test_fwbuf(void)
{
	fwbuf_t *buf = fwbuf_create(FILENAME);
	tap_assert(buf);

	tap_test(!fwbuf_set_size(buf, SIZE));

	tap_test(fwbuf_write(buf, TXT1, sizeof(TXT1)) != -1);

	intmax_t pos = fwbuf_get_pos(buf);
	tap_test(pos == (intmax_t)sizeof(TXT1));

	size_t size = 0;
	void *map = fwbuf_map(buf, POS, &size);
	tap_assert(map);
	tap_test(size == SIZE - POS);

	tap_test(fwbuf_pwrite(buf, TXT2, sizeof(TXT2), POS) != -1);

	tap_test(!strcmp(map, TXT2));

	tap_test(!fwbuf_unmap(buf));

	tap_test(fwbuf_get_pos(buf) == pos);

	tap_test(!fwbuf_commit(buf));

	fwbuf_destroy(buf);
}
