
#include "test.h"
#include <lely/can/buf.h>

static const size_t BUF_SIZE = 15u;
static const size_t MSG_SIZE = 5u;

void
test_setup(struct can_buf *const buf)
{
	tap_assert(can_buf_init(buf, BUF_SIZE) == 0);

	struct can_msg msg_tab[5u];

	const struct can_msg zero_msg = CAN_MSG_INIT;
	for (size_t i = 0; i < MSG_SIZE; ++i) {
		msg_tab[i] = zero_msg;
	}

	tap_assert(can_buf_write(buf, msg_tab, MSG_SIZE) == MSG_SIZE);
}

void
test_teardown(struct can_buf *const buf)
{
	can_buf_fini(buf);
	tap_pass();
}

void
can_buf_clear_test(void)
{
	struct can_buf buf = CAN_BUF_INIT;
	test_setup(&buf);

	can_buf_clear(&buf);

	tap_assert(can_buf_size(&buf) == 0);
	tap_assert(can_buf_capacity(&buf) == BUF_SIZE);

	test_teardown(&buf);
}

void
can_buf_peek_test(void)
{
	const size_t PEEK_SIZE = 4u;
	struct can_buf buf = CAN_BUF_INIT;
	test_setup(&buf);

	const size_t ret = can_buf_peek(&buf, NULL, PEEK_SIZE);

	tap_assert(ret == PEEK_SIZE);
	tap_assert(can_buf_size(&buf) == MSG_SIZE);
	tap_assert(can_buf_capacity(&buf) == (BUF_SIZE - MSG_SIZE));

	test_teardown(&buf);
}

void
can_buf_read_test(void)
{
	const size_t READ_SIZE = 3u;
	struct can_buf buf = CAN_BUF_INIT;
	test_setup(&buf);

	const size_t ret = can_buf_read(&buf, NULL, READ_SIZE);

	tap_assert(ret == READ_SIZE);
	tap_assert(can_buf_size(&buf) == MSG_SIZE - READ_SIZE);
	tap_assert(can_buf_capacity(&buf) == (BUF_SIZE - MSG_SIZE + READ_SIZE));

	test_teardown(&buf);
}

int
main(void)
{
	tap_plan(3u);

	can_buf_clear_test();
	can_buf_peek_test();
	can_buf_read_test();

	return 0;
}
