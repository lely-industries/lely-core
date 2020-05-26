/**@file
 * This header file is part of the CANopen library; it contains the gateway
 * declarations (see CiA 309-1 version 2.0).
 *
 * @copyright 2020 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LELY_CO_GW_H_
#define LELY_CO_GW_H_

#include <lely/co/dev.h>
#include <lely/libc/time.h>

/// The maximum number of networks in a CANopen gateway.
#define CO_GW_NUM_NET 127

/// The high number of the version of CiA 309-1 implemented by this gateway.
#define CO_GW_PROT_HI 2

/// The low number of the version of CiA 309-1 implemented by this gateway.
#define CO_GW_PROT_LO 0

/// CANopen gateway service: SDO upload.
#define CO_GW_SRV_SDO_UP 0x11

/// CANopen gateway service: SDO download.
#define CO_GW_SRV_SDO_DN 0x13

/// CANopen gateway service: Configure SDO time-out.
#define CO_GW_SRV_SET_SDO_TIMEOUT 0x14

/// CANopen gateway service: Configure RPDO.
#define CO_GW_SRV_SET_RPDO 0x21

/// CANopen gateway service: Configure TPDO.
#define CO_GW_SRV_SET_TPDO 0x22

/// CANopen gateway service: Read PDO data.
#define CO_GW_SRV_PDO_READ 0x23

/// CANopen gateway service: Write PDO data.
#define CO_GW_SRV_PDO_WRITE 0x24

/// CANopen gateway service: RPDO received.
#define CO_GW_SRV_RPDO 0x25

/// CANopen gateway service: Start node.
#define CO_GW_SRV_NMT_START 0x31

/// CANopen gateway service: Start node.
#define CO_GW_SRV_NMT_STOP 0x32

/// CANopen gateway service: Set node to pre-operational.
#define CO_GW_SRV_NMT_ENTER_PREOP 0x33

/// CANopen gateway service: Reset node.
#define CO_GW_SRV_NMT_RESET_NODE 0x34

/// CANopen gateway service: Reset communication.
#define CO_GW_SRV_NMT_RESET_COMM 0x35

/// CANopen gateway service: Enable node guarding.
#define CO_GW_SRV_NMT_NG_ENABLE 0x36

/// CANopen gateway service: Disable node guarding.
#define CO_GW_SRV_NMT_NG_DISABLE 0x37

/// CANopen gateway service: Start heartbeat consumer.
#define CO_GW_SRV_NMT_HB_ENABLE 0x38

/// CANopen gateway service: Disable heartbeat consumer.
#define CO_GW_SRV_NMT_HB_DISABLE 0x39

/// CANopen gateway service: Error control event received.
#define CO_GW_SRV_EC 0x3a

/// CANopen gateway service: Emergency event received.
#define CO_GW_SRV_EMCY 0x42

/// CANopen gateway service: Initialize gateway.
#define CO_GW_SRV_INIT 0x51

/// CANopen gateway service: Set heartbeat producer.
#define CO_GW_SRV_SET_HB 0x54

/// CANopen gateway service: Set node-ID.
#define CO_GW_SRV_SET_ID 0x55

/// CANopen gateway service: Start emergency consumer.
#define CO_GW_SRV_EMCY_START 0x56

/// CANopen gateway service: Stop emergency consumer.
#define CO_GW_SRV_EMCY_STOP 0x57

/// CANopen gateway service: Set command time-out.
#define CO_GW_SRV_SET_CMD_TIMEOUT 0x58

/// CANopen gateway service: Boot-up forwarding.
#define CO_GW_SRV_SET_BOOTUP_IND 0x59

/// CANopen gateway service: Set default network.
#define CO_GW_SRV_SET_NET 0x61

/// CANopen gateway service: Set default node-ID.
#define CO_GW_SRV_SET_NODE 0x62

/// CANopen gateway service: Get version.
#define CO_GW_SRV_GET_VERSION 0x63

/// CANopen gateway service: Set command size.
#define CO_GW_SRV_SET_CMD_SIZE 0x64

/// CANopen gateway service: LSS switch state global.
#define CO_GW_SRV_LSS_SWITCH 0x81

/// CANopen gateway service: LSS switch state selective.
#define CO_GW_SRV_LSS_SWITCH_SEL 0x82

/// CANopen gateway service: LSS configure node-ID.
#define CO_GW_SRV_LSS_SET_ID 0x83

/// CANopen gateway service: LSS configure bit-rate.
#define CO_GW_SRV_LSS_SET_RATE 0x84

/// CANopen gateway service: LSS activate new bit-rate.
#define CO_GW_SRV_LSS_SWITCH_RATE 0x85

/// CANopen gateway service: LSS store configuration.
#define CO_GW_SRV_LSS_STORE 0x86

/// CANopen gateway service: Inquire LSS address.
#define CO_GW_SRV_LSS_GET_LSSID 0x87

/// CANopen gateway service: LSS inquire node-ID.
#define CO_GW_SRV_LSS_GET_ID 0x88

/// CANopen gateway service: LSS identify remote slave.
#define CO_GW_SRV_LSS_ID_SLAVE 0x89

/// CANopen gateway service: LSS identify non-configured remote slaves.
#define CO_GW_SRV_LSS_ID_NON_CFG_SLAVE 0x8a

/// CANopen gateway service: CiA 301 progress indication download.
#define CO_GW_SRV_SDO 0x93

/// Lely-specific gateway service: LSS Slowscan.
#define CO_GW_SRV__LSS_SLOWSCAN 0x101

/// Lely-specific gateway service: LSS Fastscan.
#define CO_GW_SRV__LSS_FASTSCAN 0x102

/// Lely-specific gateway service: Synchronization event received.
#define CO_GW_SRV__SYNC 0x103

/// Lely-specific gateway service: Time stamp event received.
#define CO_GW_SRV__TIME 0x104

/// Lely-specific gateway service: Boot slave process completed.
#define CO_GW_SRV__BOOT 0x105

/// CANopen gateway internal error: Request not supported.
#define CO_GW_IEC_BAD_SRV 100

/// CANopen gateway internal error: Syntax error.
#define CO_GW_IEC_SYNTAX 101

/// CANopen gateway internal error: Request not processed due to internal state.
#define CO_GW_IEC_INTERN 102

/// CANopen gateway internal error: Time-out.
#define CO_GW_IEC_TIMEOUT 103

/// CANopen gateway internal error: No default net set.
#define CO_GW_IEC_NO_DEF_NET 104

/// CANopen gateway internal error: No default node set.
#define CO_GW_IEC_NO_DEF_NODE 105

/// CANopen gateway internal error: Unsupported net.
#define CO_GW_IEC_BAD_NET 106

/// CANopen gateway internal error: Unsupported node.
#define CO_GW_IEC_BAD_NODE 107

/// CANopen gateway internal error: Lost guarding message.
#define CO_GW_IEC_NG_OCCURRED 200

/// CANopen gateway internal error: Lost connection.
#define CO_GW_IEC_LG_OCCURRED 201

/// CANopen gateway internal error: Heartbeat started.
#define CO_GW_IEC_HB_RESOLVED 202

/// CANopen gateway internal error: Heartbeat lost.
#define CO_GW_IEC_HB_OCCURRED 203

/// CANopen gateway internal error: Wrong NMT state.
#define CO_GW_IEC_ST_OCCURRED 204

/// CANopen gateway internal error: Boot-up.
#define CO_GW_IEC_BOOTUP 205

/// CANopen gateway internal error: Error passive.
#define CO_GW_IEC_CAN_PASSIVE 300

/// CANopen gateway internal error: Bus off.
#define CO_GW_IEC_CAN_BUSOFF 301

/// CANopen gateway internal error: CAN buffer overflow.
#define CO_GW_IEC_CAN_OVERFLOW 303

/// CANopen gateway internal error: CAN init.
#define CO_GW_IEC_CAN_INIT 304

/// CANopen gateway internal error: CAN active.
#define CO_GW_IEC_CAN_ACTIVE 305

/// CANopen gateway internal error: PDO already used.
#define CO_GW_IEC_PDO_INUSE 400

/// CANopen gateway internal error: PDO length exceeded.
#define CO_GW_IEC_PDO_LEN 401

/// CANopen gateway internal error: LSS error.
#define CO_GW_IEC_LSS 501

/// CANopen gateway internal error: LSS node-ID not supported.
#define CO_GW_IEC_LSS_ID 502

/// CANopen gateway internal error: LSS bit-rate not supported.
#define CO_GW_IEC_LSS_RATE 503

/// CANopen gateway internal error: LSS parameter storing failed.
#define CO_GW_IEC_LSS_PARAM 504

/// CANopen gateway internal error: LSS command failed because of media error.
#define CO_GW_IEC_LSS_MEDIA 505

/// CANopen gateway internal error: Running out of memory.
#define CO_GW_IEC_NO_MEM 600

struct __co_gw;
#if !defined(__cplusplus) || LELY_NO_CXX
/// An opaque CANopen gateway type.
typedef struct __co_gw co_gw_t;
#endif

/// The common parameters of a CANopen gateway service.
struct co_gw_srv {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number.
	int srv;
};

/// The common parameters of a CANopen gateway request.
struct co_gw_req {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number.
	int srv;
	/// A pointer to user-specified data.
	void *data;
};

/// The common parameters of a CANopen gateway network-level request.
struct co_gw_req_net {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number.
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
};

/// The common parameters of a CANopen gateway node-level request.
struct co_gw_req_node {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number.
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The node-ID.
	co_unsigned8_t node;
};

/// The parameters of a CANopen gateway 'SDO upload' request.
struct co_gw_req_sdo_up {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_SDO_UP).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The node-ID.
	co_unsigned8_t node;
	/// The object index.
	co_unsigned16_t idx;
	/// The object sub-index.
	co_unsigned8_t subidx;
	/// The data type.
	co_unsigned16_t type;
};

/// The parameters of a CANopen gateway 'SDO download' request.
struct co_gw_req_sdo_dn {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_SDO_DN).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The node-ID.
	co_unsigned8_t node;
	/// The object index.
	co_unsigned16_t idx;
	/// The object sub-index.
	co_unsigned8_t subidx;
	/// The length of the value (in bytes).
	co_unsigned32_t len;
	/// The (first byte in the) value.
	uint_least8_t val[1];
};

/// The minimum size (in bytes) of a CANopen gateway 'SDO download' request.
#define CO_GW_REQ_SDO_DN_SIZE offsetof(struct co_gw_req_sdo_dn, val)

/// The parameters of a CANopen gateway 'Configure SDO time-out' request.
struct co_gw_req_set_sdo_timeout {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_SET_SDO_TIMEOUT).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The SDO timeout (in milliseconds).
	int timeout;
};

/// The parameters of a CANopen gateway 'Configure RPDO' request.
struct co_gw_req_set_rpdo {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_SET_RPDO).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The PDO number.
	co_unsigned16_t num;
	/// The COB-ID.
	co_unsigned32_t cobid;
	/// The transmission type.
	co_unsigned8_t trans;
	/// Number of mapped objects in PDO.
	co_unsigned8_t n;
	/// An array of objects to be mapped.
	co_unsigned32_t map[0x40];
};

/// The minimum size (in bytes) of a CANopen gateway 'Configure RPDO' request.
#define CO_GW_REQ_SET_RPDO_SIZE offsetof(struct co_gw_req_set_rpdo, map)

/// The parameters of a CANopen gateway 'Configure TPDO' request.
struct co_gw_req_set_tpdo {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_SET_TPDO).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The PDO number.
	co_unsigned16_t num;
	/// The COB-ID.
	co_unsigned32_t cobid;
	/// The transmission type.
	co_unsigned8_t trans;
	/// The inhibit time.
	co_unsigned16_t inhibit;
	/// The event timer.
	co_unsigned16_t event;
	/// The SYNC start value.
	co_unsigned8_t sync;
	/// Number of mapped objects in PDO.
	co_unsigned8_t n;
	/// An array of objects to be mapped.
	co_unsigned32_t map[0x40];
};

/// The minimum size (in bytes) of a CANopen gateway 'Configure TPDO' request.
#define CO_GW_REQ_SET_TPDO_SIZE offsetof(struct co_gw_req_set_tpdo, map)

/// The parameters of a CANopen gateway 'Read PDO' request.
struct co_gw_req_pdo_read {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_PDO_READ).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The PDO number.
	co_unsigned16_t num;
};

/// The parameters of a CANopen gateway 'Write PDO' request.
struct co_gw_req_pdo_write {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_PDO_WRITE).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The PDO number.
	co_unsigned16_t num;
	/// Number of mapped objects in PDO.
	co_unsigned8_t n;
	/// An array of object values.
	co_unsigned64_t val[0x40];
};

/// The minimum size (in bytes) of a CANopen gateway 'Write PDO' request.
#define CO_GW_REQ_PDO_WRITE_SIZE offsetof(struct co_gw_req_pdo_write, val)

/// The parameters of a CANopen gateway 'Enable node guarding' request.
struct co_gw_req_nmt_set_ng {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_NMT_NG_ENABLE).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The node-ID.
	co_unsigned8_t node;
	/// The guard time (in milliseconds).
	co_unsigned16_t gt;
	/// The lifetime factor.
	co_unsigned8_t ltf;
};

/// The parameters of a CANopen gateway 'Start heartbeat consumer' request.
struct co_gw_req_nmt_set_hb {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_NMT_HB_ENABLE).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The node-ID.
	co_unsigned8_t node;
	/// The heartbeat time (in milliseconds).
	co_unsigned16_t ms;
};

/// The parameters of a CANopen gateway 'Initialize gateway' request.
struct co_gw_req_init {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_INIT).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The bit timing index (in the range [0..9]).
	co_unsigned8_t bitidx;
};

/// The parameters of a CANopen gateway 'Set heartbeat producer' request.
struct co_gw_req_set_hb {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_SET_HB).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The heartbeat time (in milliseconds).
	co_unsigned16_t ms;
};

/// The parameters of a CANopen gateway 'Start/Stop emergency consumer' request.
struct co_gw_req_set_emcy {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_EMCY_START or #CO_GW_SRV_EMCY_STOP).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The node-ID.
	co_unsigned8_t node;
	/// The COB-ID.
	co_unsigned32_t cobid;
};

/// The parameters of a CANopen gateway 'Set command time-out' request.
struct co_gw_req_set_cmd_timeout {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_SET_CMD_TIMEOUT).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The command timeout (in milliseconds).
	int timeout;
};

/// The parameters of a CANopen gateway 'Boot-up forwarding' request.
struct co_gw_req_set_bootup_ind {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_SET_BOOTUP_IND).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/**
	 * A flag indicating whether "boot-up event received" commands should be
	 * forwarded (1) or not (0).
	 */
	unsigned cs : 1;
};

