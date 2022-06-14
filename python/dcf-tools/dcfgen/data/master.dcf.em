@{
software_update = False
for slave in slaves.values():
    if slave.software_file:
        software_update = True
configuration_update = False
for slave in slaves.values():
    if slave.configuration_file:
        configuration_update = True
}@
[DeviceComissioning]
NodeID=@master.node_id
NodeName=
NodeRefd=
Baudrate=@master.baudrate
NetNumber=1
NetworkName=
NetRefd=
CANopenManager=1
LSS_SerialNumber=@("0x{:08X}".format(master.serial_number))

[DeviceInfo]
VendorName=
VendorNumber=@("0x{:08X}".format(master.vendor_id))
ProductName=
ProductNumber=@("0x{:08X}".format(master.product_code))
RevisionNumber=@("0x{:08X}".format(master.revision_number))
OrderCode=
BaudRate_10=1
BaudRate_20=1
BaudRate_50=1
BaudRate_125=1
BaudRate_250=1
BaudRate_500=1
BaudRate_800=1
BaudRate_1000=1
SimpleBootUpMaster=1
SimpleBootUpSlave=0
Granularity=1
DynamicChannelsSupported=0
GroupMessaging=0
@{nrpdo = sum(len(slave.tpdo) for slave in slaves.values())}@
NrOfRxPDO=@nrpdo
@{ntpdo = sum(len(slave.rpdo) for slave in slaves.values())}@
NrOfTxPDO=@ntpdo
LSS_Supported=1

[DummyUsage]
Dummy0001=1
Dummy0002=1
Dummy0003=1
Dummy0004=1
Dummy0005=1
Dummy0006=1
Dummy0007=1
Dummy0010=1
Dummy0012=1
Dummy0013=1
Dummy0014=1
Dummy0015=1
Dummy0016=1
Dummy0018=1
Dummy0019=1
Dummy001A=1
Dummy001B=1

[MandatoryObjects]
SupportedObjects=3
1=0x1000
2=0x1001
3=0x1018

[OptionalObjects]
SupportedObjects=@(8 + master.heartbeat_consumer + 5 + 2 * nrpdo + 2 * ntpdo + configuration_update + 2 + software_update + 10)
1=0x1003
2=0x1005
3=0x1006
4=0x1007
5=0x1012
6=0x1013
7=0x1014
8=0x1015
@{n = 8}@
@[if master.heartbeat_consumer]@
@{n += 1}@
@n=0x1016
@[end if]@
@(n + 1)=0x1017
@(n + 2)=0x1019
@(n + 3)=0x1028
@(n + 4)=0x1029
@(n + 5)=0x102A
@{n += 5}@
@[for i in range(nrpdo)]@
@{n += 1}@
@n=@("0x{:04X}".format(0x1400 + i))
@[end for]@
@[for i in range(nrpdo)]@
@{n += 1}@
@n=@("0x{:04X}".format(0x1600 + i))
@[end for]@
@[for i in range(ntpdo)]@
@{n += 1}@
@n=@("0x{:04X}".format(0x1800 + i))
@[end for]@
@[for i in range(ntpdo)]@
@{n += 1}@
@n=@("0x{:04X}".format(0x1A00 + i))
@[end for]@
@[if configuration_update]@
@{n += 1}@
@n=0x1F22
@[end if]@
@(n + 1)=0x1F25
@(n + 2)=0x1F55
@{n += 2}@
@[if software_update]@
@{n += 1}@
@n=0x1F58
@[end if]@
@(n + 1)=0x1F80
@(n + 2)=0x1F81
@(n + 3)=0x1F82
@(n + 4)=0x1F84
@(n + 5)=0x1F85
@(n + 6)=0x1F86
@(n + 7)=0x1F87
@(n + 8)=0x1F88
@(n + 9)=0x1F89
@(n + 10)=0x1F8A

