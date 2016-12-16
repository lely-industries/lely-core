can2udp - CAN to UDP forwarding tool
====================================

Synopsis
--------

    can2udp -h
    can2udp --help
    can2udp [-4 | --ipv4 | -6 | --ipv6] [-b | --broadcast] [-D | --no-daemon]
            [-f | --flush] [-i n | --interface=n] [-k ms | --keep-alive=ms]
            [-p local_port | --port=local_port] [-v | --verbose] [interface]
            [address] [port]

Description
-----------

The CAN to UDP forwarding tool is a bridge between the CAN bus and UDP. It
converts CAN frames to and from generic frames (as specified by CiA 315) and
transmits these over UDP.

The options are as follows:

    -4, --ipv4            Use IPv4 for receiving UDP frames (default).
    -6, --ipv6            Use IPv6 for receiving UDP frames.
    -b, --broadcast       Send broadcast messages (IPv4 only).
    -D, --no-daemon       Do not run as daemon.
    -f, --flush           Flush the send buffer after every received CAN frame.
    -h, --help            Display help.
    -i <n>, --interface=<n>
                          Uses WTM interface indicator <n> (in the range
                          [1..127], default: 1).
    -k <ms>, --keep-alive=<ms>
                          Sends a keep-alive message every <ms> milliseconds
                          (default: 10000).
    -p <local port>, --port=<local port>
                          Receive UDP frames on <local port>.
    -v, --verbose         Print sent and received CAN frames.

