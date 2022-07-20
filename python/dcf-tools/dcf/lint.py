import re
import struct
import warnings


__sections = {
    "FileInfo".lower(): [
        "FileName".lower(),
        "FileVersion".lower(),
        "FileRevision".lower(),
        "EDSVersion".lower(),
        "Description".lower(),
        "CreationTime".lower(),
        "CreationDate".lower(),
        "CreatedBy".lower(),
        "ModificationTime".lower(),
        "ModificationDate".lower(),
        "ModifiedBy".lower(),
        "LastEDS".lower(),
    ],
    "DeviceComissioning".lower(): [
        "NodeID".lower(),
        "NodeName".lower(),
        "NodeRefd".lower(),
        "Baudrate".lower(),
        "NetNumber".lower(),
        "NetworkName".lower(),
        "NetRefd".lower(),
        "CANopenManager".lower(),
        "LSS_SerialNumber".lower(),
    ],
    "DeviceInfo".lower(): [
        "VendorName".lower(),
        "VendorNumber".lower(),
        "ProductName".lower(),
        "ProductNumber".lower(),
        "RevisionNumber".lower(),
        "OrderCode".lower(),
        "BaudRate_10".lower(),
        "BaudRate_20".lower(),
        "BaudRate_50".lower(),
        "BaudRate_125".lower(),
        "BaudRate_250".lower(),
        "BaudRate_500".lower(),
        "BaudRate_800".lower(),
        "BaudRate_1000".lower(),
        "SimpleBootUpMaster".lower(),
        "SimpleBootUpSlave".lower(),
        "Granularity".lower(),
        "DynamicChannelsSupported".lower(),
        "GroupMessaging".lower(),
        "NrOfRxPDO".lower(),
        "NrOfTxPDO".lower(),
        "LSS_Supported".lower(),
        "CompactPDO".lower(),
    ],
}

__p_object = re.compile(r"([0-9A-X]{4})(Name|Value|sub([0-9A-F]+))?$", re.IGNORECASE)


def lint(cfg: dict) -> bool:
    ok = True
    for section in cfg:
        if section == "DEFAULT":
            pass
        elif section.lower() in __sections.keys():
            for entry in cfg[section]:
                if entry.lower() in __sections[section.lower()]:
                    pass
                else:
                    warnings.warn(
                        "invalid entry in [{}]: {}".format(section, entry), stacklevel=2
                    )
                    ok = False
        elif section.lower() == "DummyUsage".lower():
            if not __parse_dummy_usage(cfg, section):
                ok = False
        elif section.lower() == "Comments".lower():
            pass
        elif (
            section.lower() == "MandatoryObjects".lower()
            or section.lower() == "OptionalObjects".lower()
            or section.lower() == "ManufacturerObjects".lower()
        ):
            if not __parse_objects(cfg, section):
                ok = False
        else:
            m = __p_object.match(section)
            if m:
                index = int(m.group(1), 16)
                if m.group(2) is None:
                    if not __parse_object(cfg, section, index):
                        ok = False
                elif m.group(3) is not None:
                    sub_index = int(m.group(3), 16)
                    if not __parse_sub_object(cfg, section, index, sub_index):
                        ok = False
                else:
                    if not __parse_compact_sub_object(cfg, section, index):
                        ok = False
            else:
                warnings.warn("unknown section in DCF: " + section, stacklevel=2)
                ok = False
    return ok


__p_dummy_usage = re.compile(r"Dummy([0-9A-F]{4})$", re.IGNORECASE)


def __parse_dummy_usage(cfg: dict, section: str) -> bool:
    ok = True
    for entry in cfg[section]:
        if __p_dummy_usage.match(entry):
            try:
                int(cfg[section][entry], 2)
            except ValueError:
                warnings.warn(
                    "invalid value for {} in [{}]: {}".format(
                        entry, section, cfg[section][entry]
                    ),
                    stacklevel=3,
                )
                ok = False
        else:
            warnings.warn(
                "invalid entry in [{}]: {}".format(section, entry), stacklevel=3
            )
            ok = False
    return ok


