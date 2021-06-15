import collections
import configparser
import os


class IniDict(collections.abc.MutableMapping):
    def __init__(
        self, dict_type=collections.OrderedDict, key_xform=lambda str: str.lower()
    ):
        self.__dict = dict_type()
        self.key_xform = key_xform

    def __delitem__(self, key):
        self.__dict.__delitem__(self.key_xform(key))

    def __getitem__(self, key):
        key, value = self.__dict.__getitem__(self.key_xform(key))
        return value

    def __iter__(self):
        for value, _ in self.__dict.values():
            yield value

    def __len__(self):
        return self.__dict.__len__()

    def __setitem__(self, key, value):
        self.__dict.__setitem__(self.key_xform(key), (key, value))

    def copy(self):
        copy = self.__class__(self.__dict.__class__, self.key_xform)
        copy.__dict = self.__dict.copy()
        return copy


def parse_file(filename: str) -> dict:
    cfg = configparser.ConfigParser(
        dict_type=IniDict, inline_comment_prefixes=("#", ";"), interpolation=None
    )
    cfg.optionxform = str  # case sensitive
    with open(os.path.expandvars(filename), "r") as f:
        cfg.read_file(f, filename)

    __add_compact_rpdo(cfg)
    __add_compact_tpdo(cfg)

    return cfg


def __add_compact_rpdo(cfg: dict):
    if "DeviceInfo" not in cfg:
        return
    section = cfg["DeviceInfo"]
    compact_pdo = 0
    if "CompactPDO" in section and section["CompactPDO"]:
        compact_pdo = int(section["CompactPDO"], 0)
    if compact_pdo == 0:
        return
    # Count the number of explicit Receive-PDOs.
    npdo = 0
    for i in range(512):
        if "{:04X}".format(0x1400 + i) in cfg:
            npdo += 1
    # Create implicit Receive-PDOs, if necessary.
    nr_of_rx_pdo = 0
    if "NrOfRxPDO" in section and section["NrOfRxPDO"]:
        nr_of_rx_pdo = int(section["NrOfRxPDO"], 0)
    for i in range(512):
        if nr_of_rx_pdo <= npdo:
            break

        # Check if the communication parameters already exist.
        name = "{:04X}".format(0x1400 + i)
        if name in cfg:
            continue
        npdo += 1

        n = int(cfg["OptionalObjects"]["SupportedObjects"], 0)
        cfg["OptionalObjects"]["SupportedObjects"] = str(n + 1)
        cfg["OptionalObjects"][str(n + 1)] = "0x" + name

        cfg[name] = {}
        obj = cfg[name]
        obj["SubNumber"] = "1"
        obj["ParameterName"] = "RPDO communication parameter"
        obj["ObjectType"] = "0x09"

        cfg[name + "sub0"] = {}
        subobj = cfg[name + "sub0"]
        subobj["ParameterName"] = "highest sub-index supported"
        subobj["DataType"] = "0x0005"
        subobj["AccessType"] = "const"

        sub_number = 1
        sub_index = 0
        if compact_pdo & 0x01:
            sub_number += 1
            sub_index = 1
            cfg[name + "sub1"] = {}
            subobj = cfg[name + "sub1"]
            subobj["ParameterName"] = "COB-ID used by RPDO"
            subobj["DataType"] = "0x0007"
            subobj["AccessType"] = "rw"
            if i < 4:
                subobj["DefaultValue"] = "$NODEID+0x{:X}".format(
                    (i + 1) * 0x100 + 0x100
                )
            else:
                subobj["DefaultValue"] = "0x80000000"
        if compact_pdo & 0x02:
            sub_number += 1
            sub_index = 2
            cfg[name + "sub2"] = {}
            subobj = cfg[name + "sub2"]
            subobj["ParameterName"] = "transmission type"
            subobj["DataType"] = "0x0005"
            subobj["AccessType"] = "rw"
        if compact_pdo & 0x04:
            sub_number += 1
            sub_index = 3
            cfg[name + "sub3"] = {}
            subobj = cfg[name + "sub3"]
            subobj["ParameterName"] = "inhibit time"
            subobj["DataType"] = "0x0006"
            subobj["AccessType"] = "rw"
        if compact_pdo & 0x08:
            sub_number += 1
            sub_index = 4
            cfg[name + "sub4"] = {}
            subobj = cfg[name + "sub4"]
            subobj["ParameterName"] = "compatibility entry"
            subobj["DataType"] = "0x0005"
            subobj["AccessType"] = "rw"
        if compact_pdo & 0x10:
            sub_number += 1
            sub_index = 5
            cfg[name + "sub5"] = {}
            subobj = cfg[name + "sub5"]
            subobj["ParameterName"] = "event-timer"
            subobj["DataType"] = "0x0006"
            subobj["AccessType"] = "rw"
        if compact_pdo & 0x20:
            sub_number += 1
            sub_index = 6
            cfg[name + "sub6"] = {}
            subobj = cfg[name + "sub6"]
            subobj["ParameterName"] = "SYNC start value"
            subobj["DataType"] = "0x0005"
            subobj["AccessType"] = "rw"

        cfg[name]["SubNumber"] = str(sub_number)
        cfg[name + "sub0"]["DefaultValue"] = str(sub_index)

        # Add the mapping parameters, if necessary.
        name = "{:04X}".format(0x1600 + i)
        if name not in cfg:
            cfg["OptionalObjects"]["SupportedObjects"] = str(n + 2)
            cfg["OptionalObjects"][str(n + 2)] = "0x" + name

            cfg[name] = {}
            obj = cfg[name]
            obj["ParameterName"] = "RPDO mapping parameter"
            obj["ObjectType"] = "0x09"
            obj["DataType"] = "0x0007"
            obj["AccessType"] = "rw"
            obj["CompactSubObj"] = "0x40"


