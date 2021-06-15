import ast
import re
import struct
import warnings

from .lint import lint
from .parse import parse_file


class Device(dict):
    def __init__(self, cfg: dict, env: dict = None):
        self.cfg = cfg
        self.env = env if env is not None else {}

        if (
            "NODEID" not in self.env
            and "DeviceComissioning" in self.cfg
            and "NodeID" in self.cfg["DeviceComissioning"]
        ):
            self.env["NODEID"] = int(self.cfg["DeviceComissioning"]["NodeID"], 0)
        self.node_id = self.env.get("NODEID", 255)
        if (self.node_id < 1 or self.node_id > 127) and self.node_id != 255:
            warnings.warn(
                "invalid node-ID specified: {}".format(self.node_id), stacklevel=2
            )

        for section in {"MandatoryObjects", "OptionalObjects", "ManufacturerObjects"}:
            for i in range(int(self.cfg[section]["SupportedObjects"], 10)):
                index = int(self.cfg[section][str(i + 1)], 0)
                self[index] = Object(self.cfg, index, self.env)

        self.device_type = self[0x1000][0].parse_value()
        self.vendor_id = self[0x1018][1].parse_value()
        try:
            self.product_code = self[0x1018][2].parse_value()
        except KeyError:
            self.product_code = 0
        try:
            self.revision_number = self[0x1018][3].parse_value()
        except KeyError:
            self.revision_number = 0
        try:
            self.serial_number = self[0x1018][4].parse_value()
        except KeyError:
            self.serial_number = 0
        if "DeviceInfo" in self.cfg:
            section = self.cfg["DeviceInfo"]
            if "VendorNumber" in section and section["VendorNumber"]:
                vendor_id = int(section["VendorNumber"], 0)
                if self.vendor_id != 0 and self.vendor_id != vendor_id:
                    warnings.warn(
                        "VendorNumber in [DeviceInfo] differs from vendor-ID in identity object",
                        stacklevel=2,
                    )
                self.vendor_id = vendor_id
            if "ProductNumber" in section and section["ProductNumber"]:
                product_code = int(section["ProductNumber"], 0)
                if self.product_code != 0 and self.product_code != product_code:
                    warnings.warn(
                        "ProductNumber in [DeviceInfo] differs from product code in identity object",
                        stacklevel=2,
                    )
                self.product_code = product_code
            if "RevisionNumber" in section and section["RevisionNumber"]:
                revision_number = int(section["RevisionNumber"], 0)
                if (
                    self.revision_number != 0
                    and self.revision_number != revision_number
                ):
                    warnings.warn(
                        "RevisionNumber in [DeviceInfo] differs from revision number in identity object",
                        stacklevel=2,
                    )
                self.revision_number = revision_number
        if "DeviceComissioning" in self.cfg:
            section = self.cfg["DeviceComissioning"]
            if "LSS_SerialNumber" in section and section["LSS_SerialNumber"]:
                serial_number = int(section["LSS_SerialNumber"], 0)
                if self.serial_number != 0 and self.serial_number != serial_number:
                    warnings.warn(
                        "LSS_SerialNumber in [DeviceComissioning] differs from serial number in identity object",
                        stacklevel=2,
                    )
                self.serial_number = serial_number

        self.error_behavior = {}
        if 0x1029 in self:
            for subobj in self[0x1029].values():
                if subobj.sub_index == 0:
                    continue
                self.error_behavior[subobj.sub_index] = subobj.parse_value()

        self.rpdo = {}
        for i in range(512):
            if 0x1400 + i in self:
                self.rpdo[i + 1] = PDO.from_device(self, 0x1400 + i)

        self.tpdo = {}
        for i in range(512):
            if 0x1800 + i in self:
                self.tpdo[i + 1] = PDO.from_device(self, 0x1800 + i)

    @classmethod
    def from_dcf(cls, filename: str, env: dict = {}) -> "Device":
        cfg = parse_file(filename)

        if not lint(cfg):
            raise ValueError("invalid DCF: " + filename)

        return cls(cfg, env)