def __parse_objects(cfg: dict, section: str) -> bool:
    ok = True

    supported_objects = None
    for entry in cfg[section]:
        if entry.lower() == "SupportedObjects".lower():
            try:
                supported_objects = int(cfg[section][entry], 10)
            except ValueError:
                warnings.warn(
                    "invalid value for SupportedObjects in [{}]: {}".format(
                        section, entry
                    ),
                    stacklevel=3,
                )
                ok = False
        else:
            try:
                int(entry, 10)
            except ValueError:
                warnings.warn(
                    "invalid entry in [{}]: {}".format(section, entry), stacklevel=3
                )
                ok = False

    if supported_objects is None:
        warnings.warn(
            "SupportedObjects entry missing in [" + section + "]", stacklevel=3
        )
        return False

    if supported_objects < len(cfg[section].keys()) - 1:
        warnings.warn("too many entries in [{}]".format(section), stacklevel=3)
        ok = False

    for i in range(1, supported_objects + 1):
        if str(i) in cfg[section]:
            index = cfg[section][str(i)]
            try:
                index = int(index, 0)
                if "{:04X}".format(index) not in cfg:
                    warnings.warn(
                        "object 0x{:04X} not found".format(index), stacklevel=3
                    )
                    ok = False
                if index < 0x1000:
                    warnings.warn(
                        "data type objects are not supported: 0x{:04X}".format(index),
                        stacklevel=3,
                    )
                    ok = False
                elif (
                    section.lower() == "MandatoryObjects".lower()
                    and index != 0x1000
                    and index != 0x1001
                    and index != 0x1018
                ):
                    warnings.warn(
                        "object 0x{:04X} is not mandatory".format(index), stacklevel=3
                    )
                    ok = False
                elif (
                    section.lower() == "OptionalObjects".lower()
                    and index >= 0x2000
                    and index < 0x6000
                ):
                    warnings.warn(
                        "object 0x{:04X} is manufacturer-specific".format(index),
                        stacklevel=3,
                    )
                    ok = False
                elif section.lower() == "ManufacturerObjects".lower() and (
                    index < 0x2000 or index >= 0x6000
                ):
                    warnings.warn(
                        "object 0x{:04X} is not manufacturer-specific".format(index),
                        stacklevel=3,
                    )
                    ok = False
            except ValueError:
                warnings.warn(
                    "invalid object index for entry {} in [{}]: {}".format(
                        i, section, index
                    ),
                    stacklevel=3,
                )
                ok = False
        else:
            warnings.warn("entry {} missing in [{}]".format(i, section), stacklevel=3)
            ok = False

    return ok


__object_entries = {
    "SubNumber".lower(): [],
    "ParameterName".lower(): [],
    "ObjectType".lower(): [],
    "DataType".lower(): [],
    "AccessType".lower(): [
        "ro".lower(),
        "wo".lower(),
        "rw".lower(),
        "rwr".lower(),
        "rww".lower(),
        "const".lower(),
    ],
    "LowLimit".lower(): [],
    "HighLimit".lower(): [],
    "DefaultValue".lower(): [],
    "PDOMapping".lower(): [],
    "ObjFlags".lower(): [],
    "CompactSubObj".lower(): [],
    "ParameterValue".lower(): [],
    "UploadFile".lower(): [],
    "DownloadFile".lower(): [],
    "Denotation".lower(): [],
    "ParamRefd".lower(): [],
}


