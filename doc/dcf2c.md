dcf2c - CANopen EDS/DCF to C conversion tool
============================================

Synopsis
--------

    dcf2c -h
    dcf2c --help
    dcf2c [--no-strings] [-o <file> | --output=<file>] [filename]
          [variable_name]

Description
-----------

The CANopen EDS/DCF to C conversion tool reads the EDS or DCF file given by
`filename` and generates a `co_sdev` C struct containing a static device
description. This is typically used on embedded platforms which lack the
resources to parse an EDS/DCF file at runtime.

The options are as follows:

    -h, --help            Display help.
    --no-strings          Do not include optional strings in the output.
    -o <file>, --output=<file> \
                          Write the output to <file> instead of stdout.

The `--no-strings` option prevents the names of objects and sub-objects from
appearing in the generated struct (they are displayed in the comments instead).

