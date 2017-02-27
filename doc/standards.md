CiA standards support
=====================

CiA 301 v4.2.0: CANopen application layer and communication profile
-------------------------------------------------------------------

### Implemented

- Process data object (PDO): lely/co/rpdo.h, lely/co/tpdo.h
- Service data object (SDO): lely/co/csdo.h, lely/co/ssdo.h
- Synchronization object (SYNC): lely/co/sync.h
- Time stamp object (TIME): lely/co/time.h
- Emergency object (EMCY): lely/co/emcy.h
- Network management (NMT): lely/co/nmt.h
- Network initialization and system boot-up
- Object dictionary: lely/co/dev.h, lely/co/obj.h
  - 1000: Device type
  - 1001: Error register
  - 1003: Pre-defined error field
  - 1005: COB-ID SYNC message
  - 1006: Communication cycle period
  - 1007: Synchronous window length
  - 100C: Guard time
  - 100D: Life time factor
  - 1012: COB-ID time stamp object
  - 1013: High resolution time stamp
  - 1014: COB-ID EMCY
  - 1015: Inhibit time EMCY
  - 1016: Consumer heartbeat time
  - 1017: Producer heartbeat time
  - 1018: Identity object
  - 1019: Synchronous counter overflow value
  - 1026: OS prompt: [cocat/cocatd](@ref md_doc_cocat)
  - 1028: Emergency consumer object
  - 1029: Error behavior object
  - 1200..127F: SDO server parameter
  - 1280..12FF: SDO client parameter
  - 1400..15FF: RPDO communication parameter
  - 1600..17FF: RPDO mapping parameter
  - 1800..19FF: TPDO communication parameter
  - 1A00..1BFF: TPDO mapping parameter

### Not implemented/Application specific

- Multiplex PDO (MPDO)
- Object dictionary:
  - 1002: Manufacturer status register
  - 1008: Manufacturer device name
  - 1009: Manufacturer hardware version
  - 100A: Manufacturer software version
  - 1010: Store parameters
  - 1011: Restore default parameters
  - 1020: Verify configuration
  - 1021: Store EDS
  - 1022: Store format
  - 1023: OS command
  - 1024: OS command mode
  - 1025: OS debugger interface
  - 1027: Module list
  - 1FA0..1FCF: Object scanner list
  - 1FD0..1FFF: Object dispatching list

CiA 302 v4.1.0: Additional application layer functions
------------------------------------------------------

### Implemented

- Network management (CiA 302-2): lely/co/nmt.h
  - Startup:
    - NMT startup
    - NMT startup simple
    - Start process boot NMT slave
    - Boot NMT slave
    - Check configuration
    - Check NMT state
    - Error status
  - Error control:
    - Start error control
    - Error handler
    - Bootup handler
  - Object dictionary:
    - 102A: NMT inhibit time
    - 1F80: NMT startup
    - 1F81: NMT slave assignment
    - 1F82: Request NMT
    - 1F84: Device type identification
    - 1F85: Vendor identification
    - 1F86: Product code
    - 1F87: Revision number
    - 1F88: Serial number
    - 1F89: Boot time
    - 1F8A: Restore configuration
- Configuration and program download (CiA 302-3): lely/co/nmt.h
  - Configuration manager:
    - 1F20: Store DCF
    - 1F22: Concise DCF
    - 1F25: Configuration request
    - 1F26: Expected configuration date
    - 1F26: Expected configuration time
  - Program download:
    - Check and update program software version
    - Objects for program download on a CANopen manager:
      - 1F55: Expected software identification
      - 1F58: Program data

### Not implemented/Application specific

- Network management (CiA 302-2):
  - Startup:
    - NMT flying master startup
  - Additional NMT master services and protocols
  - Object dictionary:
    - 1F83: Request node guarding
    - 1F90: NMT flying master timing parameters
    - 1F91: Self starting node timing parameters
- Configuration and program download (CiA 302-3):
  - Configuration manager:
    - 1F21: Store format
    - 1F23: Store EDS NMT slave
    - 1F24: Store format EDS NMT slave
  - Program download:
    - Bootloader
    - Objects for program download on a CANopen device:
      - 1F50: Program data
      - 1F51: Program control
      - 1F56: Program software identification
      - 1F57: Flash status identification
- Network variables and process image (CiA 302-4)
- SDO manager (CiA 302-5)
- Network redundancy (CiA 302-6)
- Multi-level networking (CiA 302-7)

CiA 305 v3.0.0: Layer setting services (LSS) and protocols: lely/co/lss.h
-------------------------------------------------------------------------

### Implemented

- Switch state:
  - Switch state global
  - Switch state selective
- Configuration:
  - Configure node-ID
  - Configure bit timing parameters
- Inquiry:
  - Inquire LSS address
  - Inquire node-ID
- Identification:
  - LSS identify remote slave
  - LSS identify slave
  - LSS identify non-configured remote slave
  - LSS identify non-configured slave
  - LSS Fastscan

### Application specific

- Configuration:
  - Activate bit timing parameters
  - Store configuration