/// The parameters of a CANopen gateway 'Set command size' request.
struct co_gw_req_set_cmd_size {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_SET_CMD_TIMEOUT).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The command size (in bytes).
	co_unsigned32_t n;
};

/// The parameters of a CANopen gateway 'LSS switch state global' request.
struct co_gw_req_lss_switch {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_LSS_SWITCH).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// 0 for waiting state, 1 for configuration state.
	co_unsigned8_t mode;
};

/// The parameters of a CANopen gateway 'LSS switch state selective' request.
struct co_gw_req_lss_switch_sel {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_LSS_SWITCH_SEL).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The LSS address of the slave to be configured.
	struct co_id id;
};

/// The parameters of a CANopen gateway 'LSS configure bit-rate' request.
struct co_gw_req_lss_set_rate {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_LSS_SET_RATE).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The bit timing selector.
	co_unsigned8_t bitsel;
	/// The bit timing index.
	co_unsigned8_t bitidx;
};

/// The parameters of a CANopen gateway 'LSS activate new bit-rate' request.
struct co_gw_req_lss_switch_rate {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_LSS_SWITCH_RATE).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The delay (in milliseconds).
	co_unsigned16_t delay;
};

/// The parameters of a CANopen gateway 'Inquire LSS address' request.
struct co_gw_req_lss_get_lssid {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_LSS_GET_LSSID).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The command specifier (one of 0x5a, 0x5b, 0x5c or 0x5d).
	co_unsigned8_t cs;
};

