import argparse
import em
import os
import pkg_resources
import struct
import yaml
import warnings

import dcf


class Slave(dcf.Device):
    def __init__(self, cfg: dict, env: dict = {}):
        super().__init__(cfg, env)

        self.name = ""
        self.dcf_path = ""
        self.time_cob_id = 0x100
        self.emcy_cob_id = 0x80000000
        self.heartbeat_multiplier = 1
        self.heartbeat_consumer = False
        self.heartbeat_producer = 0
        self.retry_factor = 1
        self.life_time_factor = 0
        self.guard_time = 0
        self.boot = True
        self.mandatory = False
        self.reset_communication = True
        self.software_file = ""
        self.software_version = 0
        self.configuration_file = ""
        self.restore_configuration = 0
        self.sdo = []

    def write_bin(self, directory: str, verbose: bool = False):
        if self.sdo:
            with open(os.path.join(directory, self.name + ".bin"), "wb") as output:
                output.write(struct.pack("<L", len(self.sdo)))
                for sdo in self.sdo:
                    output.write(sdo)
                    if verbose:
                        print_sdo(self.name, sdo)

    def concise_value(self, index: int, sub_index: int, value):
        if index not in self:
            raise KeyError(self.name + ": object 0x{:04X} does not exist".format(index))
        obj = self[index]
        if sub_index in obj:
            subobj = obj[sub_index]
            if not subobj.access_type.write:
                warnings.warn(
                    self.name
                    + ": no write access for sub-object 0x{:04X}/{}".format(
                        index, sub_index
                    ),
                    stacklevel=2,
                )
            return subobj.data_type.concise_value(index, sub_index, value)
        elif sub_index == 0 and len(obj) > 0:
            return dcf.UNSIGNED8.concise_value(index, 0, value)
        else:
            raise KeyError(
                self.name
                + ": sub-object 0x{:04X}/{} does not exist".format(index, sub_index)
            )

    def __parse_pdo_config(self, pdo: dcf.PDO, comm_idx: int, cfg, options: dict):
        sdo = []

        is_tpdo = (comm_idx & 0xFE00) == 0x1800

        cob_id = pdo.cob_id
        if "cob_id" in cfg:
            if cfg["cob_id"] == "auto":
                i = comm_idx & 0x01FF
                if i < 4:
                    if is_tpdo:
                        pdo.cob_id = (i + 1) * 0x100 + 0x80 + self.node_id
                    else:
                        pdo.cob_id = (i + 1) * 0x100 + 0x100 + self.node_id
                else:
                    pdo.cob_id = options["cob_id"]
                    if pdo.cob_id >= 0x6E0:
                        raise ValueError("no valid 11-bit COB-IDs remaining")
                    options["cob_id"] = pdo.cob_id + 1
            else:
                pdo.cob_id = int(cfg["cob_id"])

        if cob_id != pdo.cob_id | 0x80000000:
            sdo.append(self.concise_value(comm_idx, 1, pdo.cob_id | 0x80000000))

        if not (pdo.cob_id & 0x80000000):
            if "transmission" in cfg:
                transmission = int(cfg["transmission"])
                if transmission != pdo.transmission_type:
                    pdo.transmission_type = transmission
                    sdo.append(self.concise_value(comm_idx, 2, transmission))
            if "inhibit_time" in cfg:
                inhibit_time = int(cfg["inhibit_time"])
                if inhibit_time != pdo.inhibit_time:
                    pdo.inhibit_time = inhibit_time
                    if is_tpdo:
                        sdo.append(self.concise_value(comm_idx, 3, inhibit_time))
            if "event_timer" in cfg:
                event_timer = int(cfg["event_timer"])
                if event_timer != pdo.event_timer:
                    pdo.event_timer = event_timer
                    if is_tpdo:
                        sdo.append(self.concise_value(comm_idx, 5, event_timer))
            if "event_deadline" in cfg:
                event_deadline = int(cfg["event_deadline"])
                if event_deadline != pdo.event_deadline:
                    pdo.event_deadline = event_deadline
                    if not is_tpdo:
                        sdo.append(self.concise_value(comm_idx, 5, event_timer))
            if "sync_start" in cfg:
                sync_start = int(cfg["sync_start"])
                if sync_start != pdo.sync_start_value:
                    pdo.sync_start_value = sync_start
                    if is_tpdo:
                        sdo.append(self.concise_value(comm_idx, 6, sync_start))

            map_idx = comm_idx + 0x200
            if "mapping" in cfg:
                if pdo.n > 0 or len(cfg["mapping"]) > 0:
                    pdo.n = 0
                    pdo.mapping = {}
                    sdo.append(self.concise_value(map_idx, 0, 0))

                size = 0
                n = 0
                for pdo_mapping in cfg["mapping"]:
                    index = int(pdo_mapping["index"])
                    sub_index = int(pdo_mapping.get("sub_index", 0))
                    if index in self and sub_index in self[index]:
                        subobj = self[index][sub_index]
                        if not subobj.pdo_mapping:
                            warnings.warn(
                                self.name
                                + ": sub-object 0x{:04X}/{} does not support PDO mapping".format(
                                    index, sub_index
                                ),
                                stacklevel=3,
                            )
                        bits = subobj.data_type.bits()
                        size += bits
                        n += 1
                        pdo.mapping[n] = subobj
                        value = (
                            ((index & 0xFFFF) << 16)
                            | ((sub_index & 0xFF) << 8)
                            | (bits & 0xFF)
                        )
                        sdo.append(self.concise_value(map_idx, n, value))
                    else:
                        raise KeyError(
                            self.name
                            + ": sub-object 0x{:04X}/{} does not exist".format(
                                index, sub_index
                            )
                        )

                if size > 64:
                    warnings.warn(
                        self.name
                        + ": {} {} mapping exceeds 64 bits".format(
                            "TPDO" if comm_idx >= 0x1800 else "RPDO",
                            (comm_idx & 0x1FF) + 1,
                        ),
                        stacklevel=3,
                    )

                pdo.n = len(pdo.mapping)
                if pdo.n > 0:
                    sdo.append(self.concise_value(map_idx, 0, pdo.n))

            if bool(cfg.get("enabled", "true")):
                sdo.append(self.concise_value(comm_idx, 1, pdo.cob_id))

        return sdo

    @classmethod
    def from_dcf(cls, filename: str, env: dict = {}, args=None) -> "Slave":
        cfg = dcf.parse_file(filename)

        no_strict = getattr(args, "no_strict", False)
        if not dcf.lint(cfg) and not no_strict:
            raise ValueError("invalid DCF: " + filename)

        return cls(cfg, env)

    @classmethod
    def from_config(cls, name: str, cfg, options: dict, args=None) -> "Slave":
        env = {}
        if "node_id" in cfg:
            env["NODEID"] = int(cfg["node_id"])

        slave = cls.from_dcf(str(cfg["dcf"]), env, args)

        slave.name = name

        slave.dcf_path = options["dcf_path"]
        if "dcf_path" in cfg:
            slave.dcf_path = str(cfg["dcf_path"])
        slave.dcf_path = os.path.expandvars(slave.dcf_path)

        if "revision_number" in cfg:
            revision_number = int(cfg["revision_number"])
            if slave.revision_number != 0 and slave.revision_number != revision_number:
                warnings.warn(
                    name + ": specified revision number differs from DCF", stacklevel=2
                )
            slave.revision_number = revision_number

        if "serial_number" in cfg:
            serial_number = int(cfg["serial_number"])
            if slave.serial_number != 0 and slave.serial_number != serial_number:
                warnings.warn(
                    name + ": specified serial number differs from DCF", stacklevel=2
                )
            slave.serial_number = serial_number

        if 0x1012 in slave:
            slave.time_cobid = slave[0x1012][0].parse_value()

        if "time_cob_id" in cfg:
            time_cob_id = int(cfg["time_cob_id"])
            if slave.time_cob_id != time_cob_id:
                if 0x1012 in slave:
                    sdo = slave.concise_value(0x1012, 0, time_cob_id)
                    slave.sdo.append(sdo)
                else:
                    warnings.warn(name + ": object 0x1012 does not exist", stacklevel=2)
            slave.time_cobid = time_cob_id

        if 0x1014 in slave:
            slave.emcy_cob_id = slave[0x1014][0].parse_value()

        slave.heartbeat_multiplier = options["heartbeat_multiplier"]
        if "heartbeat_multiplier" in cfg:
            slave.heartbeat_multiplier = float(cfg["heartbeat_multiplier"])

        if "heartbeat_consumer" in cfg:
            slave.heartbeat_consumer = bool(cfg["heartbeat_consumer"])

        if 0x1017 in slave:
            slave.heartbeat_producer = slave[0x1017][0].parse_value()

        if "heartbeat_producer" in cfg:
            heartbeat_producer = int(cfg["heartbeat_producer"])
            if slave.heartbeat_producer != heartbeat_producer:
                if 0x1017 in slave:
                    sdo = slave.concise_value(0x1017, 0, heartbeat_producer)
                    slave.sdo.append(sdo)
                else:
                    warnings.warn(name + ": object 0x1017 does not exist", stacklevel=2)
            slave.heartbeat_producer = heartbeat_producer

        slave.retry_factor = options["retry_factor"]
        if "retry_factor" in cfg:
            slave.retry_factor = int(cfg["retry_factor"])

        if 0x100C in slave:
            slave.guard_time = slave[0x100C][0].parse_value()

        if "guard_time" in cfg:
            guard_time = int(cfg["guard_time"])
            if guard_time != slave.guard_time:
                if 0x100C in slave:
                    sdo = slave.concise_value(0x100C, 0, guard_time)
                    slave.sdo.append(sdo)
                else:
                    warnings.warn(name + ": object 0x100C does not exist", stacklevel=2)
            slave.guard_time = guard_time

        if 0x100D in slave:
            slave.life_time_factor = slave[0x100D][0].parse_value()

        if "life_time_factor" in cfg:
            life_time_factor = int(cfg["life_time_factor"])
            if life_time_factor != slave.life_time_factor:
                if 0x100D in slave:
                    sdo = slave.concise_value(0x100D, 0, life_time_factor)
                    slave.sdo.append(sdo)
                else:
                    warnings.warn(name + ": object 0x100D does not exist", stacklevel=2)
            slave.life_time_factor = life_time_factor

        if (
            slave.guard_time != 0
            and slave.life_time_factor != 0
            and slave.heartbeat_producer != 0
        ):
            warnings.warn(
                "Cannot use heartbeat protocol and node guarding protocol simultaneously",
                stacklevel=2,
            )
            slave.guard_time = 0
            slave.life_time_factor = 0

        if "error_behavior" in cfg:
            for sub_index, value in cfg["error_behavior"].items():
                sub_index = int(sub_index)
                if sub_index in slave.error_behavior.keys():
                    value = int(value)
                    if value != slave.error_behavior[sub_index]:
                        sdo = slave.concise_value(0x1029, sub_index, value)
                        slave.sdo.append(sdo)
                else:
                    warnings.warn(
                        name
                        + ": sub-object 0x1029/{} does not exist".format(sub_index),
                        stacklevel=2,
                    )

        if "rpdo" in cfg:
            for i in cfg["rpdo"]:
                if i in slave.rpdo:
                    sdo = slave.__parse_pdo_config(
                        slave.rpdo[i], 0x1400 + i - 1, cfg["rpdo"][i], options
                    )
                    slave.sdo.extend(sdo)
                else:
                    raise KeyError(name + ": Receive-PDO {} not available".format(i))

        for i in list(slave.rpdo.keys()):
            if (slave.rpdo[i].cob_id & 0x80000000) or len(slave.rpdo[i].mapping) == 0:
                del slave.rpdo[i]

        if "tpdo" in cfg:
            for i in cfg["tpdo"]:
                if i in slave.tpdo:
                    sdo = slave.__parse_pdo_config(
                        slave.tpdo[i], 0x1800 + i - 1, cfg["tpdo"][i], options
                    )
                    slave.sdo.extend(sdo)
                else:
                    raise KeyError(name + ": Transmit-PDO {} not available".format(i))

        for i in list(slave.tpdo.keys()):
            if (slave.tpdo[i].cob_id & 0x80000000) or len(slave.tpdo[i].mapping) == 0:
                del slave.tpdo[i]

        if "boot" in cfg:
            slave.boot = bool(cfg["boot"])

        if "mandatory" in cfg:
            slave.mandatory = bool(cfg["mandatory"])

        if "reset_communication" in cfg:
            slave.reset_communication = bool(cfg["reset_communication"])

        if "software_file" in cfg:
            slave.software_file = str(cfg["software_file"])

        if "software_version" in cfg:
            slave.software_version = int(cfg["software_version"])
            if 0x1F56 not in slave or 1 not in slave[0x1F56]:
                warnings.warn(
                    name + ": sub-object 0x1F56/1 does not exist", stacklevel=2
                )

        if "restore_configuration" in cfg:
            slave.restore_configuration = int(cfg["restore_configuration"])
            if 0x1011 not in slave or slave.restore_configuration not in slave[0x1011]:
                warnings.warn(
                    name
                    + ": sub-object 0x1011/{} does not exist".format(
                        slave.restore_configuration
                    ),
                    stacklevel=2,
                )

        if "sdo" in cfg:
            for sdo in cfg["sdo"]:
                index = int(sdo["index"])
                sub_index = int(sdo.get("sub_index", 0))
                value = int(sdo.get("value", 0))
                slave.sdo.append(slave.concise_value(index, sub_index, value))

        if slave.sdo:
            slave.configuration_file = os.path.join(slave.dcf_path, slave.name + ".bin")
        if "configuration_file" in cfg:
            slave.dcf_path = str(cfg["configuration_file"])

        return slave


