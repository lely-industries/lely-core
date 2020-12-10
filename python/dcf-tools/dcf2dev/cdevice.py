import struct

import dcf


class CDevice:
    def __init__(self, dev: dcf.Device):
        self.name = ""
        self.vendor_name = ""
        self.product_name = ""
        self.order_code = ""
        self.baud = "0"
        self.rate = 0
        self.lss = 0
        self.dummy = 0

        if "DeviceInfo" in dev.cfg:
            section = dev.cfg["DeviceInfo"]
            self.vendor_name = section.get("VendorName", "")
            self.product_name = section.get("ProductName", "")
            self.order_code = section.get("OrderCode", "")
            self.__parse_baud_rate(section)
            if "LSS_Supported" in section:
                self.lss = int(section["LSS_Supported"])
        if "DeviceComissioning" in dev.cfg:
            section = dev.cfg["DeviceComissioning"]
            self.name = section.get("NodeName", "")
        if "DummyUsage" in dev.cfg:
            section = dev.cfg["DummyUsage"]
            for i in range(0x20):
                if bool(int(section.get("Dummy{:04X}".format(i), "0"), 2)):
                    self.dummy = self.dummy | 1 << i

        for obj in dev.values():
            obj.c = CObject(obj)

    def __parse_baud_rate(self, section: dict):
        if bool(int(section.get("BaudRate_10", "0"), 2)):
            self.baud += " | CO_BAUD_10"
            self.rate = 10
        if bool(int(section.get("BaudRate_20", "0"), 2)):
            self.baud += " | CO_BAUD_20"
            self.rate = 20
        if bool(int(section.get("BaudRate_50", "0"), 2)):
            self.baud += " | CO_BAUD_50"
            self.rate = 50
        if bool(int(section.get("BaudRate_125", "0"), 2)):
            self.baud += " | CO_BAUD_125"
            self.rate = 125
        if bool(int(section.get("BaudRate_250", "0"), 2)):
            self.baud += " | CO_BAUD_250"
            self.rate = 250
        if bool(int(section.get("BaudRate_500", "0"), 2)):
            self.baud += " | CO_BAUD_500"
            self.rate = 500
        if bool(int(section.get("BaudRate_800", "0"), 2)):
            self.baud += " | CO_BAUD_800"
            self.rate = 800
        if bool(int(section.get("BaudRate_1000", "0"), 2)):
            self.baud += " | CO_BAUD_1000"
            self.rate = 1000


class CObject:
    def __init__(self, obj: dcf.Object):
        self.code = "CO_OBJECT_VAR"

        if obj.object_type == 0x00:
            self.code = "CO_OBJECT_NULL"
        elif obj.object_type == 0x02:
            self.code = "CO_OBJECT_DOMAIN"
        elif obj.object_type == 0x05:
            self.code = "CO_OBJECT_DEFTYPE"
        elif obj.object_type == 0x06:
            self.code = "CO_OBJECT_DEFSTRUCT"
        elif obj.object_type == 0x07:
            self.code = "CO_OBJECT_VAR"
        elif obj.object_type == 0x08:
            self.code = "CO_OBJECT_ARRAY"
        elif obj.object_type == 0x09:
            self.code = "CO_OBJECT_RECORD"
        else:
            self.code = "0x{:02X}".format(obj.object_type)

        for subobj in obj.values():
            subobj.c = CSubObject(subobj)


class CSubObject:
    def __init__(self, subobj: dcf.SubObject):
        self.type = "CO_DEFTYPE_" + subobj.data_type.name()
        self.access = "CO_ACCESS_" + subobj.access_type.name.upper()

        self.flags = "0"
        if hasattr(subobj, "upload_file"):
            self.flags += " | CO_OBJ_FLAGS_UPLOAD_FILE"
        if hasattr(subobj, "download_file"):
            self.flags += " | CO_OBJ_FLAGS_DOWNLOAD_FILE"

        if hasattr(subobj, "low_limit"):
            subobj.low_limit.c = CValue(subobj.low_limit, subobj.env)
        else:
            subobj.low_limit = dcf.Value(subobj.data_type)
            subobj.low_limit.c = CValue(
                subobj.low_limit, {}, "CO_" + subobj.data_type.name() + "_MIN"
            )
        if subobj.low_limit.has_nodeid():
            self.flags += " | CO_OBJ_FLAGS_MIN_NODEID"

        if hasattr(subobj, "high_limit"):
            subobj.high_limit.c = CValue(subobj.high_limit, subobj.env)
        else:
            subobj.high_limit = dcf.Value(subobj.data_type)
            subobj.high_limit.c = CValue(
                subobj.high_limit, {}, "CO_" + subobj.data_type.name() + "_MAX"
            )
        if subobj.high_limit.has_nodeid():
            self.flags += " | CO_OBJ_FLAGS_MAX_NODEID"

        if hasattr(subobj, "upload_file"):
            subobj.default_value.c = CValue(subobj.default_value, {}, "NULL")
            subobj.value.c = CValue.from_visible_string(subobj.upload_file)
        elif hasattr(subobj, "download_file"):
            subobj.default_value.c = CValue(subobj.default_value, {}, "NULL")
            subobj.value.c = CValue.from_visible_string(subobj.download_file)
        else:
            if subobj.default_value.has_nodeid():
                self.flags += " | CO_OBJ_FLAGS_DEF_NODEID"
            subobj.default_value.c = CValue(subobj.default_value, subobj.env)
            if subobj.value.has_nodeid():
                self.flags += " | CO_OBJ_FLAGS_VAL_NODEID"
            subobj.value.c = CValue(subobj.value, subobj.env)

        if hasattr(subobj, "parameter_value"):
            self.flags += " | CO_OBJ_FLAGS_PARAMETER_VALUE"


