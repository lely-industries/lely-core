from lely_io.io cimport *

cdef extern from "lely/io/file.h":
    enum:
        _IO_FILE_READ "IO_FILE_READ"
        _IO_FILE_WRITE "IO_FILE_WRITE"
        _IO_FILE_APPEND "IO_FILE_APPEND"
        _IO_FILE_CREATE "IO_FILE_CREATE"
        _IO_FILE_NO_EXIST "IO_FILE_NO_EXIST"
        _IO_FILE_TRUNCATE "IO_FILE_TRUNCATE"

    enum:
        _IO_SEEK_BEGIN "IO_SEEK_BEGIN"
        _IO_SEEK_CURRENT "IO_SEEK_CURRENT"
        _IO_SEEK_END "IO_SEEK_END"

    io_handle_t io_open_file(const char* path, int flags) nogil

    io_off_t io_seek(io_handle_t handle, io_off_t offset, int whence) nogil

    ssize_t io_pread(io_handle_t handle, void* buf, size_t nbytes,
                     io_off_t offset) nogil
    ssize_t io_pwrite(io_handle_t handle, const void* buf, size_t nbytes,
                      io_off_t offset) nogil


cdef class IOFile(IOHandle):
    pass


cdef IOFile IOFile_new(io_handle_t handle)