class Master:
    def __init__(self, slaves: dict = {}):
        self.node_id = 255
        self.baudrate = 1000
        self.vendor_id = 0
        self.product_code = 0
        self.revision_number = 0
        self.serial_number = 0
        self.sync_period = 0
        self.sync_window = 0
        self.sync_overflow = 0
        self.time_cob_id = 0x100
        self.emcy_inhibit_time = 0
        self.heartbeat_multiplier = 1
        self.heartbeat_consumer = True
        self.heartbeat_producer = 0
        self.error_behavior = {1: 0x00}
        self.nmt_inhibit_time = 0
        self.start = True
        self.start_nodes = True
        self.start_all_nodes = False
        self.reset_all_nodes = False
        self.stop_all_nodes = False
        self.boot_time = 0
        self.sdo = []
        self.slaves = slaves

    def write_dcf(self, directory: str, remote_pdo: bool = False):
        with open(os.path.join(directory, "master.dcf"), "w") as output:
            globals = {"master": self, "slaves": self.slaves, "remote_pdo": remote_pdo}
            interpreter = em.Interpreter(output=output, globals=globals)
            try:
                filename = pkg_resources.resource_filename(
                    __name__, "data/master.dcf.em"
                )
                interpreter.file(open(filename))
            finally:
                interpreter.shutdown()

    def write_bin(self, directory: str):
        if self.sdo:
            with open(os.path.join(directory, "master.bin"), "wb") as output:
                output.write(struct.pack("<L", len(self.sdo)))
                for sdo in self.sdo:
                    output.write(sdo)

    @classmethod
    def from_config(cls, cfg, options: dict, slaves: dict = {}) -> "Master":
        master = cls(slaves)

        if "node_id" in cfg:
            master.node_id = int(cfg["node_id"])
        if "baudrate" in cfg:
            master.baudrate = int(cfg["baudrate"])

        if "vendor_id" in cfg:
            master.vendor_id = int(cfg["vendor_id"])
        if "product_code" in cfg:
            master.product_code = int(cfg["product_code"])
        if "revision_number" in cfg:
            master.revision_number = int(cfg["revision_number"])
        if "serial_number" in cfg:
            master.serial_number = int(cfg["serial_number"])

        if master.serial_number != 0:
            sdo = dcf.UNSIGNED32.concise_value(0x1018, 0x04, master.serial_number)
            master.sdo.append(sdo)

        if "sync_period" in cfg:
            master.sync_period = int(cfg["sync_period"])
        if "sync_window" in cfg:
            master.sync_window = int(cfg["sync_window"])
        if "sync_overflow" in cfg:
            master.sync_overflow = int(cfg["sync_overflow"])

        if "time_cob_id" in cfg:
            master.time_cob_id = int(cfg["time_cob_id"])

        if "emcy_inhibit_time" in cfg:
            master.emcy_inhibit_time = int(cfg["emcy_inhibit_time"])

        master.heartbeat_multiplier = options["heartbeat_multiplier"]
        if "heartbeat_multiplier" in cfg:
            master.heartbeat_multiplier = float(cfg["heartbeat_multiplier"])
        if "heartbeat_consumer" in cfg:
            master.heartbeat_consumer = bool(cfg["heartbeat_consumer"])
        if "heartbeat_producer" in cfg:
            master.heartbeat_producer = int(cfg["heartbeat_producer"])

        if "error_behavior" in cfg:
            for sub_index, value in cfg["error_behavior"].items():
                master.error_behavior[int(sub_index)] = int(value)

        if "nmt_inhibit_time" in cfg:
            master.nmt_inhibit_time = int(cfg["nmt_inhibit_time"])

        if "start" in cfg:
            master.start = int(cfg["start"])
        if "start_nodes" in cfg:
            master.start_nodes = int(cfg["start_nodes"])
        if "start_all_nodes" in cfg:
            master.start_all_nodes = int(cfg["start_all_nodes"])
        if "reset_all_nodes" in cfg:
            master.reset_all_nodes = int(cfg["reset_all_nodes"])
        if "stop_all_nodes" in cfg:
            master.stop_all_nodes = int(cfg["stop_all_nodes"])

        if "boot_time" in cfg:
            master.boot_time = int(cfg["boot_time"])

        heartbeat = int(master.heartbeat_producer * master.heartbeat_multiplier)
        for slave in slaves.values():
            if slave.heartbeat_consumer and heartbeat > 0:
                if 0x1016 in slave:
                    sub_index = 0
                    for subobj in slave[0x1016].values():
                        if subobj.sub_index == 0:
                            continue
                        value = subobj.parse_value()
                        node_id = (value >> 16) & 0xFF
                        if node_id == master.node_id:
                            sub_index = subobj.sub_index
                            break
                    if sub_index == 0:
                        for subobj in slave[0x1016].values():
                            if subobj.sub_index == 0:
                                continue
                            value = subobj.parse_value()
                            heartbeat_time = value & 0xFFFF
                            node_id = (value >> 16) & 0xFF
                            if heartbeat_time == 0 or node_id == 0 or node_id > 127:
                                sub_index = subobj.sub_index
                                break
                    if sub_index != 0:
                        value = ((master.node_id & 0xFF) << 16) | (heartbeat & 0xFFFF)
                        slave.sdo.insert(
                            0, slave.concise_value(0x1016, sub_index, value)
                        )
                    else:
                        warnings.warn(
                            slave.name + ": no unused entry found in object 0x1016",
                            stacklevel=2,
                        )
                else:
                    warnings.warn(
                        slave.name + ": object 0x1016 does not exist", stacklevel=2
                    )
            else:
                if 0x1016 in slave:
                    for subobj in slave[0x1016].values():
                        if subobj.sub_index == 0:
                            continue
                        value = subobj.parse_value()
                        heartbeat_time = value & 0xFFFF
                        node_id = (value >> 16) & 0xFF
                        if heartbeat_time != 0 and node_id == master.node_id:
                            slave.sdo.insert(
                                0,
                                slave.concise_value(
                                    0x1016, subobj.sub_index, node_id << 16
                                ),
                            )

        for slave in slaves.values():
            if slave.software_version != 0:
                sdo = dcf.UNSIGNED32.concise_value(
                    0x1F55, slave.node_id, slave.software_version
                )
                master.sdo.append(sdo)

            if slave.revision_number != 0 and 0x03 in slave[0x1018]:
                sdo = dcf.UNSIGNED32.concise_value(
                    0x1F87, slave.node_id, slave.revision_number
                )
                master.sdo.append(sdo)

            if slave.serial_number != 0 and 0x04 in slave[0x1018]:
                sdo = dcf.UNSIGNED32.concise_value(
                    0x1F88, slave.node_id, slave.serial_number
                )
                master.sdo.append(sdo)

        return master


