#include <lely/tap/tap.h>

int
main(void)
{
	tap_plan(7);

	tap_assert(1 == 1);

	tap_test(1 == 1);
	tap_test(1 == 1, "with comment");

	tap_pass();

	tap_todo(1 == 0);
	tap_todo(1 == 0, "with comment");

	tap_skip(1 == 1);
	tap_skip(1 == 0, "with comment");

	return 0;
}