class Object(dict):
    def __init__(self, cfg: dict, index: int, env: dict = None):
        self.cfg = cfg
        self.index = index
        self.env = env if env is not None else {}

        name = "{:04X}".format(index)
        section = self.cfg[name]

        self.name = section.get("Denotation", section["ParameterName"])
        self.object_type = 0x07
        if "ObjectType" in section and section["ObjectType"]:
            self.object_type = int(section["ObjectType"], 0)

        sub_number = 0
        if "SubNumber" in section and section["SubNumber"]:
            sub_number = int(section["SubNumber"], 0)
        compact_sub_obj = 0
        if "CompactSubObj" in section and section["CompactSubObj"]:
            compact_sub_obj = int(section["CompactSubObj"], 0)
        if sub_number != 0:
            for sub_index in range(255):
                sub_name = name + "sub{:X}".format(sub_index)
                if sub_name in self.cfg:
                    self[sub_index] = SubObject.from_section(
                        self.cfg, self.cfg[sub_name], self.index, sub_index, self.env
                    )
        elif compact_sub_obj != 0:
            # Add a sub-object containing the size of the array.
            self[0] = SubObject.from_number(
                self.cfg, self.index, compact_sub_obj, self.env
            )
            for sub_index in range(1, compact_sub_obj + 1):
                self[sub_index] = SubObject.from_compact_sub_obj(
                    self.cfg, self.index, sub_index, self.env
                )
        else:
            self[0] = SubObject.from_section(self.cfg, section, self.index, 0, self.env)

    @classmethod
    def from_dummy(cls, index: int, env: dict = None) -> "Object":
        cfg = {
            "{:04X}".format(index): {
                "ParameterName": DataType(index).name(),
                "ObjectType": "0x05",
                # According to CiA 301, the data type should be 0x0007 (UNSIGNED32) and
                # the value should be the length of the type in bits. However, that
                # would make the PDO mapping code more complex. This way we don't have
                # to treat dummy mappings different from regular mappings.
                "DataType": "0x{:04X}".format(index),
                "AccessType": "rw",
                "PDOMapping": "1",
            }
        }
        return cls(cfg, index, env)


class SubObject:
    def __init__(self, cfg: dict, index: int, sub_index: int, env: dict = {}):
        self.cfg = cfg
        self.index = index
        self.sub_index = sub_index
        self.env = env

    def parse_value(self):
        return self.value.parse(self.env)

    @classmethod
    def from_section(
        cls, cfg: dict, section: dict, index: int, sub_index: int, env: dict
    ) -> "SubObject":
        subobj = cls(cfg, index, sub_index, env)

        subobj.name = section.get("Denotation", section["ParameterName"])
        subobj.access_type = AccessType(section["AccessType"])
        subobj.data_type = DataType(int(section["DataType"], 0))

        if subobj.data_type.is_basic():
            if "LowLimit" in section and section["LowLimit"]:
                subobj.low_limit = Value(subobj.data_type, section["LowLimit"])
            if "HighLimit" in section and section["HighLimit"]:
                subobj.high_limit = Value(subobj.data_type, section["HighLimit"])

        default_value = subobj.data_type.default_value()
        if "DefaultValue" in section and section["DefaultValue"]:
            default_value = section["DefaultValue"]
        value = default_value
        if "ParameterValue" in section and section["ParameterValue"]:
            value = subobj.parameter_value = section["ParameterValue"]
        subobj.default_value = Value(subobj.data_type, default_value)
        subobj.value = Value(subobj.data_type, value)

        subobj.pdo_mapping = False
        if "PDOMapping" in section and section["PDOMapping"]:
            subobj.pdo_mapping = bool(int(section["PDOMapping"], 0))

        if subobj.data_type.index == 0x000F:
            if "UploadFile" in section and section["UploadFile"]:
                subobj.upload_file = section["UploadFile"]
            if "DownloadFile" in section and section["DownloadFile"]:
                subobj.download_file = section["DownloadFile"]

        return subobj

    @classmethod
    def from_number(cls, cfg: dict, index: int, number: int, env: dict) -> "SubObject":
        subobj = cls(cfg, index, 0, env)

        subobj.name = "NrOfObjects"

        subobj.access_type = AccessType("ro")
        subobj.data_type = DataType(0x0005)  # UNSIGNED8

        subobj.default_value = Value(subobj.data_type, str(number))
        subobj.value = subobj.default_value

        subobj.pdo_mapping = False

        return subobj

    @classmethod
    def from_compact_sub_obj(
        cls, cfg: dict, index: int, sub_index: int, env: dict
    ) -> "SubObject":
        subobj = cls(cfg, index, sub_index, env)

        name = "{:04X}".format(index)
        section = cfg[name]

        if name + "Name" in cfg and str(sub_index) in cfg[name + "Name"]:
            subobj.name = cfg[name + "Name"][str(sub_index)]
        else:
            subobj.name = section.get("Denotation", section["ParameterName"]) + str(
                sub_index
            )

        subobj.access_type = AccessType(section["AccessType"])
        subobj.data_type = DataType(int(section["DataType"], 0))

        default_value = subobj.data_type.default_value()
        if "DefaultValue" in section and section["DefaultValue"]:
            default_value = section["DefaultValue"]
        value = default_value
        if name + "Value" in cfg:
            if (
                str(sub_index) in cfg[name + "Value"]
                and cfg[name + "Value"][str(sub_index)]
            ):
                value = cfg[name + "Value"][str(sub_index)]
        elif "ParameterValue" in section and section["ParameterValue"]:
            value = subobj.parameter_value = True
        subobj.default_value = Value(subobj.data_type, default_value)
        subobj.value = Value(subobj.data_type, value)

        subobj.pdo_mapping = False
        if "PDOMapping" in section and section["PDOMapping"]:
            subobj.pdo_mapping = bool(int(section["PDOMapping"], 0))

        return subobj