[ManufacturerObjects]
@{
# Determine the indices of the RPDO-mapped objects.
ridx = set()
n = 0
for slave in slaves.values():
    mapped = set()
    for pdo in slave.tpdo.values():
        n += 1
        for subobj in pdo.mapping.values():
            if (subobj.index, subobj.sub_index) not in mapped:
                mapped.add((subobj.index, subobj.sub_index))
                ridx.add(0x2000 + n - 1)
# Determine the indices of the TPDO-mapped objects.
tidx = set()
n = 0
for slave in slaves.values():
    mapped = set()
    for pdo in slave.rpdo.values():
        n += 1
        for subobj in pdo.mapping.values():
            if (subobj.index, subobj.sub_index) not in mapped:
                mapped.add((subobj.index, subobj.sub_index))
                tidx.add(0x2200 + n - 1)
}@
@[if remote_pdo]@
SupportedObjects=@(len(ridx) + len(tidx) + 2 * nrpdo + 2 * ntpdo)
@[else]@
SupportedObjects=@(len(ridx) + len(tidx))
@[end if]@
@{n = 0}@
@[for idx in sorted(ridx)]@
@{n += 1}@
@n=@("0x{:04X}".format(idx))
@[end for]@
@[for idx in sorted(tidx)]@
@{n += 1}@
@n=@("0x{:04X}".format(idx))
@[end for]@
@[if remote_pdo]@
@[for i in range(nrpdo)]@
@{n += 1}@
@n=@("0x{:04X}".format(0x5800 + i))
@[end for]@
@[for i in range(nrpdo)]@
@{n += 1}@
@n=@("0x{:04X}".format(0x5A00 + i))
@[end for]@
@[for i in range(ntpdo)]@
@{n += 1}@
@n=@("0x{:04X}".format(0x5C00 + i))
@[end for]@
@[for i in range(ntpdo)]@
@{n += 1}@
@n=@("0x{:04X}".format(0x5E00 + i))
@[end for]@
@[end if]@

[1000]
ParameterName=Device type
DataType=0x0007
AccessType=ro
DefaultValue=0x00000000

[1001]
ParameterName=Error register
DataType=0x0005
AccessType=ro

[1003]
ParameterName=Pre-defined error field
ObjectType=0x08
DataType=0x0007
AccessType=ro
CompactSubObj=254

[1005]
ParameterName=COB-ID SYNC message
DataType=0x0007
AccessType=rw
DefaultValue=0x40000080

[1006]
ParameterName=Communication cycle period
DataType=0x0007
AccessType=rw
DefaultValue=@master.sync_period

[1007]
ParameterName=Synchronous window length
DataType=0x0007
AccessType=rw
DefaultValue=@master.sync_window

[1012]
ParameterName=COB-ID time stamp object
DataType=0x0007
AccessType=rw
DefaultValue=@("0x{:08X}".format(master.time_cob_id))

[1013]
ParameterName=High resolution time stamp
DataType=0x0007
AccessType=rw
PDOMapping=1

[1014]
ParameterName=COB-ID EMCY
DataType=0x0007
AccessType=rw
DefaultValue=$NODEID+0x80

[1015]
ParameterName=Inhibit time EMCY
DataType=0x0006
AccessType=rw
DefaultValue=@master.emcy_inhibit_time
@[if master.heartbeat_consumer]@

[1016]
ParameterName=Consumer heartbeat time
ObjectType=0x08
DataType=0x0007
AccessType=rw
CompactSubObj=127

[1016Value]
NrOfEntries=@(sum(slave.heartbeat_producer != 0 for slave in slaves.values()))
@{n = 0}@
@[for slave in slaves.values()]@
@[if slave.heartbeat_producer != 0]@
@{n += 1}@
@n=@("0x{:08X}".format(((slave.node_id & 0xFF) << 16) | (int(slave.heartbeat_producer * slave.heartbeat_multiplier) & 0xFFFF)))
@[end if]@
@[end for]@
@[end if]@

[1017]
ParameterName=Producer heartbeat time
DataType=0x0006
AccessType=rw
DefaultValue=@master.heartbeat_producer

