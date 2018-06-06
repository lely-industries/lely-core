from lely_io.io cimport *

cdef extern from "lely/io/serial.h":
    enum:
        _IO_PURGE_RX "IO_PURGE_RX"
        _IO_PURGE_TX "IO_PURGE_TX"

    io_handle_t io_open_serial(const char* path, io_attr_t* attr) nogil

    int io_purge(io_handle_t handle, int flags) nogil

    int io_serial_get_attr(io_handle_t handle, io_attr_t* attr) nogil
    int io_serial_set_attr(io_handle_t handle, const io_attr_t* attr) nogil


cdef class IOSerial(IOHandle):
    pass


cdef IOSerial IOSerial_new(io_handle_t handle)

