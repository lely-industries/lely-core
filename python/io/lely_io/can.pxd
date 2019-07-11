from lely_can.err cimport *
from lely_can.msg cimport *
from lely_io.io cimport *

cdef extern from "lely/io/can.h":
    io_handle_t io_open_can(const char* path) nogil

    int io_can_read(io_handle_t handle, can_msg* msg) nogil
    int io_can_write(io_handle_t handle, const can_msg* msg) nogil

    int io_can_start(io_handle_t handle) nogil
    int io_can_stop(io_handle_t handle) nogil

    int io_can_get_state(io_handle_t handle) nogil
    int io_can_get_error(io_handle_t handle, int* perror) nogil
    int io_can_get_ec(io_handle_t handle, uint16_t* ptxec,
                      uint16_t* prxec) nogil

    int io_can_get_bitrate(io_handle_t handle, uint32_t* pbitrate) nogil
    int io_can_set_bitrate(io_handle_t handle, uint32_t bitrate) nogil

    int io_can_get_txqlen(io_handle_t handle, size_t* ptxqlen) nogil
    int io_can_set_txqlen(io_handle_t handle, size_t txqlen) nogil


cdef class IOCAN(IOHandle):
    pass


cdef IOCAN IOCAN_new(io_handle_t handle)