CiA 306 v1.3.7: Electronic device description: lely/co/dcf.h
------------------------------------------------------------

EDS/DCF files can be parsed at runtime or be compiled with a program as a static
device description (lely/co/sdev.h). [dcf2c](@ref md_doc_dcf2c) can be used to
generate a static device description in C from an EDS/DCF file.

### Implemented

- Electronic Data Sheet and Device Configuration File (CiA 306-1)
  - Electronic data sheet specification:
    - General device information
    - Object dictionary:
      - Mapping of dummy entries
      - Object descriptions:
        - Object lists
        - Object description
        - Specific flags
        - Compact storage:
          - PDO definitions
          - Array values
  - Device configuration file specification:
    - Object sections:
      - Parameter value in standard description
      - Denotation
      - Compact storage:
        - PDO definitions
        - Array values
      - Device commissioning

### Not implemented

- Electronic Data Sheet and Device Configuration File (CiA 306-1):
  - Electronic data sheet specification:
    - File information
    - Object dictionary:
      - Object descriptions:
        - Compact storage:
          - Network variables
      - Object links
      - Comments
  - Device configuration file specification:
    - File information
    - Object sections:
      - Compact storage:
        - Network variables
  - Module concept
- Profile database specification (CiA 306-2)
- Network variable handling and tool integration (CiA 306-3)

CiA 309 v2.0: Access from other networks
----------------------------------------

[coctl](@ref md_doc_coctl) is a Class 3 ASCII gateway providing shell-like
access to one or more CANopen networks.

### Implemented

- General principles and services (CiA 309-1): lely/co/gw.h
  - SDO access services:
    - SDO upload
    - SDO download
    - Configure SDO time-out
  - PDO access services:
    - Configure RPDO
    - Configure TPDO
    - Read PDO data
    - Write PDO data
    - RPDO received
  - CANopen NMT services:
    - Start node
    - Stop node
    - Set node to pre-operational
    - Reset node
    - Reset communication
    - Enable node guarding
    - Disable node guarding
    - Start heartbeat consumer
    - Disable heartbeat consumer
    - Error control event received
  - Device failure management services:
    - Emergency event received
  - CANopen interface configuration services:
    - Initialize gateway
    - Set heartbeat producer
    - Set node-ID
    - Start emergency consumer
    - Stop emergency consumer
    - Boot-up forwarding
  - Gateway management services:
    - Set default network
    - Set default node-ID
    - Get version
  - Layer setting services:
    - LSS switch state global
    - LSS switch state selective
    - LSS configure node-ID
    - LSS configure bit-rate
    - LSS activate new bit-rate
    - LSS store configuration
    - Inquire LSS address
    - LSS inquire node-ID
    - LSS identify remote slave
    - LSS identify non-configured remote slaves
    - LSS Fastscan
- ASCII mapping (CiA 309-3): lely/co/gw_txt.h
  - SDO access commands:
    - Upload SDO
    - Download SDO
    - Configure SDO time-out
  - PDO access access commands:
    - Configure RPDO
    - Configure TPDO
    - Read PDO data
    - Write PDO data
    - RPDO received
  - CANopen NMT access commands:
    - Start node
    - Stop node
    - Set node to pre-operational
    - Reset node
    - Reset communication
    - Enable node guarding
    - Disable node guarding
    - Start heartbeat consumer
    - Disable heartbeat consumer
    - Error control event received
  - Device failure management commands:
    - Emergency event received
  - CANopen interface configuration access commands:
    - Initialize gateway
    - Set heartbeat producer
    - Set node-ID
    - Set command time-out
    - Boot-up forwarding
  - Gateway management access commands:
    - Set default network
    - Set default node-ID
    - Get version
    - Set command size
  - Layer setting access commands:
    - LSS switch state global
    - LSS switch state selective
    - LSS configure node-ID
    - LSS configure bit-rate
    - LSS activate new bit-rate
    - LSS store configuration
    - Inquire LSS address
    - LSS inquire node-ID
    - LSS identify remote slave
    - Identify non-configured remote slaves
    - LSS Fastscan

### Not implemented

- General principles and services (CiA 309-1):
  - SDO access services:
    - Extended SDO upload
  - Device failure management services:
    - Read device error
  - CANopen interface configuration services:
    - Store configuration
    - Restore configuration
    - Set command time-out
  - Gateway management services:
    - Set command size
  - Controller management services
  - Layer setting services:
    - LSS assign node-ID to LSS address
    - LSS complete node-ID configuration
- Modbus/TCP mapping (CiA 309-2)
- ASCII mapping (CiA 309-3):
  - Device failure management commands:
    - Read device error
  - CANopen interface configuration commands:
    - Store configuration
    - Restore configuration
  - Layer setting commands:
    - LSS assign node-ID to LSS address
    - LSS complete node-ID configuration
- Amendment 7 to Fieldbus Integration into PROFINET IO (CiA 309-4)

CiA 315 v1.0.0: Generic frame: lely/co/wtm.h
--------------------------------------------

[can2udp](@ref md_doc_can2udp) forms a bridge between the CAN bus and UDP by
converting CAN frames to and from generic frames.

