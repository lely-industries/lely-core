IO_PARITY_NONE = _IO_PARITY_NONE
IO_PARITY_ODD = _IO_PARITY_ODD
IO_PARITY_EVEN = _IO_PARITY_EVEN


cdef class IOAttr(object):
    property speed:
        def __get__(self):
            cdef int value = io_attr_get_speed(&self._c_attr)
            if value == -1:
                io_error()
            return value

        def __set__(self, int value):
            cdef int result = io_attr_set_speed(&self._c_attr, value)
            if result == -1:
                io_error()

    property flow_control:
        def __get__(self):
            cdef int value = io_attr_get_flow_control(&self._c_attr)
            if value == -1:
                io_error()
            return <bint>value

        def __set__(self, bint value):
            cdef int result = io_attr_set_flow_control(&self._c_attr, value)
            if result == -1:
                io_error()

    property parity:
        def __get__(self):
            cdef int value = io_attr_get_parity(&self._c_attr)
            if value == -1:
                io_error()
            return value

        def __set__(self, int value):
            cdef int result = io_attr_set_parity(&self._c_attr, value)
            if result == -1:
                io_error()

    property stop_bits:
        def __get__(self):
            cdef int value = io_attr_get_stop_bits(&self._c_attr)
            if value == -1:
                io_error()
            return <bint>value

        def __set__(self, bint value):
            cdef int result = io_attr_set_stop_bits(&self._c_attr, value)
            if result == -1:
                io_error()

    property char_size:
        def __get__(self):
            cdef int value = io_attr_get_char_size(&self._c_attr)
            if value == -1:
                io_error()
            return value

        def __set__(self, int value):
            cdef int result = io_attr_set_char_size(&self._c_attr, value)
            if result == -1:
                io_error()


cdef IOAttr IOAttr_new(const io_attr_t* attr):
    cdef IOAttr obj = IOAttr()
    obj._c_attr = attr[0]
    return obj

