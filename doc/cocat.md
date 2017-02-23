cocat/cocatd - CANopen cat tools
================================

Synopsis
--------

    cocat -h
    cocat --help
    cocat [-i ms | --inhibit=ms] [interface] [StdIn_COBID] [StdOut_COBID]
          [StdErr_COBID]

    cocatd -h
    cocatd --help
    cocatd [-D | --no-daemon] [-f filenname | --file=filename]
           [-n node-ID | --node=node-ID] [interface] [command]

Description
-----------

The CANopen cat tools provide netcat-like functionality over the CAN bus. Their
implementation is based on object 1026 (OS prompt), which provides SDO and
(event-driven) PDO access to the `stdin`, `stdout` and `stderr` streams on a
CANopen device.

### cocat

`cocat` is the client program. It reads bytes from `stdin` and writes them as
single-byte PDOs to the CAN bus, provided the second argument is a valid PDO
COB-ID. If the third and fourth arguments are valid COB-IDs, bytes received from
PDOs with those COB-IDs are written to `stdout` and `stderr`, respectively.

The options are as follows:

    -h, --help            Display help.
    -i <ms>, --inhibit=<ms>
                          Wait at least <ms> milliseconds between PDOs
                          (default: 1).

### cocatd

`cocatd` is the server program, and by default runs as a daemon. It executes the
provided command in a child process, and connects the `stdin`, `stdout` and
`stderr` streams of that process to sub-objects 1026:01, 1026:02 and 1026:03 in
its object dictionary, respectively. The default configuration enables PDO
access to each of these sub-objects. Regardless of the configuration, `cocatd`
expects RPDO1 to map to 1026:01 (StdIn), and TPDO1 and TPDO2 to map to 1026:02
(StdOut) and 1026:03 (StdErr), respectively.

The options are as follows:

    -D, --no-daemon       Do not run as daemon.
    -f <filename>, --file=<filename>
                          Use <filename> as the EDS/DCF file instead of using
                          the default configuration.
    -h, --help            Display help.
    -n <node-ID> --node=<node-ID>
                          Use <node-ID> as the CANopen node-ID.

Example
-------

The CANopen cat tools can be used to provide shell access over the CAN bus. To
setup such a configuration, run

    cocatd can0 -n 1 /bin/sh

on the server, and

    cocat can0 0x201 0x181 0x281

on the client. Any commands typed at the client side are executed on the server,
and the response is printed by the client.

