from libc.stdint cimport *

cdef void io_error() except *

cdef extern from "lely/io/io.h":
    cdef struct io_handle:
        pass

    ctypedef io_handle* io_handle_t

    enum:
        _IO_HANDLE_ERROR "IO_HANDLE_ERROR"

    ctypedef int64_t io_off_t

    cdef union __io_attr:
        pass

    ctypedef __io_attr io_attr_t

    cdef struct __io_addr:
        pass

    ctypedef __io_addr io_addr_t

    enum:
        _IO_TYPE_CAN "IO_TYPE_CAN"
        _IO_TYPE_FILE "IO_TYPE_FILE"
        _IO_TYPE_SERIAL "IO_TYPE_SERIAL"
        _IO_TYPE_SOCK "IO_TYPE_SOCK"

    enum:
        _IO_FLAG_NO_CLOSE "IO_FLAG_NO_CLOSE"
        _IO_FLAG_NONBLOCK "IO_FLAG_NONBLOCK"
        _IO_FLAG_LOOPBACK "IO_FLAG_LOOPBACK"

    int lely_io_init() nogil
    void lely_io_fini() nogil

    io_handle_t io_handle_acquire(io_handle_t handle) nogil
    void io_handle_release(io_handle_t handle) nogil
    int io_handle_unique(io_handle_t handle) nogil

    int io_close(io_handle_t handle) nogil

    int io_get_type(io_handle_t handle) nogil

    int io_get_flags(io_handle_t handle) nogil
    int io_set_flags(io_handle_t handle, int flags) nogil

    ssize_t io_read(io_handle_t handle, void* buf, size_t nbytes) nogil
    ssize_t io_write(io_handle_t handle, const void* buf, size_t nbytes) nogil

    int io_flush(io_handle_t handle) nogil


cdef class IOHandle(object):
    cdef io_handle_t _c_handle


cdef IOHandle IOHandle_new(io_handle_t handle)