def print_sdo(name: str, sdo: bytes):
    index, sub_index, n = struct.unpack("<HBL", sdo[:7])
    print(
        name
        + ": writing {} bytes to 0x{:04X}/{}: {}".format(
            n, index, sub_index, " ".join("{:02X}".format(b) for b in sdo[7:])
        )
    )


def main():
    parser = argparse.ArgumentParser(
        description="Generate the DCF for a CANopen master from a YAML configuration file."
    )
    parser.add_argument(
        "-d",
        "--directory",
        metavar="DIR",
        type=str,
        default="",
        help="the directory in which to store the generated file(s)",
    )
    parser.add_argument(
        "-r", "--remote-pdo", action="store_true", help="generate remote PDO mappings"
    )
    parser.add_argument(
        "-S",
        "--no-strict",
        action="store_true",
        help="do not abort in case of an invalid slave EDS/DCF",
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="print the generated SDO requests"
    )
    parser.add_argument(
        "filename", nargs=1, help="the name of the YAML configuration file"
    )
    args = parser.parse_args()

    with open(args.filename[0], "r") as input:
        cfg = yaml.safe_load(input)

    options = {
        "cob_id": 0x680,
        "dcf_path": "",
        "heartbeat_multiplier": 3.0,
        "retry_factor": 3,
    }
    if "options" in cfg:
        if "dcf_path" in cfg["options"]:
            options["dcf_path"] = str(cfg["options"]["dcf_path"])
        if "heartbeat_multiplier" in cfg["options"]:
            options["heartbeat_multiplier"] = float(
                cfg["options"]["heartbeat_multiplier"]
            )
        if "retry_factor" in cfg["options"]:
            options["retry_factor"] = int(cfg["options"]["retry_factor"])

    slaves = {}
    for name in cfg:
        if name == "master" or name == "options" or name.startswith("."):
            continue
        slave = Slave.from_config(name, cfg[name], options, args)
        if slave.node_id != 255:
            slaves[name] = slave
        else:
            warnings.warn(
                name + ": ignoring slave with unconfigured node-ID", stacklevel=2
            )

    master = Master.from_config(cfg["master"], options, slaves)

    for slave in slaves.values():
        slave.write_bin(args.directory, args.verbose)
    master.write_dcf(args.directory, args.remote_pdo)
    master.write_bin(args.directory)


if __name__ == "__main__":
    main()