class AccessType:
    def __init__(self, access_type: str):
        self.name = access_type.lower()

        self.read = False
        self.write = False
        self.tpdo = False
        self.rpdo = False

        if self.name == "ro" or self.name == "const":
            self.read = True
        elif self.name == "wo":
            self.write = True
        elif self.name == "rw":
            self.read = True
            self.write = True
        elif self.name == "rwr":
            self.read = True
            self.write = True
            self.tpdo = True
        elif self.name == "rww":
            self.read = True
            self.write = True
            self.rpdo = True
        else:
            raise ValueError("invalid AccessType: " + access_type)


def _parse_int(value: str):
    return int(value, 0)


def _parse_real32(value: str):
    value = "{:08X}".format(int(value, 0))
    return float(struct.unpack("!f", bytes.fromhex(value))[0])


def _parse_real64(value: str):
    value = "{:016X}".format(int(value, 0))
    return float(struct.unpack("!d", bytes.fromhex(value))[0])


def _parse_string(value: str):
    # VISIBLE_STRING and UNICODE_STRING values are interpreted as Python string literals.
    return ast.literal_eval(value)


def _parse_hex(value: str):
    # OCTET_STRING and DOMAIN values are specified as hexadecimal strings.
    return bytes.fromhex(value)


def _parse_time(value: str):
    # TIME_OF_DAY and TIME_DIFFERENCE values are tuples of days and milliseconds.
    if not value:
        value = "0 0"
    days, ms = value.split()
    return (int(days, 0), int(ms, 0))