/// The parameters of a CANopen gateway 'LSS identify remote slave' request.
struct co_gw_req_lss_id_slave {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_LSS_ID_SLAVE).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/// The lower bound of the LSS address.
	struct co_id lo;
	/// The upper bound of the LSS address.
	struct co_id hi;
};

/// The parameters of a Lely-specific gateway 'LSS Slowscan/Fastscan' request.
struct co_gw_req__lss_scan {
	/// The size of this struct (in bytes).
	size_t size;
	/**
	 * The service number (#CO_GW_SRV__LSS_SLOWSCAN or
	 * #CO_GW_SRV__LSS_FASTSCAN).
	 */
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The network-ID.
	co_unsigned16_t net;
	/**
	 * In case of an LSS Slowscan request, the lower bound of the LSS
	 * address; in case of an LSS Fastscan request, the bits of LSS address
	 * that are already known.
	 */
	struct co_id id_1;
	/**
	 * In case of an LSS Slowscan request, the upper bound of the LSS
	 * address; in case of an LSS Fastscan request, the mask specifying
	 * which bits of the LSS address are already known.
	 */
	struct co_id id_2;
};

/// The common parameters of a CANopen gateway confirmation.
struct co_gw_con {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number.
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The internal error code (0 on success).
	int iec;
	/// The SDO abort code (0 on success).
	co_unsigned32_t ac;
};

