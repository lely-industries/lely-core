from lely_io.io cimport *

cdef extern from "lely/io/pipe.h":
    int io_open_pipe(io_handle_t handle_vector[2]) nogil


cdef class IOPipe(IOHandle):
    pass


cdef IOPipe IOPipe_new(io_handle_t handle)

