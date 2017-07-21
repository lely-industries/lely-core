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
                          Use WTM interface indicator <n> (in the range
                          [1..127], default: 1).
    -k <ms>, --keep-alive=<ms>
                          Sends a keep-alive message every <ms> milliseconds
                          (default: 10000).
    -p <local port>, --port=<local port>
                          Receive UDP frames on <local port>.
    -v, --verbose         Print sent and received CAN frames.

Example
-------

Linux supports virtual CAN interfaces (through SocketCAN). This allows a user to
run CAN programs on machines (such as a developer PC) which do not have a
physical CAN bus. `can2udp` makes it possible to connect the virtual CAN
interface to an actual CAN bus on a remote device, as long as there is an
IPv4/IPv6 connection.

To setup the connection, run

    can2udp -fp 6000 can0 192.168.0.101 6001

on the device with a physical CAN bus, and

    can2udp -fp 6001 vcan0 192.168.0.100 6000

on the machine with a virtual CAN interface. The first device listens on
192.168.0.100:6000 for incoming UDP frames, and puts the CAN frames they contain
on can0. Frames originating from the CAN bus are sent to 192.168.0.101:6001. The
second devices receives those messages and puts them on vcan0. In this way, CAN
frames are duplicated on can0 and vcan0, effectively combining the two remote
CAN interfaces into a single CAN bus.

To monitor the CAN frames sent and received (like candump), run

    can2udp -Dfvp 6001 vcan0 192.168.0.100 6000

