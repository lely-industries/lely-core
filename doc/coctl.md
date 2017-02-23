coctl - CANopen control tool
============================

Synopsis
--------

    coctl -h
    coctl --help
    coctl [-e | --exit] [-W | --no-wait] [-i ms | --inhibit=ms]
          [interface filename]...

Description
-----------

The CANopen control tool provides shell-like access to one or more CANopen
network according to the ASCII gateway protocol (CiA 309-3).

The options are as follows:

    -e, --exit            Exit on error.
    -h, --help            Display help.
    -i <ms>, --inhibit=<ms>
                          Wait at least <ms> milliseconds between requests
                          (default: 100).
    -W, --no-wait         Do not wait for the previous command to complete
                          before accepting the next one.

For each of the CANopen networks the gateway needs to access, a CAN interface
and an EDS/DCF filename must be provided. The networks are numbered
sequentially, starting with 1.

Syntax
------

The gateway is controlled by commands. A command is composed of tokens, which
are separated by whitespace, and is terminated by a new-line character.
Whitespace is handled as specified in [ISO/IEC 9899].

Commands are case insensitive. Some commands have short form as well as a long
form. The long form is obtained by concatenating the short form and the string
enclosed in brackets (e.g., "r[ead]" can be specified as both "r" and "read").

Network- and node-level commands are prefixed with a network and/or node number
(in the range [1..127]). These numbers can be omitted if default values are set
(with `set network` and `set node`, respectively). `[net]` and
`[[net] node]` indicate which commands require a prefix.

The gateway prepends a sequence number (enclosed in brackets) to each command.
The confirmation of the command also begins with the same sequence number. This
allows the user to match responses to requests. Event-triggered messages from
the gateway do not begin with a sequence number.

### Data types

Some SDO and PDO commands require data type indications. The data type syntax is
provided in the following table:

Syntax | CANopen data type
------ | -----------------
b      | BOOLEAN
u8     | UNSIGNED8
u16    | UNSIGNED16
u24    | UNSIGNED24
u32    | UNSIGNED32
u40    | UNSIGNED40
u48    | UNSIGNED48
u56    | UNSIGNED56
u64    | UNSIGNED64
i8     | INTEGER8
i16    | INTEGER16
i24    | INTEGER24
i32    | INTEGER32
i40    | INTEGER40
i48    | INTEGER48
i56    | INTEGER56
i64    | INTEGER64
r32    | REAL32
r64    | REAL64
t      | TIME_OF_DAY
td     | TIME_DIFFERENCE
vs     | VISIBLE_STRING
os     | OCTET_STRING
us     | UNICODE_STRING
d      | DOMAIN

Numerical values are represented as specified in [ISO/IEC 9899]. Values of data
type TIME_OF_DAY and TIME_DIFFERENCE consist of two unsigned integers separated
by whitespace. Values of data type VISIBLE_STRING are enclosed with double
quotes. If a double quote is used within the string, it is escaped by a second
quote (e.g, "Hello, ""World""!"). Other escape sequences as specified in
[ISO/IEC 9899] are allowed. Values of data type OCTET_STRING, UNICODE_STRING and
DOMAIN are encoded in MIME Base64 (as specified in [RFC 2045]).

### Bit-rates

The 'Initialize gateway' (`init`) and 'LSS configure bit-rate'
(`lss_conf_bitrate`) commands require a bit-rate indication. Bit-rates are
specified with a table_selector and table_index. The only value currently
supported for table_selector is 0. The valid values for table_index are provided
in the following table:

table_index | Bit rate
----------- | ----------------------------
0           | 1000 kbit/s
1           | 800 kbit/s
2           | 500 kbit/s
3           | 250 kbit/s
4           | 125 kbit/s
5           | reserved
6           | 50 kbit/s
7           | 20 kbit/s
8           | 10 kbit/s
9           | Automatic bit rate detection

[ISO/IEC 9899]: http://www.open-std.org/jtc1/sc22/wg14/www/standards.html
[RFC 2045]: https://tools.ietf.org/html/rfc2045

SDO access services
-------------------

### Upload SDO

    [[net] node] r[read] <multiplexer> <datatype>

Initiates an SDO upload service. `<multiplexer>` is a combination of an object
index and sub-index, while `<datatype>` must be one of the values in the table
above.

### Download SDO

    [[net] node] w[rite] <multiplexer> <datatype> <value>

Initiates an SDO download service. `<multiplexer>` is a combination of an object
index and sub-index, while `<datatype>` must be one of the values in the table
above.

