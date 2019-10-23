#include "test.h"
#include <lely/co/dcf.h>
#include <lely/co/sdev.h>
#include <lely/co/val.h>

#include <stdlib.h>

#include "test-co-sdev.h"

int
main(void)
{
	tap_plan(3 * 25 + 6);

	co_dev_t *dev = co_dev_create_from_dcf_file(TEST_SRCDIR "/co-sdev.dcf");
	tap_assert(dev);

	co_dev_t *sdev = co_dev_create_from_sdev(&test_co_sdev);
	tap_assert(sdev);

	co_unsigned16_t maxidx = co_dev_get_idx(dev, 0, NULL);
	co_unsigned16_t *idx = malloc(maxidx * sizeof(co_unsigned16_t));
	tap_assert(idx);
	co_dev_get_idx(dev, maxidx, idx);
	for (size_t i = 0; i < maxidx; i++) {
		if (idx[i] < 0x2000)
			continue;

		co_obj_t *obj = co_dev_find_obj(dev, idx[i]);
		tap_assert(obj);
		co_obj_t *sobj = co_dev_find_obj(sdev, idx[i]);
		tap_assert(sobj);

		co_unsigned8_t subidx[0xff];
		co_unsigned8_t maxsubidx = co_obj_get_subidx(obj, 0xff, subidx);
		for (size_t j = 0; j < maxsubidx; j++) {
			co_sub_t *sub = co_obj_find_sub(obj, subidx[j]);
			tap_assert(sub);
			co_sub_t *ssub = co_obj_find_sub(sobj, subidx[j]);
			tap_assert(ssub);

			co_unsigned16_t type = co_sub_get_type(sub);
			const void *val = co_sub_get_val(sub);
			const void *sval = co_sub_get_val(ssub);
			tap_test(!co_val_cmp(type, val, sval),
					"!co_val_cmp(%04X, <dev>:%04X:%02X, <sdev>:%04X:%02X)",
					type, idx[i], subidx[j], idx[i],
					subidx[j]);

			char buf[256];
			char *cp = buf;
			size_t chars = co_val_print(
					type, val, &cp, cp + sizeof(buf));
			if (chars >= sizeof(buf))
				continue;
			*cp = '\0';

			union co_val u;
			co_val_init(type, &u);
			tap_test(co_val_lex(type, &u, buf, cp, NULL) == chars,
					"co_val_lex(%04X, ..., \"%s\", ...)",
					type, buf);
			tap_test(!co_val_cmp(type, val, &u),
					"!co_val_cmp(%04X, <dev>:%04X:%02X, \"%s\")",
					type, idx[i], subidx[j], buf);
			co_val_fini(type, &u);
		}
	}
	free(idx);

	void *dev_dom = NULL;
	void *sdev_dom = NULL;

	tap_test(!co_dev_write_dcf(dev, 0, 0xffff, &dev_dom),
			"!co_dev_write_dcf(<dev>, ...)");
	tap_test(!co_dev_write_dcf(sdev, 0, 0xffff, &sdev_dom),
			"!co_dev_write_dcf(<sdev>, ...)");

	tap_test(!co_val_cmp(CO_DEFTYPE_DOMAIN, &dev_dom, &sdev_dom),
			"!co_val_cmp(%04X, <dev>, <sdev>)", CO_DEFTYPE_DOMAIN);

	tap_test(!co_dev_read_dcf(sdev, NULL, NULL, &sdev_dom),
			"!co_val_read_dcf(<sdev>, ...)");
	co_val_fini(CO_DEFTYPE_DOMAIN, &sdev_dom);
	tap_test(!co_dev_write_dcf(sdev, 0, 0xffff, &sdev_dom),
			"!co_val_write_dcf(<sdev>, ...)");

	tap_test(!co_val_cmp(CO_DEFTYPE_DOMAIN, &dev_dom, &sdev_dom),
			"!co_val_cmp(%04X, <dev>, <sdev>)", CO_DEFTYPE_DOMAIN);

	co_val_fini(CO_DEFTYPE_DOMAIN, &sdev_dom);
	co_val_fini(CO_DEFTYPE_DOMAIN, &dev_dom);

	co_dev_destroy(sdev);
	co_dev_destroy(dev);

	return 0;
}
