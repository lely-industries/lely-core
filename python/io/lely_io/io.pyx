cimport cython
from cpython.exc cimport PyErr_SetObject

from lely_io.can cimport IOCAN_new
from lely_io.file cimport IOFile_new
from lely_io.serial cimport IOSerial_new
from lely_io.sock cimport IOSock_new

cdef extern from "lely/util/errnum.h":
    int get_errc() nogil
    const char* errc2str(int errc) nogil

cdef void io_error() except *:
    cdef int errc = get_errc()
    PyErr_SetObject(IOError, (errc, errc2str(errc)))

IO_TYPE_CAN = _IO_TYPE_CAN
IO_TYPE_FILE = _IO_TYPE_FILE
IO_TYPE_SERIAL = _IO_TYPE_SERIAL
IO_TYPE_SOCK = _IO_TYPE_SOCK

IO_FLAG_NO_CLOSE = _IO_FLAG_NO_CLOSE
IO_FLAG_NONBLOCK = _IO_FLAG_NONBLOCK
IO_FLAG_LOOPBACK = _IO_FLAG_LOOPBACK


cdef class IOHandle(object):
    def __dealloc__(self):
        if self._c_handle is not NULL:
            with nogil:
                io_handle_release(self._c_handle)
            self._c_handle = NULL

    def close(self):
        cdef int result
        with nogil:
            result = io_close(self._c_handle)
        self._c_handle = NULL
        if result == -1:
            io_error()

    property type:
        def __get__(self):
            cdef int type
            with nogil:
                type = io_get_type(self._c_handle)
            if type == -1:
                io_error()
            return type

    property flags:
        def __get__(self):
            cdef int flags
            with nogil:
                flags = io_get_flags(self._c_handle)
            if flags == -1:
                io_error()
            return flags

        def __set__(self, int value):
            cdef int result
            with nogil:
                result = io_set_flags(self._c_handle, value)
            if result == -1:
                io_error()

    @cython.boundscheck(False)
    def read(self, uint8_t[::1] buf not None):
        cdef size_t nbytes = len(buf)
        cdef ssize_t result
        with nogil:
            result = io_read(self._c_handle, &buf[0], nbytes)
        if result == -1:
            io_error()
        return result

    @cython.boundscheck(False)
    def write(self, const uint8_t[::1] buf not None):
        cdef size_t nbytes = len(buf)
        cdef ssize_t result
        with nogil:
            result = io_write(self._c_handle, &buf[0], nbytes)
        if result == -1:
            io_error()
        return result

    def flush(self):
        cdef int result
        with nogil:
            result = io_flush(self._c_handle)
        if result == -1:
            io_error()


cdef IOHandle IOHandle_new(io_handle_t handle):
    cdef int type
    if handle is not NULL:
        with nogil:
            type = io_get_type(handle)
        if type == _IO_TYPE_CAN:
            return IOCAN_new(handle)
        if type == _IO_TYPE_FILE:
            return IOFile_new(handle)
        if type == _IO_TYPE_SERIAL:
            return IOSerial_new(handle)
        if type == _IO_TYPE_SOCK:
            return IOSock_new(handle)
        else:
            io_error()

lely_io_init()

