import argparse
import contextlib
import em
import os
import pkg_resources
import sys

import dcf

from .cdevice import CDevice


@contextlib.contextmanager
def open_or_stdout(filename):
    if filename != "-":
        with open(filename, "w") as out:
            yield out
    else:
        with os.fdopen(os.dup(sys.stdout.fileno()), "w") as out:
            yield out


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
        "-o",
        "--output",
        metavar="FILE",
        type=str,
        default="-",
        help="write the output to FILE instead of stdout",
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

    env = {"NODEID": 10}
    dev = dcf.Device.from_dcf(args.filename[0], env)
    dev.c = CDevice(dev)

    with open_or_stdout(args.output) as output:
        globals = {"no_strings": args.no_strings, "dev": dev, "name": args.name[0]}
        interpreter = em.Interpreter(output=output, globals=globals)
        try:
            filename = pkg_resources.resource_filename(__name__, "data/dev.c.em")
            interpreter.file(open(filename))
        finally:
            interpreter.shutdown()


if __name__ == "__main__":
    main()