def __add_compact_tpdo(cfg: dict):
    if "DeviceInfo" not in cfg:
        return
    section = cfg["DeviceInfo"]
    compact_pdo = 0
    if "CompactPDO" in section and section["CompactPDO"]:
        compact_pdo = int(section["CompactPDO"], 0)
    if compact_pdo == 0:
        return
    # Count the number of explicit Transmit-PDOs.
    npdo = 0
    for i in range(512):
        if "{:04X}".format(0x1800 + i) in cfg:
            npdo += 1
    # Create implicit Transmit-PDOs, if necessary.
    nr_of_tx_pdo = 0
    if "NrOfTxPDO" in section and section["NrOfTxPDO"]:
        nr_of_tx_pdo = int(section["NrOfTxPDO"], 0)
    for i in range(512):
        if nr_of_tx_pdo <= npdo:
            break

        # Check if the communication parameters already exist.
        name = "{:04X}".format(0x1800 + i)
        if name in cfg:
            continue
        npdo += 1

        n = int(cfg["OptionalObjects"]["SupportedObjects"], 0)
        cfg["OptionalObjects"]["SupportedObjects"] = str(n + 1)
        cfg["OptionalObjects"][str(n + 1)] = "0x" + name

        cfg[name] = {}
        obj = cfg[name]
        obj["SubNumber"] = "1"
        obj["ParameterName"] = "TPDO communication parameter"
        obj["ObjectType"] = "0x09"

        cfg[name + "sub0"] = {}
        subobj = cfg[name + "sub0"]
        subobj["ParameterName"] = "highest sub-index supported"
        subobj["DataType"] = "0x0005"
        subobj["AccessType"] = "const"

        sub_number = 1
        sub_index = 0
        if compact_pdo & 0x01:
            sub_number += 1
            sub_index = 1
            cfg[name + "sub1"] = {}
            subobj = cfg[name + "sub1"]
            subobj["ParameterName"] = "COB-ID used by TPDO"
            subobj["DataType"] = "0x0007"
            subobj["AccessType"] = "rw"
            if i < 4:
                subobj["DefaultValue"] = "$NODEID+0x{:X}".format((i + 1) * 0x100 + 0x80)
            else:
                subobj["DefaultValue"] = "0x80000000"
        if compact_pdo & 0x02:
            sub_number += 1
            sub_index = 2
            cfg[name + "sub2"] = {}
            subobj = cfg[name + "sub2"]
            subobj["ParameterName"] = "transmission type"
            subobj["DataType"] = "0x0005"
            subobj["AccessType"] = "rw"
        if compact_pdo & 0x04:
            sub_number += 1
            sub_index = 3
            cfg[name + "sub3"] = {}
            subobj = cfg[name + "sub3"]
            subobj["ParameterName"] = "inhibit time"
            subobj["DataType"] = "0x0006"
            subobj["AccessType"] = "rw"
        if compact_pdo & 0x08:
            sub_number += 1
            sub_index = 4
            cfg[name + "sub4"] = {}
            subobj = cfg[name + "sub4"]
            subobj["ParameterName"] = "reserved"
            subobj["DataType"] = "0x0005"
            subobj["AccessType"] = "rw"
        if compact_pdo & 0x10:
            sub_number += 1
            sub_index = 5
            cfg[name + "sub5"] = {}
            subobj = cfg[name + "sub5"]
            subobj["ParameterName"] = "event timer"
            subobj["DataType"] = "0x0006"
            subobj["AccessType"] = "rw"
        if compact_pdo & 0x20:
            sub_number += 1
            sub_index = 6
            cfg[name + "sub6"] = {}
            subobj = cfg[name + "sub6"]
            subobj["ParameterName"] = "SYNC start value"
            subobj["DataType"] = "0x0005"
            subobj["AccessType"] = "rw"

        cfg[name]["SubNumber"] = str(sub_number)
        cfg[name + "sub0"]["DefaultValue"] = str(sub_index)

        # Add the mapping parameters, if necessary.
        name = "{:04X}".format(0x1A00 + i)
        if name not in cfg:
            cfg["OptionalObjects"]["SupportedObjects"] = str(n + 2)
            cfg["OptionalObjects"][str(n + 2)] = "0x" + name

            cfg[name] = {}
            obj = cfg[name]
            obj["ParameterName"] = "TPDO mapping parameter"
            obj["ObjectType"] = "0x09"
            obj["DataType"] = "0x0007"
            obj["AccessType"] = "rw"
            obj["CompactSubObj"] = "0x40"