def __parse_object(cfg: dict, section: str, index: int) -> bool:
    ok = True

    for entry in cfg[section]:
        if entry.lower() in __object_entries:
            if (
                __object_entries[entry.lower()]
                and cfg[section][entry].lower() not in __object_entries[entry.lower()]
            ):
                warnings.warn(
                    "invalid value for {} in [{}]: {}".format(
                        entry, section, cfg[section][entry]
                    ),
                    stacklevel=3,
                )
                ok = False
        else:
            warnings.warn(
                "invalid entry in [{}]: {}".format(section, entry), stacklevel=3
            )
            ok = False

    if "ParameterName" not in cfg[section]:
        warnings.warn(
            "ParameterName not specified in [{}]".format(section), stacklevel=3
        )
        ok = False

    object_type = 0x07
    if "ObjectType" in cfg[section] and cfg[section]["ObjectType"]:
        object_type = int(cfg[section]["ObjectType"], 0)
    if (object_type == 0x05 or object_type == 0x07) and (
        "DataType" not in cfg[section] or not cfg[section]["DataType"]
    ):
        warnings.warn("DataType not specified in [{}]".format(section), stacklevel=3)
        ok = False
    sub_number = 0
    if "SubNumber" in cfg[section] and cfg[section]["SubNumber"]:
        sub_number = int(cfg[section]["SubNumber"], 0)
    compact_sub_obj = 0
    if "CompactSubObj" in cfg[section] and cfg[section]["CompactSubObj"]:
        compact_sub_obj = int(cfg[section]["CompactSubObj"], 0)
    if "AccessType" in cfg[section] and cfg[section]["AccessType"]:
        if (
            object_type == 0x06 or object_type == 0x08 or object_type == 0x09
        ) and compact_sub_obj == 0:
            warnings.warn(
                "AccessType not supported in [{}]".format(section), stacklevel=3
            )
            ok = False
    elif object_type != 0x02 and compact_sub_obj != 0:
        warnings.warn("AccessType not specified in [{}]".format(section), stacklevel=3)
        ok = False
    if sub_number != 0 and compact_sub_obj != 0:
        warnings.warn(
            "SubNumber and CompactSubObj specified in [{}]".format(section),
            stacklevel=3,
        )
        ok = False
    elif sub_number != 0:
        if object_type != 0x08 and object_type != 0x09:
            warnings.warn(
                "ObjectType should be 0x08 (ARRAY) or 0x09 (RECORD) in [{}]".format(
                    section
                ),
                stacklevel=3,
            )
            ok = False
        n = 0
        for sub_index in range(255):
            sub_name = "{:04X}sub{:X}".format(index, sub_index)
            if sub_name in cfg:
                n += 1
                if sub_index == 0:
                    data_type = 0
                    if "DataType" in cfg[sub_name] and cfg[sub_name]["DataType"]:
                        data_type = int(cfg[sub_name]["DataType"], 0)
                    if data_type != 0x0005:
                        warnings.warn(
                            "DataType should be UNSIGNED8 in [{}]".format(sub_name),
                            stacklevel=3,
                        )
                        ok = False
        if n < sub_number:
            warnings.warn(
                "{} missing sub-object(s) for object 0x{:04X}".format(
                    sub_number - n, index
                ),
                stacklevel=3,
            )
            ok = False
        elif n > sub_number:
            warnings.warn(
                "{} extra sub-object(s) for object 0x{:04X}".format(
                    n - sub_number, index
                ),
                stacklevel=3,
            )
            ok = False
    elif compact_sub_obj != 0:
        if object_type != 0x08 and object_type != 0x09:
            warnings.warn(
                "ObjectType should be 0x08 (ARRAY) or 0x09 (RECORD) in [{}]".format(
                    section
                ),
                stacklevel=3,
            )
            ok = False

    if sub_number == 0:
        if not __parse_sub_object_0(cfg, section):
            ok = False

    return ok


def __parse_sub_object_0(cfg: dict, section: str) -> bool:
    ok = True

    if __parse_data_type(cfg, section):
        if "DefaultValue" in cfg[section] and cfg[section]["DefaultValue"]:
            if not __parse_value(
                cfg[section], section, "DefaultValue", cfg[section]["DefaultValue"]
            ):
                ok = False
        if "ParameterValue" in cfg[section] and cfg[section]["ParameterValue"]:
            if not __parse_value(
                cfg[section], section, "ParameterValue", cfg[section]["ParameterValue"]
            ):
                ok = False
    else:
        ok = False

    return ok