[1018]
SubNumber=5
ParameterName=Identity Object
ObjectType=0x09

[1018sub0]
ParameterName=Highest sub-index supported
DataType=0x0005
AccessType=const
DefaultValue=4

[1018sub1]
ParameterName=Vendor-ID
DataType=0x0007
AccessType=ro
DefaultValue=@("0x{:08X}".format(master.vendor_id))

[1018sub2]
ParameterName=Product code
DataType=0x0007
AccessType=ro
DefaultValue=@("0x{:08X}".format(master.product_code))

[1018sub3]
ParameterName=Revision number
DataType=0x0007
AccessType=ro
DefaultValue=@("0x{:08X}".format(master.revision_number))

[1018sub4]
ParameterName=Serial number
DataType=0x0007
AccessType=ro

[1019]
ParameterName=Synchronous counter overflow value
DataType=0x0005
AccessType=rw
DefaultValue=@master.sync_overflow

[1028]
ParameterName=Emergency consumer object
ObjectType=0x08
DataType=0x0007
AccessType=rw
DefaultValue=0x80000000
CompactSubObj=127

[1028Value]
NrOfEntries=@(sum(not (slave.emcy_cob_id & 0x80000000) for slave in slaves.values()))
@[for slave in slaves.values()]@
@[if not (slave.emcy_cob_id & 0x80000000)]@
@slave.node_id=@("0x{:08X}".format(slave.emcy_cob_id))
@[end if]@
@[end for]@

[1029]
ParameterName=Error behavior object
ObjectType=0x08
DataType=0x0005
AccessType=rw
CompactSubObj=254

[1029Value]
NrOfEntries=@len(master.error_behavior)
@[for i in sorted(master.error_behavior)]@
@i=@("0x{:02X}".format(master.error_behavior[i]))
@[end for]@

[102A]
ParameterName=NMT inhibit time
DataType=0x0006
AccessType=rw
DefaultValue=@master.nmt_inhibit_time
@{n = 0}@
@[for slave in slaves.values()]@
@[for pdo in slave.tpdo.values()]@
@{name = "{:04X}".format(0x1400 + n); n += 1}@

[@name]
SubNumber=6
ParameterName=RPDO communication parameter
ObjectType=0x09

[@(name)sub0]
ParameterName=highest sub-index supported
DataType=0x0005
AccessType=const
DefaultValue=5

[@(name)sub1]
ParameterName=COB-ID used by RPDO
DataType=0x0007
AccessType=rw
DefaultValue=@("0x{:08X}".format(pdo.cob_id))

[@(name)sub2]
ParameterName=transmission type
DataType=0x0005
AccessType=rw
DefaultValue=@("0x{:02X}".format(pdo.transmission_type))

[@(name)sub3]
ParameterName=inhibit time
DataType=0x0006
AccessType=rw

[@(name)sub4]
ParameterName=compatibility entry
DataType=0x0005
AccessType=rw

[@(name)sub5]
ParameterName=event-timer
DataType=0x0006
AccessType=rw
DefaultValue=@pdo.event_deadline
@[end for]@
@[end for]@
@{n = 0}@
@[for slave in slaves.values()]@
@{mapping = {}}@
@[for pdo in slave.tpdo.values()]@
@{name = "{:04X}".format(0x1600 + n); n += 1; i = 0; j = 0}@

[@name]
ParameterName=RPDO mapping parameter
ObjectType=0x09
DataType=0x0007
AccessType=rw
CompactSubObj=@len(pdo.mapping)

[@(name)Value]
NrOfEntries=@len(pdo.mapping)
@[for subobj in pdo.mapping.values()]@
@{
if (subobj.index, subobj.sub_index) not in mapping:
    i += 1
    mapping[(subobj.index, subobj.sub_index)] = "0x{:04X}{:02X}{:02X}".format(
        0x2000 + n - 1, i, subobj.data_type.bits()
    )
}@
@{j += 1}@
@j=@mapping[(subobj.index, subobj.sub_index)]
@[end for]@
@[end for]@
@[end for]@
@{n = 0}@
@[for slave in slaves.values()]@
@[for pdo in slave.rpdo.values()]@
@{name = "{:04X}".format(0x1800 + n); n += 1}@

