from lely_io.attr cimport *

IO_PURGE_RX = _IO_PURGE_RX
IO_PURGE_TX = _IO_PURGE_TX


cdef class IOSerial(IOHandle):
    @staticmethod
    def open(str path, IOAttr attr = None):
        cdef bytes _path = path.encode('UTF-8')
        cdef char* __path = _path
        cdef io_attr_t* _attr = NULL
        if attr is not None:
            _attr = &attr._c_attr
        cdef io_handle_t handle
        with nogil:
            handle = io_open_serial(__path, _attr)
        if handle is NULL:
            io_error()
        try:
            return IOSerial_new(handle)
        except:
            with nogil:
                io_handle_release(handle)
            raise

    def purge(self, int flags = _IO_PURGE_RX | _IO_PURGE_TX):
        cdef int result
        with nogil:
            result = io_purge(self._c_handle, flags)
        if result == -1:
            io_error()

    property attr:
        def __get__(self):
            cdef IOAttr value = IOAttr()
            cdef int result
            with nogil:
                result = io_serial_get_attr(self._c_handle, &value._c_attr)
            if result == -1:
                io_error()
            return value

        def __set__(self, IOAttr value not None):
            cdef int result
            with nogil:
                result = io_serial_set_attr(self._c_handle, &value._c_attr)
            if result == -1:
                io_error()


cdef IOSerial IOSerial_new(io_handle_t handle):
    cdef IOSerial obj = IOSerial()
    obj._c_handle = handle
    return obj

