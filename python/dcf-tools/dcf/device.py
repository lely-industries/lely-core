import re
import struct
import warnings

from .lint import lint
from .parse import parse_file


class Device(dict):
    def __init__(self, cfg: dict, env: dict = {}):
        self.cfg = cfg
        self.env = env

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
    def from_dcf(cls, filename: str, env: dict = {}) -> "Master":
        cfg = parse_file(filename)

        if not lint(cfg):
            raise ValueError("invalid DCF: " + filename)

        return cls(cfg, env)


class Object(dict):
    def __init__(self, cfg: dict, index: int, env: dict = {}):
        self.cfg = cfg
        self.index = index
        self.env = env

        name = "{:04X}".format(index)
        section = self.cfg[name]

        self.name = section.get("Denotation", section["ParameterName"])

        sub_number = int(section.get("SubNumber", "0"), 0)
        compact_sub_obj = int(section.get("CompactSubObj", "0"), 0)
        if sub_number != 0:
            for sub_index in range(255):
                sub_name = name + "sub{:X}".format(sub_index)
                if sub_name in self.cfg:
                    self[sub_index] = SubObject.from_section(
                        self.cfg, self.cfg[sub_name], self.index, sub_index, self.env
                    )
        elif compact_sub_obj != 0:
            for sub_index in range(1, compact_sub_obj + 1):
                self[sub_index] = SubObject.from_compact_sub_obj(
                    self.cfg, self.index, sub_index, self.env
                )
        else:
            self[0] = SubObject.from_section(self.cfg, section, self.index, 0, self.env)

    @classmethod
    def from_dummy(cls, index: int, env: dict = {}) -> "Object":
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
        return self.data_type.parse_value(self.value, self.env)

    @classmethod
    def from_section(
        cls, cfg: dict, section: dict, index: int, sub_index: int, env: dict
    ) -> "SubObject":
        subobj = cls(cfg, index, sub_index, env)

        subobj.name = section.get("Denotation", section["ParameterName"])
        subobj.access_type = AccessType(section["AccessType"])
        subobj.data_type = DataType(int(section["DataType"], 0))
        subobj.value = section.get("ParameterValue", section.get("DefaultValue", "0"))
        subobj.pdo_mapping = bool(int(section.get("PDOMapping", "0"), 2))

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

        if name + "Value" in cfg and str(sub_index) in cfg[name + "Value"]:
            subobj.value = cfg[name + "Value"][str(sub_index)]
        else:
            subobj.value = section.get(
                "ParameterValue", section.get("DefaultValue", "0")
            )

        subobj.pdo_mapping = bool(int(section.get("PDOMapping", "0"), 2))

        return subobj


class AccessType:
    def __init__(self, access_type: str):
        self.read = False
        self.write = False

        if access_type.lower() == "ro" or access_type.lower() == "const":
            self.read = True
        elif access_type.lower() == "wo":
            self.write = True
        elif (
            access_type.lower() == "rw"
            or access_type.lower() == "rwr"
            or access_type.lower() == "rww"
        ):
            self.read = True
            self.write = True
        else:
            raise ValueError("invalid AccessType: " + access_type)


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

    __fmt = {
        0x0001: "<HBLB",  # BOOLEAN
        0x0002: "<HBLB",  # INTEGER8
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

    __p_value = re.compile(
        r"\s*(\$(?P<variable>NODEID)\s*\+\s*)?(?P<value>(-?0x[0-9A-F]+)|(-?[0-9]+))$",
        re.IGNORECASE,
    )

    def __init__(self, index: int):
        self.index = index

    def name(self):
        return DataType.__name[self.index]

    def bits(self):
        return DataType.__bits[self.index]

    def concise_value(self, index: int, sub_index: int, value):
        fmt = DataType.__fmt[self.index]
        n = int((DataType.__bits[self.index] + 7) / 8)
        if self.index == 0x0008 or self.index == 0x0011:
            return struct.pack(fmt, index, sub_index, n, float(value))
        else:
            return struct.pack(fmt, index, sub_index, n, int(value))

    def parse_value(self, value: str, env: dict = {}):
        m = DataType.__p_value.match(value)
        if m:
            offset = 0
            variable = m.group("variable")
            if variable is not None:
                if variable.upper() in env:
                    offset = env[variable.upper()]
                else:
                    raise KeyError("$" + variable + " not defined")
            return offset + int(m.group("value"), 0)
        else:
            raise ValueError("invalid value: " + value)


class PDO:
    def __init__(self):
        self.cob_id = 0x80000000
        self.transmission_type = 0
        self.inhibit_time = 0
        self.event_timer = 0
        self.event_deadline = 0
        self.sync_start_value = 0
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
        n = 64
        if 0 in pdo_mapping:
            n = pdo_mapping[0].parse_value()
        for i in range(1, n + 1):
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