def __parse_sub_object(cfg: dict, section: str, index: int, sub_index: int) -> bool:
    ok = True

    name = "{:04X}".format(index)
    if name not in cfg:
        warnings.warn("object 0x{} not defined: {}".format(name, section), stacklevel=3)
        ok = False

    if sub_index > 254:
        warnings.warn("invalid sub-index: " + section, stacklevel=3)
        return False

    for entry in cfg[section]:
        if entry.lower() in __object_entries:
            if (
                __object_entries[entry.lower()]
                and cfg[section][entry].lower() not in __object_entries[entry.lower()]
            ):
                warnings.warn(
                    "invalid value for {} in [{}]: {}".format(
                        entry, section, cfg[section][entry]
                    ),
                    stacklevel=3,
                )
                ok = False
        else:
            warnings.warn(
                "invalid entry in [{}]: {}".format(section, entry), stacklevel=3
            )
            ok = False

    if "ParameterName" not in cfg[section]:
        warnings.warn(
            "ParameterName not specified in [{}]".format(section), stacklevel=3
        )
        ok = False

    if __parse_data_type(cfg, section):
        if "DefaultValue" in cfg[section] and cfg[section]["DefaultValue"]:
            if not __parse_value(
                cfg[section], section, "DefaultValue", cfg[section]["DefaultValue"]
            ):
                ok = False
        if "ParameterValue" in cfg[section] and cfg[section]["ParameterValue"]:
            if not __parse_value(
                cfg[section], section, "ParameterValue", cfg[section]["ParameterValue"]
            ):
                ok = False
    else:
        ok = False

    if "AccessType" not in cfg[section] or not cfg[section]["AccessType"]:
        warnings.warn("AccessType not specified in [{}]".format(section), stacklevel=3)
        ok = False

    return ok


def __parse_compact_sub_object(cfg: dict, section: str, index: int) -> bool:
    ok = True

    compact_sub_obj = 0
    name = "{:04X}".format(index)
    if name in cfg:
        if "CompactSubObj" in cfg[name] and cfg[name]["CompactSubObj"]:
            compact_sub_obj = int(cfg[name]["CompactSubObj"], 0)
        else:
            warnings.warn(
                "object 0x{} does not support compact storage".format(name),
                stacklevel=3,
            )
            ok = False
        if not __parse_data_type(cfg, name):
            ok = False
    else:
        warnings.warn("object 0x{} not defined: {}".format(name, section), stacklevel=3)
        ok = False

    nr_of_entries = None
    for entry in cfg[section]:
        if entry.lower() == "NrOfEntries".lower():
            try:
                nr_of_entries = int(cfg[section][entry], 10)
            except ValueError:
                warnings.warn(
                    "invalid value for NrOfEntries in [{}]: {}".format(section, entry),
                    stacklevel=3,
                )
                ok = False
        else:
            try:
                i = int(entry, 10)
                if i > compact_sub_obj:
                    warnings.warn(
                        "invalid sub-index in [{}]: {}".format(section, i), stacklevel=3
                    )
                    ok = False
            except ValueError:
                warnings.warn(
                    "invalid entry in [{}]: {}".format(section, entry), stacklevel=3
                )
                ok = False

    if nr_of_entries is None:
        warnings.warn("NrOfEntries entry missing in [" + section + "]", stacklevel=3)
        return False

    if nr_of_entries < len(cfg[section].keys()) - 1:
        warnings.warn("too many entries in [{}]".format(section), stacklevel=3)
        ok = False
    elif nr_of_entries > len(cfg[section].keys()) - 1:
        warnings.warn("too few entries in [{}]".format(section), stacklevel=3)
        ok = False

    if ok and section.lower().endswith("Value".lower()):
        for entry, value in cfg[section].items():
            if entry.lower() == "NrOfEntries".lower():
                continue
            if not __parse_value(cfg[name], section, "entry " + entry, value):
                ok = False

    return ok