[@name]
SubNumber=7
ParameterName=TPDO communication parameter
ObjectType=0x09

[@(name)sub0]
ParameterName=highest sub-index supported
DataType=0x0005
AccessType=const
DefaultValue=6

[@(name)sub1]
ParameterName=COB-ID used by TPDO
DataType=0x0007
AccessType=rw
DefaultValue=@("0x{:08X}".format(pdo.cob_id))

[@(name)sub2]
ParameterName=transmission type
DataType=0x0005
AccessType=rw
DefaultValue=@("0x{:02X}".format(pdo.transmission_type))

[@(name)sub3]
ParameterName=inhibit time
DataType=0x0006
AccessType=rw
DefaultValue=@pdo.inhibit_time

[@(name)sub4]
ParameterName=reserved
DataType=0x0005
AccessType=rw

[@(name)sub5]
ParameterName=event timer
DataType=0x0006
AccessType=rw
DefaultValue=@pdo.event_timer

[@(name)sub6]
ParameterName=SYNC start value
DataType=0x0005
AccessType=rw
DefaultValue=@pdo.sync_start_value
@[end for]@
@[end for]@
@{n = 0}@
@[for slave in slaves.values()]@
@{mapping = {}}@
@[for pdo in slave.rpdo.values()]@
@{name = "{:04X}".format(0x1A00 + n); n += 1; i = 0; j = 0}@

[@name]
ParameterName=TPDO mapping parameter
ObjectType=0x09
DataType=0x0007
AccessType=rw
CompactSubObj=@len(pdo.mapping)

[@(name)Value]
NrOfEntries=@len(pdo.mapping)
@[for subobj in pdo.mapping.values()]@
@{
if (subobj.index, subobj.sub_index) not in mapping:
    i += 1
    mapping[(subobj.index, subobj.sub_index)] = "0x{:04X}{:02X}{:02X}".format(
        0x2200 + n - 1, i, subobj.data_type.bits()
    )
}@
@{j += 1}@
@j=@mapping[(subobj.index, subobj.sub_index)]
@[end for]@
@[end for]@
@[end for]@
@[if configuration_update]@

[1F22]
SubNumber=128
ParameterName=Concise DCF
ObjectType=0x08

[1F22sub0]
ParameterName=Highest sub-index supported
DataType=0x0005
AccessType=const
DefaultValue=127
@[for i in range(127)]@

[1F22sub@("{:X}".format(i + 1))]
ParameterName=Node-ID @(i + 1)
DataType=0x000F
AccessType=ro
@[for name, slave in slaves.items()]@
@[if slave.node_id == i + 1 and slave.configuration_file]@
UploadFile=@slave.configuration_file
@[end if]@
@[end for]@
@[end for]@
@[end if]@

[1F25]
ParameterName=Configuration request
ObjectType=0x08
DataType=0x0005
AccessType=wo
CompactSubObj=127

[1F55]
ParameterName=Expected software identification
ObjectType=0x08
DataType=0x0007
AccessType=rw
CompactSubObj=127
@[if software_update]@

[1F58]
SubNumber=128
ParameterName=Program data
ObjectType=0x08

[1F58sub0]
ParameterName=Highest sub-index supported
DataType=0x0005
AccessType=const
DefaultValue=127
@[for i in range(127)]@

[1F58sub@("{:X}".format(i + 1))]
ParameterName=Program data of node-ID @(i + 1)
DataType=0x000F
AccessType=ro
@[for slave in slaves.values()]@
@[if slave.node_id == i + 1 and slave.software_file]@
UploadFile=@slave.software_file
@[end if]@
@[end for]@
@[end for]@
@[end if]@