/// The parameters of a CANopen gateway 'SDO upload' confirmation.
struct co_gw_con_sdo_up {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_SDO_UP).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The internal error code (0 on success).
	int iec;
	/// The SDO abort code (0 on success).
	co_unsigned32_t ac;
	/// The data type.
	co_unsigned16_t type;
	/// The length of the value (in bytes).
	co_unsigned32_t len;
	/// The (first byte in the) value.
	uint_least8_t val[1];
};

/// The minimum size (in bytes) of a CANopen gateway 'SDO upload' confirmation.
#define CO_GW_CON_SDO_UP_SIZE offsetof(struct co_gw_con_sdo_up, val)

/// The parameters of a CANopen gateway 'Read PDO' confirmation.
struct co_gw_con_pdo_read {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_PDO_READ).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The internal error code (0 on success).
	int iec;
	/// The SDO abort code (0 on success).
	co_unsigned32_t ac;
	/// The network-ID.
	co_unsigned16_t net;
	/// The PDO number.
	co_unsigned16_t num;
	/// Number of mapped objects in PDO.
	co_unsigned8_t n;
	/// An array of object values.
	co_unsigned64_t val[0x40];
};

/// The minimum size (in bytes) of a CANopen gateway 'Read PDO' confirmation.
#define CO_GW_CON_PDO_READ_SIZE offsetof(struct co_gw_con_pdo_read, val)

