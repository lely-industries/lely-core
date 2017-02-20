from lely_io.io cimport *

cdef extern from "lely/io/attr.h":
    enum:
        _IO_PARITY_NONE "IO_PARITY_NONE"
        _IO_PARITY_ODD "IO_PARITY_ODD"
        _IO_PARITY_EVEN "IO_PARITY_EVEN"

    int io_attr_get_speed(const io_attr_t* attr) nogil
    int io_attr_set_speed(io_attr_t* attr, int speed) nogil

    int io_attr_get_flow_control(const io_attr_t* attr) nogil
    int io_attr_set_flow_control(io_attr_t* attr, int flow_control) nogil

    int io_attr_get_parity(const io_attr_t* attr) nogil
    int io_attr_set_parity(io_attr_t* attr, int parity) nogil

    int io_attr_get_stop_bits(const io_attr_t* attr) nogil
    int io_attr_set_stop_bits(io_attr_t* attr, int stop_bits) nogil

    int io_attr_get_char_size(const io_attr_t* attr) nogil
    int io_attr_set_char_size(io_attr_t* attr, int char_size) nogil


cdef class IOAttr(object):
    cdef io_attr_t _c_attr


cdef IOAttr IOAttr_new(const io_attr_t* attr)