class CDataType:
    __format = {
        0x0001: "{:d}",  # BOOLEAN
        0x0002: "{:d}",  # INTEGER8
        0x0003: "{:d}",  # INTEGER16
        0x0004: "{:d}",  # INTEGER32
        0x0005: "0x{:02X}",  # UNSIGNED8
        0x0006: "0x{:04X}",  # UNSIGNED16
        0x0007: "0x{:08X}",  # UNSIGNED32
        0x0008: "{:.9g}",  # REAL32
        0x0009: 'CO_VISIBLE_STRING_C("{:s}")',  # VISIBLE_STRING
        0x000A: 'CO_OCTET_STRING_C("{:s}")',  # OCTET_STRING
        0x000B: "CO_UNICODE_STRING_C({{ {:s} }})",  # UNICODE_STRING
        0x000C: "{{ .ms = {0[1]:d}, .days = {0[0]:d} }}",  # TIME_OF_DAY
        0x000D: "{{ .ms = {0[1]:d}, .days = {0[0]:d} }}",  # TIME_DIFF
        0x000F: "CO_DOMAIN_C(co_unsigned8_t, {{ {:s} }})",  # DOMAIN
        0x0010: "{:d}",  # INTEGER24
        0x0011: "{:.17g}",  # REAL64
        0x0012: "{:d}",  # INTEGER40
        0x0013: "{:d}",  # INTEGER48
        0x0014: "{:d}",  # INTEGER56
        0x0015: "{:d}",  # INTEGER64
        0x0016: "0x{:06X}",  # UNSIGNED24
        0x0018: "0x{:010X}",  # UNSIGNED40
        0x0019: "0x{:012X}",  # UNSIGNED48
        0x001A: "0x{:014X}",  # UNSIGNED56
        0x001B: "0x{:016X}",  # UNSIGNED64
    }

    __member = {
        0x0001: "b",  # BOOLEAN
        0x0002: "i8",  # INTEGER8
        0x0003: "i16",  # INTEGER16
        0x0004: "i32",  # INTEGER32
        0x0005: "u8",  # UNSIGNED8
        0x0006: "u16",  # UNSIGNED16
        0x0007: "u32",  # UNSIGNED32
        0x0008: "r32",  # REAL32
        0x0009: "vs",  # VISIBLE_STRING
        0x000A: "os",  # OCTET_STRING
        0x000B: "us",  # UNICODE_STRING
        0x000C: "t",  # TIME_OF_DAY
        0x000D: "td",  # TIME_DIFF
        0x000F: "dom",  # DOMAIN
        0x0010: "i24",  # INTEGER24
        0x0011: "r64",  # REAL64
        0x0012: "i40",  # INTEGER40
        0x0013: "i48",  # INTEGER48
        0x0014: "i56",  # INTEGER56
        0x0015: "i64",  # INTEGER64
        0x0016: "u24",  # UNSIGNED24
        0x0018: "u40",  # UNSIGNED40
        0x0019: "u48",  # UNSIGNED48
        0x001A: "u56",  # UNSIGNED56
        0x001B: "u64",  # UNSIGNED64
    }

    def __init__(self, data_type: dcf.DataType):
        self.typename = "co_" + data_type.name().lower() + "_t"
        self.member = CDataType.__member[data_type.index]

    @staticmethod
    def add_custom(index: int, member: str, format_spec: str):
        CDataType.__member[index] = member
        CDataType.__format[index] = format_spec

    @staticmethod
    def print_value(val: dcf.Value, env: dict = {}):
        value = val.parse(env)
        if val.data_type.is_basic():
            if value == val.data_type.min() and value != 0:
                return "CO_" + val.data_type.name() + "_MIN"
            elif value == val.data_type.max():
                return "CO_" + val.data_type.name() + "_MAX"
        elif val.data_type.is_array():
            if not value:
                return "CO_ARRAY_C"
            if val.data_type.index == 0x0009:  # VISIBLE_STRING
                value = (
                    value.encode("unicode_escape").decode("utf_8").replace('"', '\\"')
                )
            elif val.data_type.index == 0x000A:  # OCTET_STRING
                u8 = list(value)
                value = "".join("\\x{:02x}".format(x) for x in u8)
            elif val.data_type.index == 0x000B:  # UNICODE_STRING
                # Add a terminating null byte.
                us = (value + "\x00").encode("utf_16_le")
                u16 = list(x[0] for x in struct.iter_unpack("<H", us))
                value = ", ".join("0x{:04x}".format(x) for x in u16)
            elif val.data_type.index == 0x000F:  # DOMAIN
                u8 = list(value)
                value = ", ".join("0x{:02x}".format(x) for x in u8)
        return CDataType.__format[val.data_type.index].format(value)


class CValue:
    def __init__(self, val: dcf.Value, env: dict = None, value=""):
        if env is None:
            env = {}

        val.data_type.c = CDataType(val.data_type)

        if not value:
            value = CDataType.print_value(val, env)
        self.value = value

    @classmethod
    def from_visible_string(cls, filename: str) -> "CValue":
        return cls(dcf.Value(dcf.DataType(0x0009), '"' + filename + '"'))