/// The parameters of a CANopen gateway 'Get version' confirmation.
struct co_gw_con_get_version {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_GET_VERSION).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The internal error code (0 on success).
	int iec;
	/// The SDO abort code (0 on success).
	co_unsigned32_t ac;
	/// The vendor-ID.
	co_unsigned32_t vendor_id;
	/// The product code.
	co_unsigned32_t product_code;
	/// The revision number.
	co_unsigned32_t revision;
	/// The serial number.
	co_unsigned32_t serial_nr;
	/// The gateway class.
	co_unsigned8_t gw_class;
	/// The protocol version (high number).
	co_unsigned8_t prot_hi;
	/// The protocol version (low number).
	co_unsigned8_t prot_lo;
};

/// The parameters of a CANopen gateway 'Inquire LSS address' confirmation.
struct co_gw_con_lss_get_lssid {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_LSS_GET_LSSID).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The internal error code (0 on success).
	int iec;
	/// The SDO abort code (0 on success).
	co_unsigned32_t ac;
	/// The LSS number.
	co_unsigned32_t id;
};

/// The parameters of a CANopen gateway 'LSS inquire node-ID' confirmation.
struct co_gw_con_lss_get_id {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_LSS_GET_ID).
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The internal error code (0 on success).
	int iec;
	/// The SDO abort code (0 on success).
	co_unsigned32_t ac;
	/// The node-ID.
	co_unsigned8_t id;
};

