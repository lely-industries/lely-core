from lely_io.io cimport *

cdef extern from "lely/io/poll.h":
    enum:
        _IO_EVENT_SIGNAL "IO_EVENT_SIGNAL"
        _IO_EVENT_ERROR "IO_EVENT_ERROR"
        _IO_EVENT_READ "IO_EVENT_READ"
        _IO_EVENT_WRITE "IO_EVENT_WRITE"

    struct io_event:
        int events
        unsigned char sig "u.sig"
        io_handle_t handle "u.handle"

    struct __io_poll:
        pass

    ctypedef __io_poll io_poll_t

    io_poll_t* io_poll_create() nogil
    void io_poll_destroy(io_poll_t* poll) nogil

    int io_poll_watch(io_poll_t* poll, io_handle_t handle, io_event* event,
                      int keep) nogil
    int io_poll_wait(io_poll_t* poll, int maxevents, io_event* events,
                     int timeout) nogil
    int io_poll_signal(io_poll_t* poll, unsigned char sig) nogil


cdef class __IOEvent(object):
    cdef io_event* _c_event


cdef __IOEvent __IOEvent_new(io_event* event)


cdef class IOEvent(__IOEvent):
    cdef io_event __event


cdef IOEvent IOEvent_new(const io_event* event)


cdef class IOPoll(object):
    cdef io_poll_t* _c_poll

