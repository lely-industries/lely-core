from libc.stdlib cimport malloc, free

IO_EVENT_SIGNAL = _IO_EVENT_SIGNAL
IO_EVENT_ERROR = _IO_EVENT_ERROR
IO_EVENT_READ = _IO_EVENT_READ
IO_EVENT_WRITE = _IO_EVENT_WRITE


cdef class __IOEvent(object):
    def __dealloc__(self):
        with nogil:
            if self._c_event.events != _IO_EVENT_SIGNAL:
                io_handle_release(self._c_event.handle)

    property events:
        def __get__(self):
            return self._c_event.events

        def __set__(self, int value):
            self._c_event.events = value

    property sig:
        def __get__(self):
            if self._c_event.events != _IO_EVENT_SIGNAL:
                raise AttributeError()
            return self._c_event.sig

        def __set__(self, unsigned char value):
            if self._c_event.events != _IO_EVENT_SIGNAL:
                raise AttributeError()
            self._c_event.sig = value

    property handle:
        def __get__(self):
            if self._c_event.events == _IO_EVENT_SIGNAL:
                raise AttributeError()
            with nogil:
                io_handle_acquire(self._c_event.handle)
            try:
                return IOHandle_new(self._c_event.handle)
            except:
                with nogil:
                    io_handle_release(self._c_event.handle)
                raise

        def __set__(self, IOHandle value not None):
            if self._c_event.events == _IO_EVENT_SIGNAL:
                raise AttributeError()
            with nogil:
                io_handle_acquire(value._c_handle)
                io_handle_release(self._c_event.handle)
                self._c_event.handle = value._c_handle


cdef __IOEvent __IOEvent_new(io_event* event):
    cdef __IOEvent obj = __IOEvent()
    obj._c_event = event
    with nogil:
        if obj._c_event.events != _IO_EVENT_SIGNAL:
            io_handle_acquire(obj._c_event.handle)
    return obj


cdef class IOEvent(__IOEvent):
    def __cinit__(self, __IOEvent event = None):
        self._c_event = &self.__event
        if event is not None:
            self._c_event[0] = event._c_event[0]
            with nogil:
                if self._c_event.events != _IO_EVENT_SIGNAL:
                    io_handle_acquire(self._c_event.handle)


cdef IOEvent IOEvent_new(const io_event* event):
    cdef IOEvent obj = IOEvent()
    obj._c_event[0] = event[0]
    with nogil:
        if obj._c_event.events != _IO_EVENT_SIGNAL:
            io_handle_acquire(obj._c_event.handle)
    return obj

cdef class IOPoll(object):
    def __cinit__(self):
        self._c_poll = io_poll_create()
        if self._c_poll is NULL:
            raise OSError()

    def __dealloc__(self):
        if self._c_poll is not NULL:
            io_poll_destroy(self._c_poll)
            self._c_poll = NULL

    def watch(self, IOHandle handle not None, __IOEvent event, bint keep = 0):
        cdef io_event* _event = NULL
        if event is not None:
            _event = event._c_event
        cdef int result
        with nogil:
            result = io_poll_watch(self._c_poll, handle._c_handle, _event, keep)
        if result == -1:
            io_error()

    def wait(self, int maxevents, int timeout = 0):
        cdef list events = []
        cdef io_event* _events = NULL
        cdef int result
        if maxevents > 0:
            _events = <io_event*>malloc(maxevents * sizeof(io_event))
            if not _events:
                raise MemoryError()
            with nogil:
                result = io_poll_wait(self._c_poll, maxevents, _events, timeout)
            try:
                if result == -1:
                    io_error()
                for i in range(result):
                    events.append(IOEvent_new(&_events[i]))
            except:
                free(_events)
                raise
            free(_events)
        return events

    def signal(self, unsigned char sig = 0):
        cdef int result
        with nogil:
            result = io_poll_signal(self._c_poll, sig)
        if result == -1:
            io_error()