/**
 * The parameters of a Lely-specific gateway 'LSS Slowscan/Fastscan'
 * confirmation.
 */
struct co_gw_con__lss_scan {
	/// The size of this struct (in bytes).
	size_t size;
	/**
	 * The service number (#CO_GW_SRV__LSS_SLOWSCAN or
	 * #CO_GW_SRV__LSS_FASTSCAN).
	 */
	int srv;
	/// A pointer to user-specified data.
	void *data;
	/// The internal error code (0 on success).
	int iec;
	/// The SDO abort code (0 on success).
	co_unsigned32_t ac;
	/// The LSS address.
	struct co_id id;
};

/// The parameters of a CANopen gateway 'RPDO received' indication.
struct co_gw_ind_rpdo {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_RPDO).
	int srv;
	/// The network-ID.
	co_unsigned16_t net;
	/// The PDO number.
	co_unsigned16_t num;
	/// Number of mapped objects in PDO.
	co_unsigned8_t n;
	/// An array of object values.
	co_unsigned64_t val[0x40];
};

/// The minimum size (in bytes) of a CANopen gateway 'RPDO received' indication.
#define CO_GW_IND_RPDO_SIZE offsetof(struct co_gw_ind_rpdo, val)

/**
 * The parameters of a CANopen gateway 'Error control event received'
 * indication.
 */
struct co_gw_ind_ec {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_EC).
	int srv;
	/// The network-ID.
	co_unsigned16_t net;
	/// The node-ID.
	co_unsigned8_t node;
	/// The state of the node, or 0 in case of a boot-up event.
	co_unsigned8_t st;
	/// The internal error code (0 on success).
	int iec;
};

/// The parameters of a CANopen gateway 'Emergency event received' indication.
struct co_gw_ind_emcy {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_EMCY).
	int srv;
	/// The network-ID.
	co_unsigned16_t net;
	/// The node-ID.
	co_unsigned8_t node;
	/// The emergency error code.
	co_unsigned16_t ec;
	/// The error register.
	co_unsigned8_t er;
	/// The manufacturer-specific error code.
	co_unsigned8_t msef[5];
};

/**
 * The parameters of a CANopen gateway 'CiA 301 progress indication download'
 * indication.
 */
struct co_gw_ind_sdo {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV_SDO).
	int srv;
	/// The network-ID.
	co_unsigned16_t net;
	/// The node-ID.
	co_unsigned8_t node;
	/// The transferred bytes.
	co_unsigned32_t nbyte;
	/// The transfer direction: download (0) or upload (1).
	unsigned int up : 1;
	/// A pointer to user-specified data of the SDO upload/download request.
	void *data;
	/// The Lely-specific total value size (in bytes).
	co_unsigned32_t _size;
};

/**
 * The parameters of a Lely-specific gateway 'Synchronization event received'
 * indication.
 */
struct co_gw_ind__sync {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV__SYNC).
	int srv;
	/// The network-ID.
	co_unsigned16_t net;
	/// The SYNC counter.
	co_unsigned8_t cnt;
};

/**
 * The parameters of a Lely-specific gateway 'Time stamp event received'
 * indication.
 */
struct co_gw_ind__time {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV__TIME).
	int srv;
	/// The network-ID.
	co_unsigned16_t net;
	/// The absolute time.
	struct timespec ts;
};

/**
 * The parameters of a Lely-specific gateway 'Boot slave process completed'
 * indication.
 */