__limits = {
    0x0001: [0, 1],  # BOOLEAN
    0x0002: [-0x80, 0x7F],  # INTEGER8
    0x0003: [-0x8000, 0x7FFF],  # INTEGER16
    0x0004: [-0x80000000, 0x7FFFFFFF],  # INTEGER32
    0x0005: [0, 0xFF],  # UNSIGNED8
    0x0006: [0, 0xFFFF],  # UNSIGNED16
    0x0007: [0, 0xFFFFFFFF],  # UNSIGNED32
    0x0008: [
        -3.40282346638528859811704183484516925e38,
        3.40282346638528859811704183484516925e38,
    ],  # REAL32
    0x0010: [-0x800000, 0x7FFFFF],  # INTEGER24
    0x0011: [
        -1.79769313486231570814527423731704357e308,
        1.79769313486231570814527423731704357e308,
    ],  # REAL64
    0x0012: [-0x8000000000, 0x7FFFFFFFFF],  # INTEGER40
    0x0013: [-0x800000000000, 0x7FFFFFFFFFFF],  # INTEGER48
    0x0014: [-0x80000000000000, 0x7FFFFFFFFFFFFF],  # INTEGER56
    0x0015: [-0x8000000000000000, 0x7FFFFFFFFFFFFFFF],  # INTEGER64
    0x0016: [0, 0xFFFFFF],  # UNSIGNED24
    0x0018: [0, 0xFFFFFFFFFF],  # UNSIGNED40
    0x0019: [0, 0xFFFFFFFFFFFF],  # UNSIGNED48
    0x001A: [0, 0xFFFFFFFFFFFFFF],  # UNSIGNED56
    0x001B: [0, 0xFFFFFFFFFFFFFFFF],  # UNSIGNED64
}


def __parse_data_type(cfg: dict, section: str) -> bool:
    ok = True

    if "DataType" not in cfg[section] or not cfg[section]["DataType"]:
        return ok

    try:
        data_type = int(cfg[section]["DataType"], 0)
    except ValueError:
        warnings.warn(
            "invalid DataType in [{}]: {}".format(section, cfg[section]["DataType"]),
            stacklevel=4,
        )
        ok = False

    if data_type in __limits.keys():
        if not __parse_limit(cfg, section, "LowLimit", data_type) or not __parse_limit(
            cfg, section, "HighLimit", data_type
        ):
            ok = False
    else:
        if "LowLimit" in cfg[section] and cfg[section]["LowLimit"]:
            warnings.warn("LowLimit not supported in [" + section + "]", stacklevel=4)
            ok = False
        if "HighLimit" in cfg[section] and cfg[section]["HighLimit"]:
            warnings.warn("HighLimit not supported in [" + section + "]", stacklevel=4)
            ok = False

    return ok


def __parse_float(value: str, data_type: int):
    if data_type == 0x0008:
        value = "{:08X}".format(int(value, 0))
        return float(struct.unpack("!f", bytes.fromhex(value))[0])
    elif data_type == 0x0011:
        value = "{:016X}".format(int(value, 0))
        return float(struct.unpack("!d", bytes.fromhex(value))[0])
    return 0


__p_value = re.compile(
    r"(\$(?P<variable>NODEID)\s*\+\s*)?(?P<value>(-?0x[0-9A-F]+)|(-?[0-9]+))$",
    re.IGNORECASE,
)