[1F80]
ParameterName=NMT startup
DataType=0x0007
AccessType=rw
@{
value=0x00000001
if master.start_all_nodes:
    value |= 0x02
if not master.start:
    value |= 0x04
if not master.start_nodes:
    value |= 0x08
if master.reset_all_nodes:
    value |= 0x10
if master.stop_all_nodes:
    value |= 0x40
}@
DefaultValue=@("0x{:08X}".format(value))

[1F81]
ParameterName=NMT slave assignment
ObjectType=0x08
DataType=0x0007
AccessType=rw
CompactSubObj=127

[1F81Value]
NrOfEntries=@len(slaves)
@[for slave in slaves.values()]@
@{
value = 0x00000001
if slave.boot:
    value |= 0x04
if slave.mandatory:
    value |= 0x08
if not slave.reset_communication:
    value |= 0x10
if slave.software_version != 0:
    value |= 0x20
if slave.software_file:
    value |= 0x40
if slave.restore_configuration != 0:
    value |= 0x80
if slave.retry_factor != 0:
    value |= slave.retry_factor << 8
if slave.guard_time != 0:
    value |= slave.guard_time << 16
}@
@slave.node_id=@("0x{:08X}".format(value))
@[end for]@

[1F82]
ParameterName=Request NMT
ObjectType=0x08
DataType=0x0005
AccessType=rw
CompactSubObj=127

[1F84]
ParameterName=Device type identification
ObjectType=0x08
DataType=0x0007
AccessType=rw
CompactSubObj=127

[1F84Value]
NrOfEntries=@(sum(slave.device_type != 0 for slave in slaves.values()))
@[for slave in slaves.values()]@
@[if slave.device_type != 0]@
@slave.node_id=@("0x{:08X}".format(slave.device_type))
@[end if]@
@[end for]@

[1F85]
ParameterName=Vendor identification
ObjectType=0x08
DataType=0x0007
AccessType=rw
CompactSubObj=127

[1F85Value]
NrOfEntries=@(sum(slave.vendor_id != 0 for slave in slaves.values()))
@[for slave in slaves.values()]@
@[if slave.vendor_id != 0]@
@slave.node_id=@("0x{:08X}".format(slave.vendor_id))
@[end if]@
@[end for]@

[1F86]
ParameterName=Product code
ObjectType=0x08
DataType=0x0007
AccessType=rw
CompactSubObj=127

[1F86Value]
NrOfEntries=@(sum(slave.product_code != 0 for slave in slaves.values()))
@[for slave in slaves.values()]@
@[if slave.product_code != 0 and 0x02 in slave[0x1018]]@
@slave.node_id=@("0x{:08X}".format(slave.product_code))
@[end if]@
@[end for]@

[1F87]
ParameterName=Revision_number
ObjectType=0x08
DataType=0x0007
AccessType=rw
CompactSubObj=127

[1F88]
ParameterName=Serial number
ObjectType=0x08
DataType=0x0007
AccessType=rw
CompactSubObj=127

[1F89]
ParameterName=Boot time
DataType=0x0007
AccessType=rw
DefaultValue=@master.boot_time

[1F8A]
ParameterName=Restore configuration
ObjectType=0x08
DataType=0x0005
AccessType=rw
CompactSubObj=127

[1F8AValue]
NrOfEntries=@(sum(slave.restore_configuration != 0 for slave in slaves.values()))
@[for slave in slaves.values()]@
@[if slave.restore_configuration != 0]@
@slave.node_id=@("0x{:02X}".format(slave.restore_configuration))
@[end if]@
@[end for]@
@{n = 0}@
@[for slave in slaves.values()]@
@{mapped = set()}@
@[for pdo in slave.tpdo.values()]@
@{
n += 1
mapping = {}
for i, subobj in pdo.mapping.items():
    if (subobj.index, subobj.sub_index) not in mapped:
        mapped.add((subobj.index, subobj.sub_index))
        mapping[i] = subobj
}@
@[if mapping]@
@{name = "{:04X}".format(0x2000 + n - 1); i = 0}@

