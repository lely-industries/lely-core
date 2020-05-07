import argparse
import sys

from .device import Device
from .lint import lint
from .parse import parse_file
from .print import print_rpdo, print_tpdo


def main():
    parser = argparse.ArgumentParser(
        description="Check the validity of an EDS/DCF file."
    )
    parser.add_argument(
        "-n", "--node_id", metavar="ID", type=int, default=255, help="the node-ID"
    )
    parser.add_argument(
        "-p", "--print", action="store_true", help="print the PDO mappings"
    )
    parser.add_argument(
        "filename", nargs=1, help="the name of the EDS/DCF file to be checked"
    )
    args = parser.parse_args()

    cfg = parse_file(args.filename[0])
    if not lint(cfg):
        sys.exit(1)

    env = {}
    if args.node_id != 255:
        env["NODEID"] = args.node_id
    dev = Device(cfg, env)

    if args.print:
        print_rpdo(dev)
        print_tpdo(dev)


if __name__ == "__main__":
    main()
