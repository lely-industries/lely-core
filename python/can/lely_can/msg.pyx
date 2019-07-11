__all__ = [
    'CAN_MASK_BID',
    'CAN_MASK_EID',
    'CAN_FLAG_IDE',
    'CAN_FLAG_RTR',
    'CAN_FLAG_FDF',
    'CAN_FLAG_EDL',
    'CAN_FLAG_BRS',
    'CAN_FLAG_ESI',
    'CAN_MAX_LEN',
    'CANFD_MAX_LEN',
    'CAN_MSG_MAX_LEN',
    'CANMsg',
    'CAN_MSG_BITS_MODE_NO_STUFF',
    'CAN_MSG_BITS_MODE_WORST',
    'CAN_MSG_BITS_MODE_EXACT'
]

CAN_MASK_BID = _CAN_MASK_BID
CAN_MASK_EID = _CAN_MASK_EID
CAN_FLAG_IDE = _CAN_FLAG_IDE
CAN_FLAG_RTR = _CAN_FLAG_RTR
CAN_FLAG_FDF = _CAN_FLAG_FDF
CAN_FLAG_EDL = _CAN_FLAG_EDL
CAN_FLAG_BRS = _CAN_FLAG_BRS
CAN_FLAG_ESI = _CAN_FLAG_ESI
CAN_MAX_LEN = _CAN_MAX_LEN
CANFD_MAX_LEN = _CANFD_MAX_LEN
CAN_MSG_MAX_LEN = _CAN_MSG_MAX_LEN


cdef class CANMsg(object):
    property id:
        def __get__(self):
            return self._c_msg.id

        def __set__(self, uint32_t value):
            self._c_msg.id = value

    property flags:
        def __get__(self):
            return self._c_msg.flags

        def __set__(self, uint8_t value):
            self._c_msg.flags = value

    property data:
        def __get__(self):
            cdef uint8_t[::1] view
            if self._c_msg.len > 0:
                view = <uint8_t[:self._c_msg.len]>self._c_msg.data
            else:
                view = None
            return view

        def __set__(self, value not None):
            self._c_msg.len = min(len(value), _CAN_MSG_MAX_LEN)
            for i in range(self._c_msg.len):
                self._c_msg.data[i] = value[i]

    def bits(self, can_msg_bits_mode mode = _CAN_MSG_BITS_MODE_EXACT):
            return can_msg_bits(&self._c_msg, mode)


cdef CANMsg CANMsg_new(const can_msg* msg):
    cdef CANMsg obj = CANMsg()
    obj._c_msg.id = msg.id
    obj._c_msg.flags = msg.flags
    obj._c_msg.len = min(msg.len, _CAN_MSG_MAX_LEN)
    for i in range(obj._c_msg.len):
        obj._c_msg.data[i] = msg.data[i]
    return obj

CAN_MSG_BITS_MODE_NO_STUFF = _CAN_MSG_BITS_MODE_NO_STUFF
CAN_MSG_BITS_MODE_WORST = _CAN_MSG_BITS_MODE_WORST
CAN_MSG_BITS_MODE_EXACT = _CAN_MSG_BITS_MODE_EXACT