### Configure SDO time-out

    [net] set sdo_timeout <value>

Configures the time-out (in milliseconds) for all client-SDOs on the gateway.
The actual time-out used is limited by the value set with `set command_timeout`.

PDO access services
-------------------

### Configure RPDO

    [net] set rpdo <nr> <COB> <tx-type> <nr-of-data> <map-obj1>[..<map-obj64>]

where

    <tx-type> ::= "rtr" | "event" | "sync<0..240>"

is the transmission type.

Creates an RPDO on the gateway. `<nr-of-data>` specifies the number of
`<map-obj>` values, each of which is an object index and sub-index (i.e., a
`<multiplexer>`). Dummy mappings can be created by specifying the object index
of one of the static data types (in the range [0x0001..0x001F]).

### Simple configure TPDO

    [net] set tpdo <nr> <COB> <tx-type> <nr-of-data> <map-obj1>[..<map-obj64>]

where

    <tx-type> ::= "rtr" | "event" | "sync<0..240>"

is the transmission type.

Creates a TPDO on the gateway. `<nr-of-data>` specifies the number of
`<map-obj>` values, each of which is an object index and sub-index (i.e., a
`<multiplexer>`).

### Extended configure TPDO

    [net] set tpdox <nr> <COB> <tx-type> <nr-of-data> <map-obj1>[..<map-obj64>]

where

    <tx-type> ::= "rtr" | "event<intime> <etime>" | "sync<0..240> <scount>"

is the transmission type.

Similar to the `set tpdo` command, but allows the configuration of the inhibit
time (`<intime>`) and event time (`<etime>`), both in milliseconds, and the SYNC
start value (`<scount>`).

### Read PDO data

    [net] r[ead] p[do] <nr>

Reads the values of the mapped objects of an RPDO.

Note that this implementation does not support the RTR transmission type.

### Write PDO data

    [net] w[rite] p[do] <nr> <nr-of-data> <value1>[..<value64>]

Writes the values to the mapped objects of a TPDO and triggers its transmission.

CANopen NMT services
--------------------

With the exception of heartbeat consumption, NMT services are only available on
a class 3 gateway, which acts as the CANopen manager.

### Start node

    [[net] node] start

Sets the specified slave to the NMT state operational. If `node` is 0, all
slaves are affected.

### Stop node

    [[net] node] stop

Sets the specified slave to the NMT state stopped. If `node` is 0, all slaves
are affected.

### Set node to pre-operational

    [[net] node] preop[erational]

Sets the specified slave to the NMT state pre-operational. If `node` is 0, all
slaves are affected.

### Reset node

    [[net] node] reset node

Sets the specified slave to the NMT state reset application. If `node` is 0, all
slaves are affected.

### Reset communication

    [[net] node] reset comm[unication]

Sets the specified slave to the NMT state communication. If `node` is 0, all
slaves are affected.

### Enable node guarding

    [[net] node] enable guarding <guardingtime> <lifetimefactor>

Starts node guarding for the specified slave. `<guardingtime>` is specified in
milliseconds. If `<guardingtime>` or `<lifetimefactor>` is 0, node guarding is
disabled.

### Disable node guarding

    [[net] node] disable guarding

Stops node node guarding for the specified slave.

### Start heartbeat consumer

    [[net] node] enable heartbeat <heartbeat_consumer_time>

Starts the consumption of heartbeat messages for the specified slave.
`<heartbeat_consumer_time>` is specified in milliseconds. A value of 0 disables
heartbeat consumption.

### Disable heartbeat consumer

    [[net] node] disable heartbeat

Stops the consumption of heartbeat messages for the specified slave.

CANopen interface configuration services
----------------------------------------

### Initialize gateway

    [net] init <bitrate>

Initializes the bit rate of the gateway and performs a power-on equivalent reset
of the CANopen interface. `<bitrate>` must be one of the valid table_index
values specified in the bit-rates table.

### Set heartbeat producer

    [net] set heartbeat <value>

Sets the heartbeat producer time (in milliseconds) of the gateway. A value of 0
disables heartbeat production.

### Set node-ID

    [net] set id <value>

Sets the pending node-ID of the gateway.

### Set command time-out

    [net] set command_timeout <value>

Sets the maximum allowed duration (in milliseconds) for processing a command.

Note that in this implementation the time-out applies to a single SDO or LSS
response, _not_ the overall duration of a command.

### Boot-up forwarding

    [net] boot_up_indication <cs>

