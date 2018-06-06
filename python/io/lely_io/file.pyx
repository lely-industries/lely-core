cimport cython

IO_FILE_READ = _IO_FILE_READ
IO_FILE_WRITE = _IO_FILE_WRITE
IO_FILE_APPEND = _IO_FILE_APPEND
IO_FILE_CREATE = _IO_FILE_CREATE
IO_FILE_NO_EXIST = _IO_FILE_NO_EXIST
IO_FILE_TRUNCATE = _IO_FILE_TRUNCATE

IO_SEEK_BEGIN = _IO_SEEK_BEGIN
IO_SEEK_CURRENT = _IO_SEEK_CURRENT
IO_SEEK_END = _IO_SEEK_END


cdef class IOFile(IOHandle):
    @staticmethod
    def open(str path, int flags):
        cdef bytes _path = path.encode('UTF8')
        cdef char* __path = _path
        cdef io_handle_t handle
        with nogil:
            handle = io_open_file(__path, flags)
        if handle is NULL:
            io_error()
        try:
            return IOFile_new(handle)
        except:
            with nogil:
                io_handle_release(handle)
            raise

    def seek(self, io_off_t offset, int whence):
        cdef io_off_t result
        with nogil:
            result = io_seek(self._c_handle, offset, whence)
        if result == -1:
            io_error()
        return result

    property offset:
        def __get__(self):
            return self.seek(0, _IO_SEEK_CURRENT)

        def __set__(self, io_off_t value):
            self.seek(value, _IO_SEEK_BEGIN)

    @cython.boundscheck(False)
    def pread(self, uint8_t[::1] buf not None, io_off_t offset):
        cdef size_t nbytes = len(buf)
        cdef ssize_t result
        with nogil:
            result = io_pread(self._c_handle, &buf[0], nbytes, offset)
        if result == -1:
            io_error()
        return result

    @cython.boundscheck(False)
    def pwrite(self, const uint8_t[::1] buf not None, io_off_t offset):
        cdef size_t nbytes = len(buf)
        cdef ssize_t result
        with nogil:
            result = io_pwrite(self._c_handle, &buf[0], nbytes, offset)
        if result == -1:
            io_error()
        return result


cdef IOFile IOFile_new(io_handle_t handle):
    cdef IOFile obj = IOFile()
    obj._c_handle = handle
    return obj

