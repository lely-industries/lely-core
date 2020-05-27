from .device import Device


def print_rpdo(dev: Device):
    for i, pdo in dev.rpdo.items():
        if pdo.cob_id & 0x80000000:
            continue
        if 0x5800 + i - 1 in dev:
            value = dev[0x5800 + i - 1][0].parse_value()
            j = value >> 8
            node_id = value & 0xFF
        else:
            if pdo.cob_id & 0x20000000:
                continue
            cob_id = pdo.cob_id & 0x780
            if (
                cob_id != 0x180
                and cob_id != 0x280
                and cob_id != 0x380
                and cob_id != 0x480
            ):
                continue
            j = cob_id >> 8
            node_id = pdo.cob_id & 0x7F
        print("RPDO {} mapped to TPDO {} on node {}".format(i, j, node_id))
        for k, subobj in pdo.mapping.items():
            if 0x5A00 + i - 1 in dev and k in dev[0x5A00 + i - 1]:
                value = dev[0x5A00 + i - 1][k].parse_value()
                index = (value >> 16) & 0xFFFF
                sub_index = (value >> 8) & 0xFF
                print(
                    "  0x{:04X}/{} <- 0x{:04X}/{}".format(
                        subobj.index, subobj.sub_index, index, sub_index
                    )
                )


def print_tpdo(dev: Device):
    for i, pdo in dev.tpdo.items():
        if pdo.cob_id & 0x80000000:
            continue
        if 0x5C00 + i - 1 in dev:
            value = dev[0x5C00 + i - 1][0].parse_value()
            j = value >> 8
            node_id = value & 0xFF
        else:
            if pdo.cob_id & 0x20000000:
                continue
            cob_id = pdo.cob_id & 0x780
            if (
                cob_id != 0x200
                and cob_id != 0x300
                and cob_id != 0x400
                and cob_id != 0x500
            ):
                continue
            j = (cob_id >> 8) - 1
            node_id = pdo.cob_id & 0x7F
        print("TPDO {} mapped to RPDO {} on node {}".format(i, j, node_id))
        for k, subobj in pdo.mapping.items():
            if 0x5E00 + i - 1 in dev and k in dev[0x5E00 + i - 1]:
                value = dev[0x5E00 + i - 1][k].parse_value()
                index = (value >> 16) & 0xFFFF
                sub_index = (value >> 8) & 0xFF
                print(
                    "  0x{:04X}/{} -> 0x{:04X}/{}".format(
                        subobj.index, subobj.sub_index, index, sub_index
                    )
                )