class DataType:
    __name = {
        0x0001: "BOOLEAN",
        0x0002: "INTEGER8",
        0x0003: "INTEGER16",
        0x0004: "INTEGER32",
        0x0005: "UNSIGNED8",
        0x0006: "UNSIGNED16",
        0x0007: "UNSIGNED32",
        0x0008: "REAL32",
        0x0009: "VISIBLE_STRING",
        0x000A: "OCTET_STRING",
        0x000B: "UNICODE_STRING",
        0x000C: "TIME_OF_DAY",
        0x000D: "TIME_DIFF",
        0x000F: "DOMAIN",
        0x0010: "INTEGER24",
        0x0011: "REAL64",
        0x0012: "INTEGER40",
        0x0013: "INTEGER48",
        0x0014: "INTEGER56",
        0x0015: "INTEGER64",
        0x0016: "UNSIGNED24",
        0x0018: "UNSIGNED40",
        0x0019: "UNSIGNED48",
        0x001A: "UNSIGNED56",
        0x001B: "UNSIGNED64",
    }

    __parse = {
        0x0001: _parse_int,  # BOOLEAN
        0x0002: _parse_int,  # INTEGER8
        0x0003: _parse_int,  # INTEGER16
        0x0004: _parse_int,  # INTEGER32
        0x0005: _parse_int,  # UNSIGNED8
        0x0006: _parse_int,  # UNSIGNED16
        0x0007: _parse_int,  # UNSIGNED32
        0x0008: _parse_real32,  # REAL32
        0x0009: _parse_string,  # VISIBLE_STRING
        0x000A: _parse_hex,  # OCTET_STRING
        0x000B: _parse_string,  # UNICODE_STRING
        0x000C: _parse_time,  # TIME_OF_DAY
        0x000D: _parse_time,  # TIME_DIFF
        0x000F: _parse_hex,  # DOMAIN
        0x0010: _parse_int,  # INTEGER24
        0x0011: _parse_real64,  # REAL64
        0x0012: _parse_int,  # INTEGER40
        0x0013: _parse_int,  # INTEGER48
        0x0014: _parse_int,  # INTEGER56
        0x0015: _parse_int,  # INTEGER64
        0x0016: _parse_int,  # UNSIGNED24
        0x0018: _parse_int,  # UNSIGNED40
        0x0019: _parse_int,  # UNSIGNED48
        0x001A: _parse_int,  # UNSIGNED56
        0x001B: _parse_int,  # UNSIGNED64
    }

    __bits = {
        0x0001: 1,  # BOOLEAN
        0x0002: 8,  # INTEGER8
        0x0003: 16,  # INTEGER16
        0x0004: 32,  # INTEGER32
        0x0005: 8,  # UNSIGNED8
        0x0006: 16,  # UNSIGNED16
        0x0007: 32,  # UNSIGNED32
        0x0008: 32,  # REAL32
        0x0010: 24,  # INTEGER24
        0x0011: 64,  # REAL64
        0x0012: 40,  # INTEGER40
        0x0013: 48,  # INTEGER48
        0x0014: 56,  # INTEGER56
        0x0015: 64,  # INTEGER64
        0x0016: 24,  # UNSIGNED24
        0x0018: 40,  # UNSIGNED40
        0x0019: 48,  # UNSIGNED48
        0x001A: 56,  # UNSIGNED56
        0x001B: 64,  # UNSIGNED64
    }

    __min = {
        0x0001: 0,  # BOOLEAN
        0x0002: -0x80,  # INTEGER8
        0x0003: -0x8000,  # INTEGER16
        0x0004: -0x80000000,  # INTEGER32
        0x0005: 0,  # UNSIGNED8
        0x0006: 0,  # UNSIGNED16
        0x0007: 0,  # UNSIGNED32
        0x0008: -3.40282346638528859811704183484516925e38,  # REAL32
        0x0010: -0x800000,  # INTEGER24
        0x0011: -1.79769313486231570814527423731704357e308,  # REAL64
        0x0012: -0x8000000000,  # INTEGER40
        0x0013: -0x800000000000,  # INTEGER48
        0x0014: -0x80000000000000,  # INTEGER56
        0x0015: -0x8000000000000000,  # INTEGER64
        0x0016: 0,  # UNSIGNED24
        0x0018: 0,  # UNSIGNED40
        0x0019: 0,  # UNSIGNED48
        0x001A: 0,  # UNSIGNED56
        0x001B: 0,  # UNSIGNED64
    }

    __max = {
        0x0001: 1,  # BOOLEAN
        0x0002: 0x7F,  # INTEGER8
        0x0003: 0x7FFF,  # INTEGER16
        0x0004: 0x7FFFFFFF,  # INTEGER32
        0x0005: 0xFF,  # UNSIGNED8
        0x0006: 0xFFFF,  # UNSIGNED16
        0x0007: 0xFFFFFFFF,  # UNSIGNED32
        0x0008: 3.40282346638528859811704183484516925e38,  # REAL32
        0x0010: 0x7FFFFF,  # INTEGER24
        0x0011: 1.79769313486231570814527423731704357e308,  # REAL64
        0x0012: 0x7FFFFFFFFF,  # INTEGER40
        0x0013: 0x7FFFFFFFFFFF,  # INTEGER48
        0x0014: 0x7FFFFFFFFFFFFF,  # INTEGER56
        0x0015: 0x7FFFFFFFFFFFFFFF,  # INTEGER64
        0x0016: 0xFFFFFF,  # UNSIGNED24
        0x0018: 0xFFFFFFFFFF,  # UNSIGNED40
        0x0019: 0xFFFFFFFFFFFF,  # UNSIGNED48
        0x001A: 0xFFFFFFFFFFFFFF,  # UNSIGNED56
        0x001B: 0xFFFFFFFFFFFFFFFF,  # UNSIGNED64
    }

    __fmt = {
        0x0001: "<HBLB",  # BOOLEAN
        0x0002: "<HBLb",  # INTEGER8
        0x0003: "<HBLh",  # INTEGER16
        0x0004: "<HBLl",  # INTEGER32
        0x0005: "<HBLB",  # UNSIGNED8
        0x0006: "<HBLH",  # UNSIGNED16
        0x0007: "<HBLL",  # UNSIGNED32
        0x0008: "<HBLf",  # REAL32
        0x0010: "<HBLl",  # INTEGER24
        0x0011: "<HBLd",  # REAL64
        0x0012: "<HBLq",  # INTEGER40
        0x0013: "<HBLq",  # INTEGER48
        0x0014: "<HBLq",  # INTEGER56
        0x0015: "<HBLq",  # INTEGER64
        0x0016: "<HBLL",  # UNSIGNED24
        0x0018: "<HBLQ",  # UNSIGNED40
        0x0019: "<HBLQ",  # UNSIGNED48
        0x001A: "<HBLQ",  # UNSIGNED56
        0x001B: "<HBLQ",  # UNSIGNED64
    }

    def __init__(self, index: int):
        self.index = index

    def name(self):
        return DataType.__name[self.index]

    def is_basic(self):
        return self.index in DataType.__bits.keys()

    def is_array(self):
        if (
            self.index == 0x0009
            or self.index == 0x000A
            or self.index == 0x000B
            or self.index == 0x000F
        ):
            return True
        else:
            return False

    def bits(self):
        return DataType.__bits[self.index]

    def min(self):
        return DataType.__min[self.index]

    def max(self):
        return DataType.__max[self.index]

    def default_value(self):
        if self.is_basic():
            return "0"
        elif self.index == 0x0009 or self.index == 0x000B:
            return '""'
        else:
            return ""

    def parse_value(self, value: str):
        return DataType.__parse[self.index](value)

    def concise_value(self, index: int, sub_index: int, value):
        fmt = DataType.__fmt[self.index]
        n = int((DataType.__bits[self.index] + 7) / 8)
        if self.index == 0x0008 or self.index == 0x0011:
            return struct.pack(fmt, index, sub_index, n, float(value))
        else:
            return struct.pack(fmt, index, sub_index, n, int(value))

    @staticmethod
    def add_custom(index: int, name: str, parse):
        DataType.__name[index] = name
        DataType.__parse[index] = parse


