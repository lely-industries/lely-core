cdef class IOCAN(IOHandle):
    @staticmethod
    def open(str path):
        cdef bytes _path = path.encode('UTF8')
        cdef char* __path = _path
        cdef io_handle_t handle
        with nogil:
            handle = io_open_can(__path)
        if handle is NULL:
            io_error()
        try:
            return IOCAN_new(handle)
        except:
            with nogil:
                io_handle_release(handle)
            raise

    def read(self):
        cdef CANMsg msg = CANMsg()
        cdef int result
        with nogil:
            result = io_can_read(self._c_handle, &msg._c_msg)
        if result == 1:
            return msg
        elif result == -1:
            io_error()

    def write(self, CANMsg msg not None):
        cdef int result
        with nogil:
            result = io_can_write(self._c_handle, &msg._c_msg)
        if result == -1:
            io_error()

    def start(self):
        cdef int result
        with nogil:
            result = io_can_start(self._c_handle)
        if result == -1:
            io_error()

    def stop(self):
        cdef int result
        with nogil:
            result = io_can_stop(self._c_handle)
        if result == -1:
            io_error()

    property state:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_can_get_state(self._c_handle)
            if result == -1:
                io_error()
            return result

    property error:
        def __get__(self):
            cdef int value = 0
            cdef int result
            with nogil:
                result = io_can_get_error(self._c_handle, &value)
            if result == -1:
                io_error()
            return value

    property ec:
        def __get__(self):
            cdef uint16_t txec = 0
            cdef uint16_t rxec = 0
            cdef int result
            with nogil:
                result = io_can_get_ec(self._c_handle, &txec, &rxec)
            if result == -1:
                io_error()
            return [txec, rxec]

    property bitrate:
        def __get__(self):
            cdef uint32_t value = 0
            cdef int result
            with nogil:
                result = io_can_get_bitrate(self._c_handle, &value)
            if result == -1:
                io_error()
            return value

        def __set__(self, uint32_t value):
            cdef int result
            with nogil:
                result = io_can_set_bitrate(self._c_handle, value)
            if result == -1:
                io_error()

    property txqlen:
        def __get__(self):
            cdef size_t value = 0
            cdef int result
            with nogil:
                result = io_can_get_txqlen(self._c_handle, &value)
            if result == -1:
                io_error()
            return value

        def __set__(self, size_t value):
            cdef int result
            with nogil:
                result = io_can_set_txqlen(self._c_handle, value)
            if result == -1:
                io_error()


cdef IOCAN IOCAN_new(io_handle_t handle):
    cdef IOCAN obj = IOCAN()
    obj._c_handle = handle
    return obj

