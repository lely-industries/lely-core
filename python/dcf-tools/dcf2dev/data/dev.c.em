@[if include_config]@
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

@[end if]@
#if !LELY_NO_MALLOC
#error Static object dictionaries are only supported when dynamic memory allocation is disabled.
#endif

#include <lely/co/detail/dev.h>
#include <lely/co/detail/obj.h>
#include <lely/util/cmp.h>

@[if no_strings]@
#define CO_DEV_STRING(s) NULL
@[else]@
#define CO_DEV_STRING(s) s
@[end if]@

static co_dev_t @name = {
	.netid = 0,
	.id = @dev.node_id,
	.tree = { .cmp = &uint16_cmp },
#if !LELY_NO_CO_OBJ_NAME
	.name = CO_DEV_STRING("@dev.c.name"),
	.vendor_name = CO_DEV_STRING("@dev.c.vendor_name"),
#endif
	.vendor_id = @("0x{:08X}".format(dev.vendor_id)),
#if !LELY_NO_CO_OBJ_NAME
	.product_name = CO_DEV_STRING("@dev.c.product_name"),
#endif
	.product_code = @("0x{:08X}".format(dev.product_code)),
	.revision = @("0x{:08X}".format(dev.revision_number)),
#if !LELY_NO_CO_OBJ_NAME
	.order_code = CO_DEV_STRING("@dev.c.order_code"),
#endif
	.baud = @dev.c.baud,
	.rate = @dev.c.rate,
	.lss = @dev.c.lss,
	.dummy = @("0x{:08X}".format(dev.c.dummy))
};

@[for idx in sorted(dev.keys())]@
@{obj = dev[idx]}@
@{obj_name = name + "_{:04X}".format(idx)}@
static struct {
@[for subidx in sorted(obj.keys())]@
@{subobj = obj[subidx]}@
	@subobj.value.data_type.c.typename sub@("{:X}".format(subidx));
@[end for]@
} @(obj_name)_val = {
@[for subidx in sorted(obj.keys())]@
@{subobj = obj[subidx]}@
@{member_name = "sub{:X}".format(subidx)}@
	.@member_name = @subobj.value.c.value,
@[end for]@
};

static co_obj_t @obj_name = {
	.node = { .key = &@(obj_name).idx },
	.idx = @("0x{:04X}".format(idx)),
	.code = @obj.c.code,
#if !LELY_NO_CO_OBJ_NAME
	.name = CO_DEV_STRING("@obj.name"),
#endif
	.tree = { .cmp = &uint8_cmp },
	.val = &@(obj_name)_val,
	.size = sizeof(@(obj_name)_val)
};

@[for subidx in sorted(obj.keys())]@
@{subobj = obj[subidx]}@
@{subobj_name = obj_name + "sub{:X}".format(subidx)}@
static co_sub_t @subobj_name = {
	.node = { .key = &@(subobj_name).subidx },
	.subidx = @("0x{:02X}".format(subidx)),
	.type = CO_DEFTYPE_@subobj.data_type.name(),
#if !LELY_NO_CO_OBJ_NAME
	.name = CO_DEV_STRING("@subobj.name"),
#endif
#if !LELY_NO_CO_OBJ_LIMITS
	.min = { .@subobj.low_limit.data_type.c.member = @subobj.low_limit.c.value },
	.max = { .@subobj.high_limit.data_type.c.member = @subobj.high_limit.c.value },
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
	.def = { .@subobj.default_value.data_type.c.member = @subobj.default_value.c.value },
#endif
	.val = &@(obj_name)_val.sub@("{:X}".format(subidx)),
	.access = @subobj.c.access,
	.pdo_mapping = @(int(subobj.pdo_mapping)),
	.flags = @subobj.c.flags,
	.dn_ind = &co_sub_default_dn_ind,
#if !LELY_NO_CO_OBJ_UPLOAD
	.up_ind = &co_sub_default_up_ind
#endif
};

@[end for]@
@[end for]@
// suppress missing-prototype warning
co_dev_t * @(name)_init(void);
co_dev_t *
@(name)_init(void) {
	static co_dev_t *dev = NULL;
	if (!dev) {
		dev = &@name;
@[for idx in sorted(dev.keys())]@
@{obj = dev[idx]}@
@{obj_name = name + "_{:04X}".format(idx)}@

		co_dev_insert_obj(&@name, &@obj_name);
@[for subidx in sorted(obj.keys())]@
@{subobj = obj[subidx]}@
@{subobj_name = obj_name + "sub{:X}".format(subidx)}@
		co_obj_insert_sub(&@obj_name, &@subobj_name);
@[end for]@
@[end for]@
	}
	return dev;
}
