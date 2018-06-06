cimport cython


cdef class IOPipe(IOHandle):
    @staticmethod
    def open_pipe():
        cdef io_handle_t handle_vector[2]
        cdef int result
        with nogil:
            result = io_open_pipe(handle_vector)
        if result == -1:
            io_error()
        try:
            return [IOPipe_new(handle_vector[0]), IOPipe_new(handle_vector[1])]
        except:
            with nogil:
                io_handle_release(handle_vector[1])
                io_handle_release(handle_vector[0])
            raise


cdef IOPipe IOPipe_new(io_handle_t handle):
    cdef IOPipe obj = IOPipe()
    obj._c_handle = handle
    return obj