class Value:
    __p_value = re.compile(
        r"(\$(?P<variable>NODEID)\s*\+\s*)?(?P<value>(-?0x[0-9A-F]+)|(-?[0-9]+))$",
        re.IGNORECASE,
    )

    def __init__(self, data_type: DataType, value: str = ""):
        self.data_type = data_type
        self.value = value if value else self.data_type.default_value()
        self.variable = None

        if self.data_type.is_basic():
            if self.value.upper() == "$NODEID":
                self.variable = self.value[1:]
                self.value = "0"
            else:
                m = Value.__p_value.match(self.value)
                if m:
                    self.value = m.group("value")
                    self.variable = m.group("variable")
                else:
                    raise ValueError("invalid value: " + self.value)

    def parse(self, env: dict = {}):
        value = self.data_type.parse_value(self.value)
        if self.data_type.is_basic():
            offset = 0
            if self.variable is not None:
                if self.variable.upper() in env:
                    offset = env[self.variable.upper()]
                else:
                    raise KeyError("$" + self.variable + " not defined")
            value += offset
        return value

    def has_nodeid(self):
        return self.variable is not None and self.variable.upper() == "NODEID"


class PDO:
    def __init__(self):
        self.cob_id = 0x80000000
        self.transmission_type = 0
        self.inhibit_time = 0
        self.event_timer = 0
        self.event_deadline = 0
        self.sync_start_value = 0
        self.n = 0
        self.mapping = {}

    @classmethod
    def from_device(cls, dev: Device, index: int) -> "PDO":
        pdo = cls()

        is_tpdo = (index & 0xFE00) == 0x1800

        pdo_comm = dev[index]
        n = 0
        if 0 in pdo_comm:
            n = pdo_comm[0].parse_value()
        if n >= 1 and 1 in pdo_comm:
            pdo.cob_id = pdo_comm[1].parse_value()
        if n >= 2 and 2 in pdo_comm:
            pdo.transmission_type = pdo_comm[2].parse_value()
        if n >= 3 and 3 in pdo_comm and is_tpdo:
            pdo.inhibit_time = pdo_comm[3].parse_value()
        if n >= 5 and 5 in pdo_comm:
            if is_tpdo:
                pdo.event_timer = pdo_comm[5].parse_value()
            else:
                pdo.event_deadline = pdo_comm[5].parse_value()
        if n >= 6 and 6 in pdo_comm and is_tpdo:
            pdo.sync_start_value = pdo_comm[6].parse_value()

        pdo_mapping = dev[index + 0x200]
        if 0 in pdo_mapping:
            pdo.n = pdo_mapping[0].parse_value()
        for i in range(1, pdo.n + 1):
            if i in pdo_mapping:
                value = pdo_mapping[i].parse_value()
                if value == 0:
                    continue
                index = (value >> 16) & 0xFFFF
                sub_index = (value >> 8) & 0xFF
                if index < 0x1000:
                    pdo.mapping[i] = Object.from_dummy(index)[sub_index]
                else:
                    pdo.mapping[i] = dev[index][sub_index]

        return pdo