struct co_gw_ind__boot {
	/// The size of this struct (in bytes).
	size_t size;
	/// The service number (#CO_GW_SRV__BOOT).
	int srv;
	/// The network-ID.
	co_unsigned16_t net;
	/// The node-ID.
	co_unsigned8_t node;
	/// The the state of the node (including the toggle bit).
	co_unsigned8_t st;
	/// The error status (in the range ['A'..'O'], or 0 on success).
	char es;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a CANopen gateway send callback function, invoked by a gateway
 * when an indication or confirmation needs to be sent.
 *
 * @param srv  a pointer to the service parameters to be sent.
 * @param data a pointer to user-specified data.
 *
 * @returns 0 on success, or -1 on error. In the latter case, implementations
 * SHOULD set the error number with `set_errnum()`.
 */
typedef int co_gw_send_func_t(const struct co_gw_srv *srv, void *data);

/**
 * The type of a CANopen gateway 'set bit timing' function, invoked when a
 * baudrate switch is needed after an 'Initialize gateway' command is received.
 *
 * @param net  the network-ID (in the range [1..127]).
 * @param rate the baudrate (in kbit/s), or 0 for automatic bit rate detection.
 * @param data a pointer to user-specified data.
 */
typedef void co_gw_rate_func_t(
		co_unsigned16_t net, co_unsigned16_t rate, void *data);

/// Returns a string describing an internal error code.
const char *co_gw_iec2str(int iec);

void *__co_gw_alloc(void);
void __co_gw_free(void *ptr);
struct __co_gw *__co_gw_init(struct __co_gw *gw);
void __co_gw_fini(struct __co_gw *gw);

/// Creates a new CANopen gateway. @see co_gw_destroy()
co_gw_t *co_gw_create(void);

/// Destroys a CANopen gateway. @see co_gw_create()
void co_gw_destroy(co_gw_t *gw);

/**
 * Registers a CANopen network with a gateway.
 *
 * @param gw  a pointer to a CANopen gateway.
 * @param id  the network-ID (in the range [1..127]). If <b>id</b> is 0, the
 *            network-ID is obtained from the CANopen device of the NMT service.
 * @param nmt a pointer to the NMT service of the gateway node in the network.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_gw_fini_net()
 */
int co_gw_init_net(co_gw_t *gw, co_unsigned16_t id, co_nmt_t *nmt);

/**
 * Unregisters a CANopen network with a gateway.
 *
 * @param gw a pointer to a CANopen gateway.
 * @param id the network-ID (in the range [1..127]).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_gw_init_net()
 */
int co_gw_fini_net(co_gw_t *gw, co_unsigned16_t id);

/**
 * Receives and processes a request with a CANopen gateway.
 *
 * @param gw  a pointer to a CANopen gateway.
 * @param req a pointer to the parameters of the request.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_gw_recv(co_gw_t *gw, const struct co_gw_req *req);

/**
 * Retrieves the callback function used to send indications and confirmations
 * from a CANopen gateway.
 *
 * @param gw   a pointer to a CANopen gateway.
 * @param pfunc the address at which to store a pointer to the callback function
 *              (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_gw_set_send_func()
 */
void co_gw_get_send_func(
		const co_gw_t *gw, co_gw_send_func_t **pfunc, void **pdata);

/**
 * Sets the callback function used to send indications and confirmations from a
 * CANopen gateway.
 *
 * @param gw   a pointer to a CANopen gateway.
 * @param func a pointer to the function to be invoked when an indication or
 *             confirmation needs to be sent.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>func</b>.
 *
 * @see co_gw_get_send_func()
 */
void co_gw_set_send_func(co_gw_t *gw, co_gw_send_func_t *func, void *data);

/**
 * Retrieves the callback function invoked when a baudrate switch is needed
 * after an 'Initialize gateway' command is received.
 *
 * @param gw   a pointer to a CANopen gateway.
 * @param pfunc the address at which to store a pointer to the callback function
 *              (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_gw_set_rate_func()
 */
void co_gw_get_rate_func(
		const co_gw_t *gw, co_gw_rate_func_t **pfunc, void **pdata);

/**
 * Sets the callback function invoked when a baudrate switch is needed after an
 * 'Initialize gateway' command is received.
 *
 * @param gw   a pointer to a CANopen gateway.
 * @param func a pointer to the function to be invoked when an indication or
 *             confirmation needs to be sent.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>func</b>.
 *
 * @see co_gw_get_rate_func()
 */
void co_gw_set_rate_func(co_gw_t *gw, co_gw_rate_func_t *func, void *data);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_GW_H_