where

    <cs> ::= "Enable" | "Disable"

Enables or disables the forwarding of "boot-up message received" events.

Gateway management services
---------------------------

### Set default network

    set network <value>

Sets the default network number to be used for all services.

### Set default node-ID

    [net] set node <value>

Sets the default node-ID to be used for all services.

### Get version

    [net] info version

Returns information on the gateway and its CANopen interface.

### Set command size

    set command_size <value>

Sets the maximum size (in bytes) of a command to be accepted by the gateway.

Note that this command is implemented but will always return an error since we
cannot make any guarantees about memory resources.

Layer setting services
----------------------

Layer setting services (LSS) are only available on a class 3 gateway, which acts
as the CANopen manager.

### LSS switch state global

    [net] lss_switch_glob <0|1>

Switches all slaves to the waiting (0) or configuration (1) state.

### LSS switch state selective

    [net] lss_switch_sel <vendorID> <product code> <revisionNo> <serialNo>

Switches the slave with the specified LSS address to the configuration state.

### LSS configure node-ID

    [net] lss_set_node <node>

Configures the pending node-ID of the slave that is in the configuration state.

### LSS configure bit-rate

    [net] lss_conf_bitrate <table_selector> <table_index>

Configures the new pending bit rate of the slave that is in the configuration
state. `<table_selector>` must be 0, while `<table_index>` must be one of the
values in the bit-rates table.

### LSS activate new bit-rate

    [net] lss_activate_bitrate <switch_delay>

Activates simultaneously the bit rate of all CANopen devices in the network,
where `<switch_delay>` specifies the length (in milliseconds) of two delay
periods of equal length. Each slaves waits one period before switching the
pending bit rate to the active bit rate, then waits another period before
transmitting any messages.

### LSS store configuration

    [net] lss_store

Requests the slave to store the configured local layer settings to non-volatile
memory.

### Inquire LSS address

    [net] lss_inquire_addr <Code>

Requests an LSS number from the slave that is in the configuration state, where

    <Code> ::= "0x5A" | "0x5B" | "0x5C" | "0x5D"

is the code for the vendor-ID, product code, revision number and serial number,
respectively.

### LSS inquire node-ID

    [net] lss_get_node

Requests the active node-ID of the slave that is in the configuration state.

### LSS identify remote slave

    [net] lss_identity <vendorID> <product code> <revisionNo_low> \
                       <revisionNo_high> <serialNo_low> <serialNo_high>

Checks if a slave exists with the specified vendor-ID and product code, and a
revision number and serial number within the specified range.

### LSS identify non-configured remote slaves command

    [net] lss_ident_nonconf

Checks if there are any slaves with an invalid node-ID (0xff/255).

### LSS Slowscan (Lely specific)

    [net] _lss_slowscan <vendorID> <product code> <revisionNo_low> \
                        <revisionNo_high> <serialNo_low> <serialNo_high>

Performs a binary search using `lss_identity` to identify a single slave with an
LSS address in the specified range. On success, the slave is switched to the
configuration state with `lss_switch_sel`.

Because this is an aggregate command, it may take longer to complete than the
time set with `set command_timeout`.

### LSS Fastscan (Lely specific)

    [net] _lss_fastscan <vendorID> <vendorID mask> <product code> \
                        <product code mask> <revisionNo> <revisionNo mask> \
                        <serialNo> <serialNo mask>

Uniquely identifies a single slave by repeatedly employing the LSS Fastscan
service. On success, the slave is switch to the configuration state. For each
LSS number the user supplies a value with the known bits and a mask identifying
which bits are known.

Because this is an aggregate command, it may take longer to complete than the
time set with `set command_timeout`.

Example
-------

A typical session may look like:

    $ coctl can0 /etc/coctl.dcf
    > [1] set network 1     # Set the default network, so it can be omitted.
    < [1] OK
    > [2] set id 1          # Set the node-ID of the gateway to 1.
    < [2] OK
    > [3] init 0            # Initialize the gateway, and configure to CAN bus
    > [3]                   # with a bit rate of 1000 kbit/s.
    < [3] OK
    < 1 2 BOOT_UP
    < 1 2 USER BOOT A (The CANopen device is not listed in object 1F81.)
    > [4] set node 2        # Target the slave with node-ID 2.
    < [4] OK
    > [5] r 0x1018 0x01 u32 # Read the vendor-ID (sub-object 1018:01).
    < [5] 0x00000360        # A device by Lely Industries N.V.

