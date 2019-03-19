#include "test.h"

int
main(void)
{
	tap_plan(7);

	tap_assert(1 == 1);

	tap_test(1 == 1);
	tap_test(1 == 1, "with\nmultiline\ncomment");

	tap_pass();

	tap_todo(1 == 0);
	tap_todo(1 == 0, "with\nmultiline\ncomment");

	tap_skip(1 == 1);
	tap_skip(1 == 0, "with\nmultiline\ncomment");

	tap_diag("all done");

	return 0;
}
