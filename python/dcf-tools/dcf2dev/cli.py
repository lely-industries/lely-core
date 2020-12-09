import argparse
import contextlib
import em
import os
import pkg_resources
import sys

import dcf

from .cdevice import CDevice, CDataType


@contextlib.contextmanager
def open_or_stdout(filename):
    if filename != "-":
        with open(filename, "w") as out:
            yield out
    else:
        with os.fdopen(os.dup(sys.stdout.fileno()), "w") as out:
            yield out


def parse_time_scet(value: str):
    if not value:
        value = "0 0"
    seconds, subseconds = value.split()
    return (int(seconds, 0), int(subseconds, 0))


def parse_time_sutc(value: str):
    if not value:
        value = "0 0 0"
    days, ms, usec = value.split()
    return (int(days, 0), int(ms, 0), int(usec, 0))


def read_device_from_dcf(filename):
    env = {"NODEID": 255}
    dev = dcf.Device.from_dcf(filename, env)
    dev.c = CDevice(dev)
    return dev


def main():
    parser = argparse.ArgumentParser(
        description="Generate a static C device description from an EDS/DCF file."
    )
    parser.add_argument(
        "--no-strings",
        action="store_true",
        help="do not include optional strings in the output",
    )
    parser.add_argument(
        "--include-config",
        action="store_true",
        help="add '#include <config.h>' snippet to the output",
    )
    parser.add_argument(
        "-o",
        "--output",
        metavar="FILE",
        type=str,
        default="-",
        help="write the output to FILE instead of stdout",
    )
    parser.add_argument(
        "--header",
        action="store_true",
        help="generate header file with function prototype",
    )
    parser.add_argument(
        "--deftype-time-scet",
        metavar="INDEX",
        nargs=1,
        type=int,
        help="use INDEX for the ECSS SCET time data type",
    )
    parser.add_argument(
        "--deftype-time-sutc",
        metavar="INDEX",
        nargs=1,
        type=int,
        help="use INDEX for the ECSS SUTC time data type",
    )
    parser.add_argument("filename", nargs=1, help="the name of the EDS/DCF file")
    parser.add_argument(
        "name",
        nargs=1,
        type=str,
        default="",
        help="the variable name of the generated device description",
    )
    args = parser.parse_args()

    if args.deftype_time_scet:
        index = args.deftype_time_scet[0]
        dcf.DataType.add_custom(index, "TIME_SCET", parse_time_scet)
        CDataType.add_custom(
            index, "scet", "{{ .subseconds = {0[1]:d}, .seconds = {0[0]:d} }}"
        )
    if args.deftype_time_sutc:
        index = args.deftype_time_sutc[0]
        dcf.DataType.add_custom(index, "TIME_SUTC", parse_time_sutc)
        CDataType.add_custom(
            index, "sutc", "{{ .usec = {0[2]:d}, .ms = {0[1]:d}, .days = {0[0]:d} }}"
        )

    if args.header:
        dev = None
        filename = "dev.h.em"
    else:
        dev = read_device_from_dcf(args.filename[0])
        filename = "dev.c.em"

    with open_or_stdout(args.output) as output:
        params = {
            "no_strings": args.no_strings,
            "include_config": args.include_config,
            "dev": dev,
            "name": args.name[0],
        }
        interpreter = em.Interpreter(output=output, globals=params)
        try:
            filename = pkg_resources.resource_filename(__name__, "data/" + filename)
            interpreter.file(open(filename))
        finally:
            interpreter.shutdown()


if __name__ == "__main__":
    main()
