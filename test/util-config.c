#include "test.h"
#include <lely/util/cmp.h>
#include <lely/util/config.h>

int
main(void)
{
	tap_plan(3);

	config_t *config = config_create(0);
	tap_assert(config);

	tap_test(config_parse_ini_file(config, TEST_SRCDIR "/util-config.ini"));

	tap_test(!str_cmp(config_get(config, "", "key"), "value"));
	tap_test(!str_cmp(config_get(config, "section", "key"), ""));

	config_destroy(config);

	return 0;
}