[@name]
SubNumber=@(len(mapping) + 1)
ParameterName=Mapped application objects for RPDO @n
ObjectType=0x09

[@(name)sub0]
ParameterName=Highest sub-index supported
DataType=0x0005
AccessType=const
DefaultValue=@(len(mapping))
@[for subobj in mapping.values()]@
@{i += 1}@

[@(name)sub@("{:X}".format(i))]
ParameterName=@slave.name: @subobj.name
DataType=@("0x{:04X}".format(subobj.data_type.index))
AccessType=rww
PDOMapping=1
@[end for]@
@[end if]@
@[end for]@
@[end for]@
@{n = 0}@
@[for slave in slaves.values()]@
@{mapped = set()}@
@[for pdo in slave.rpdo.values()]@
@{
n += 1
mapping = {}
for i, subobj in pdo.mapping.items():
    if (subobj.index, subobj.sub_index) not in mapped:
        mapped.add((subobj.index, subobj.sub_index))
        mapping[i] = subobj
}@
@[if mapping]@
@{name = "{:04X}".format(0x2200 + n - 1); i = 0}@

[@name]
SubNumber=@(len(mapping) + 1)
ParameterName=Mapped application objects for TPDO @n
ObjectType=0x09

[@(name)sub0]
ParameterName=Highest sub-index supported
DataType=0x0005
AccessType=const
DefaultValue=@(len(mapping))
@[for subobj in mapping.values()]@
@{i += 1}@

[@(name)sub@("{:X}".format(i))]
ParameterName=@slave.name: @subobj.name
DataType=@("0x{:04X}".format(subobj.data_type.index))
AccessType=rwr
PDOMapping=1
@[end for]@
@[end if]@
@[end for]@
@[end for]@
@[if remote_pdo]@
@{n = 0}@
@[for slave in slaves.values()]@
@[for i, pdo in slave.tpdo.items()]@
@{name = "{:04X}".format(0x5800 + n); n += 1}@

[@name]
ParameterName=Remote TPDO number and node-ID
DataType=0x0007
AccessType=rw
DefaultValue=@("0x{:06X}{:02X}".format(i, slave.node_id))
@[end for]@
@[end for]@
@{n = 0}@
@[for slave in slaves.values()]@
@[for pdo in slave.tpdo.values()]@
@{name = "{:04X}".format(0x5A00 + n); n += 1; i = 0}@

[@name]
ParameterName=Remote TPDO mapping parameter
ObjectType=0x08
DataType=0x0007
AccessType=rw
CompactSubObj=@len(pdo.mapping)

[@(name)Value]
NrOfEntries=@len(pdo.mapping)
@[for subobj in pdo.mapping.values()]@
@{i += 1}@
@i=@("0x{:04X}{:02X}{:02X}".format(subobj.index, subobj.sub_index, subobj.data_type.bits()))
@[end for]@
@[end for]@
@[end for]@
@{n = 0}@
@[for slave in slaves.values()]@
@[for i, pdo in slave.rpdo.items()]@
@{name = "{:04X}".format(0x5C00 + n); n += 1}@

[@name]
ParameterName=Remote RPDO number and node-ID
DataType=0x0007
AccessType=rw
DefaultValue=@("0x{:06X}{:02X}".format(i, slave.node_id))
@[end for]@
@[end for]@
@{n = 0}@
@[for slave in slaves.values()]@
@[for pdo in slave.rpdo.values()]@
@{name = "{:04X}".format(0x5E00 + n); n += 1; i = 0}@

[@name]
ParameterName=Remote RPDO mapping parameter
ObjectType=0x08
DataType=0x0007
AccessType=rw
CompactSubObj=@len(pdo.mapping)

[@(name)Value]
NrOfEntries=@len(pdo.mapping)
@[for subobj in pdo.mapping.values()]@
@{i += 1}@
@i=@("0x{:04X}{:02X}{:02X}".format(subobj.index, subobj.sub_index, subobj.data_type.bits()))
@[end for]@
@[end for]@
@[end for]@
@[end if]@