def __parse_limit(cfg: dict, section: str, entry: str, data_type: int) -> bool:
    if entry in cfg[section] and cfg[section][entry]:
        value = 0
        value_has_nodeid = False
        try:
            if data_type == 0x0008 or data_type == 0x0011:
                value = __parse_float(cfg[section][entry], data_type)
            elif cfg[section][entry].upper() == "$NODEID":
                value_has_nodeid = True
            else:
                m = __p_value.match(cfg[section][entry])
                if m:
                    value = int(m.group("value"), 0)
                    value_has_nodeid = m.group("variable") is not None
                else:
                    warnings.warn(
                        "invalid {} in [{}]: {}".format(
                            entry, section, cfg[section][entry]
                        ),
                        stacklevel=5,
                    )
        except ValueError:
            warnings.warn(
                "invalid {} in [{}]: {}".format(entry, section, cfg[section][entry]),
                stacklevel=5,
            )
            return False

        low_limit = __limits[data_type][0]
        if value_has_nodeid:
            low_limit -= 1
        if value < low_limit:
            warnings.warn("{} underflow in [{}]".format(entry, section), stacklevel=5)
            return False

        high_limit = __limits[data_type][1]
        if value_has_nodeid:
            high_limit -= 127
        if value > high_limit:
            warnings.warn("{} overflow in [{}]".format(entry, section), stacklevel=5)
            return False

    return True


def __parse_value(cfg: dict, section: str, entry: str, value: str) -> bool:
    data_type = int(cfg["DataType"], 0)
    if data_type in __limits.keys():
        low_limit = __limits[data_type][0]
        low_limit_has_nodeid = False
        high_limit = __limits[data_type][1]
        high_limit_has_nodeid = False
        if data_type == 0x0008 or data_type == 0x0011:
            if "LowLimit" in cfg and cfg["LowLimit"]:
                low_limit = __parse_float(cfg["LowLimit"], data_type)
            if "HighLimit" in cfg and cfg["HighLimit"]:
                high_limit = __parse_float(cfg["HighLimit"], data_type)
        else:
            if "LowLimit" in cfg and cfg["LowLimit"]:
                if cfg["LowLimit"].upper() == "$NODEID":
                    low_limit = 0
                    low_limit_has_nodeid = True
                else:
                    m = __p_value.match(cfg["LowLimit"])
                    if m:
                        low_limit = int(m.group("value"), 0)
                        low_limit_has_nodeid = m.group("variable") is not None
                    else:
                        warnings.warn(
                            "invalid LowLimit in [{}]: {}".format(
                                section, cfg["LowLimit"]
                            ),
                            stacklevel=5,
                        )
                        return False
            if "HighLimit" in cfg and cfg["HighLimit"]:
                if cfg["HighLimit"].upper() == "$NODEID":
                    high_limit = 0
                    high_limit_has_nodeid = True
                else:
                    m = __p_value.match(cfg["HighLimit"])
                    if m:
                        high_limit = int(m.group("value"), 0)
                        high_limit_has_nodeid = m.group("variable") is not None
                    else:
                        warnings.warn(
                            "invalid HighLimit in [{}]: {}".format(
                                section, cfg["HighLimit"]
                            ),
                            stacklevel=5,
                        )
                        return False

        value_has_nodeid = False
        if data_type == 0x0008 or data_type == 0x0011:
            try:
                value = __parse_float(value, data_type)
            except ValueError:
                warnings.warn(
                    "invalid {} in [{}]: {}".format(entry, section, value), stacklevel=5
                )
                return False
        elif value == "$NODEID":
            value = 0
            value_has_nodeid = True
        else:
            m = __p_value.match(value)
            if m:
                value = int(m.group("value"), 0)
                value_has_nodeid = m.group("variable") is not None
            else:
                warnings.warn(
                    "invalid {} in [{}]: {}".format(entry, section, value), stacklevel=5
                )
                return False

        if not value_has_nodeid and low_limit_has_nodeid:
            low_limit += 1
        elif value_has_nodeid and not low_limit_has_nodeid:
            low_limit -= 1
        if value < low_limit:
            warnings.warn("{} underflow in [{}]".format(entry, section), stacklevel=5)
            return False

        if not value_has_nodeid and high_limit_has_nodeid:
            high_limit += 127
        elif value_has_nodeid and not high_limit_has_nodeid:
            high_limit -= 127
        if value > high_limit:
            warnings.warn("{} overflow in [{}]".format(entry, section), stacklevel=5)
            return False

    return True
