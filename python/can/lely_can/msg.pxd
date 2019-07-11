from libc.stdint cimport *

cdef extern from "lely/can/msg.h":
    enum:
        _CAN_MASK_BID "CAN_MASK_BID"
        _CAN_MASK_EID "CAN_MASK_EID"
        _CAN_FLAG_IDE "CAN_FLAG_IDE"
        _CAN_FLAG_RTR "CAN_FLAG_RTR"
        _CAN_FLAG_FDF "CAN_FLAG_FDF"
        _CAN_FLAG_EDL "CAN_FLAG_EDL"
        _CAN_FLAG_BRS "CAN_FLAG_BRS"
        _CAN_FLAG_ESI "CAN_FLAG_ESI"
        _CAN_MAX_LEN "CAN_MAX_LEN"
        _CANFD_MAX_LEN "CANFD_MAX_LEN"
        _CAN_MSG_MAX_LEN "CAN_MSG_MAX_LEN"

    cdef struct can_msg:
        uint32_t id
        uint8_t flags
        uint8_t len
        uint8_t data[_CAN_MSG_MAX_LEN]

    enum can_msg_bits_mode:
        _CAN_MSG_BITS_MODE_NO_STUFF "CAN_MSG_BITS_MODE_NO_STUFF"
        _CAN_MSG_BITS_MODE_WORST "CAN_MSG_BITS_MODE_WORST"
        _CAN_MSG_BITS_MODE_EXACT "CAN_MSG_BITS_MODE_EXACT"

    int can_msg_bits(const can_msg* msg, can_msg_bits_mode mode) nogil


cdef class CANMsg(object):
    cdef can_msg _c_msg


cdef CANMsg CANMsg_new(const can_msg* msg)

