cimport cython

from lely_io.addr cimport *

IO_SOCK_BTH = _IO_SOCK_BTH
IO_SOCK_IPV4 = _IO_SOCK_IPV4
IO_SOCK_IPV6 = _IO_SOCK_IPV6
IO_SOCK_UNIX = _IO_SOCK_UNIX

IO_SOCK_STREAM = _IO_SOCK_STREAM
IO_SOCK_DGRAM = _IO_SOCK_DGRAM

IO_MSG_PEEK = _IO_MSG_PEEK
IO_MSG_OOB = _IO_MSG_OOB
IO_MSG_WAITALL = _IO_MSG_WAITALL

IO_SHUT_RD = _IO_SHUT_RD
IO_SHUT_WR = _IO_SHUT_WR
IO_SHUT_RDWR = _IO_SHUT_RDWR


cdef class IOSock(IOHandle):
    @staticmethod
    def open(int domain, int type):
        cdef io_handle_t handle
        with nogil:
            handle = io_open_socket(domain, type)
        if handle is NULL:
            io_error()
        try:
            return IOSock_new(handle)
        except:
            with nogil:
                io_handle_release(handle)
            raise

    @staticmethod
    def open_socketpair(int domain, int type):
        cdef io_handle_t handle_vector[2]
        cdef int result
        with nogil:
            result = io_open_socketpair(domain, type, handle_vector)
        if result == -1:
            io_error()
        try:
            return [IOSock_new(handle_vector[0]), IOSock_new(handle_vector[1])]
        except:
            with nogil:
                io_handle_release(handle_vector[1])
                io_handle_release(handle_vector[0])
            raise

    @cython.boundscheck(False)
    def recv(self, uint8_t[::1] buf not None, IOAddr addr = None,
             int flags = 0):
        cdef size_t nbytes = len(buf)
        cdef io_addr_t* _addr = NULL
        if addr is not None:
            _addr = addr._c_addr
        cdef ssize_t result
        with nogil:
            result = io_recv(self._c_handle, &buf[0], nbytes, _addr, flags)
        if result == -1:
            io_error()
        return result

    @cython.boundscheck(False)
    def send(self, const uint8_t[::1] buf not None, IOAddr addr = None,
             int flags = 0):
        cdef size_t nbytes = len(buf)
        cdef io_addr_t* _addr = NULL
        if addr is not None:
            _addr = addr._c_addr
        cdef ssize_t result
        with nogil:
            result = io_send(self._c_handle, &buf[0], nbytes, _addr, flags)
        if result == -1:
            io_error()
        return result

    def accept(self, IOAddr addr = None):
        cdef io_addr_t* _addr = NULL
        if addr is not None:
            _addr = addr._c_addr
        cdef io_handle_t handle
        with nogil:
            handle = io_accept(self._c_handle, _addr)
        if handle is NULL:
            io_error()
        try:
            return IOSock_new(handle)
        except:
            with nogil:
                io_handle_release(handle)
            raise

    def conect(self, IOAddr addr not None):
        cdef int result
        with nogil:
            result = io_connect(self._c_handle, addr._c_addr)
        if result == -1:
            io_error()

    property domain:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_domain(self._c_handle)
            if result == -1:
                io_error()
            return result

    property type:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_type(self._c_handle)
            if result == -1:
                io_error()
            return result

    def bind(self, IOAddr addr not None):
        cdef int result
        with nogil:
            result = io_sock_bind(self._c_handle, addr._c_addr)
        if result == -1:
            io_error()

    def listen(self, int backlog = 0):
        cdef int result
        with nogil:
            result = io_sock_listen(self._c_handle, backlog)
        if result == -1:
            io_error()

    def shutdown(self, int how):
        cdef int result
        with nogil:
            result = io_sock_shutdown(self._c_handle, how)
        if result == -1:
            io_error()

    property sockname:
        def __get__(self):
            cdef IOAddr value = IOAddr()
            cdef int result
            with nogil:
                result = io_sock_get_sockname(self._c_handle, value._c_addr)
            if result == -1:
                io_error()
            return value

    property peername:
        def __get__(self):
            cdef IOAddr value = IOAddr()
            cdef int result
            with nogil:
                result = io_sock_get_peername(self._c_handle, value._c_addr)
            if result == -1:
                io_error()
            return value

    @staticmethod
    def get_maxconn():
        cdef int result
        with nogil:
            result = io_sock_get_maxconn()
        if result == -1:
            io_error()
        return result

    property acceptcon:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_acceptconn(self._c_handle)
            if result == -1:
                io_error()
            return <bint>result

    property broadcast:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_broadcast(self._c_handle)
            if result == -1:
                io_error()
            return <bint>result

        def __set__(self, bint value):
            cdef int result
            with nogil:
                result = io_sock_set_broadcast(self._c_handle, value)
            if result == -1:
                io_error()

    property debug:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_debug(self._c_handle)
            if result == -1:
                io_error()
            return <bint>result

        def __set__(self, bint value):
            cdef int result
            with nogil:
                result = io_sock_set_debug(self._c_handle, value)
            if result == -1:
                io_error()

    property dontroute:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_dontroute(self._c_handle)
            if result == -1:
                io_error()
            return <bint>result

        def __set__(self, bint value):
            cdef int result
            with nogil:
                result = io_sock_set_dontroute(self._c_handle, value)
            if result == -1:
                io_error()

    property error:
        def __get__(self):
            cdef int value = 0
            cdef int result
            with nogil:
                result = io_sock_get_error(self._c_handle, &value)
            if result == -1:
                io_error()
            return value

    property keepalive:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_keepalive(self._c_handle)
            if result == -1:
                io_error()
            return <bint>result

        def __set__(self, value):
            cdef bint keepalive
            cdef int time = 0
            cdef int interval = 0
            try:
                keepalive, time, interval = value
            except TypeError:
                keepalive = value
            cdef int result
            with nogil:
                result = io_sock_set_keepalive(self._c_handle, keepalive, time,
                                               interval)
            if result == -1:
                io_error()

    property linger:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_linger(self._c_handle)
            if result == -1:
                io_error()
            return result

        def __set__(self, int value):
            cdef int result
            with nogil:
                result = io_sock_set_linger(self._c_handle, value)
            if result == -1:
                io_error()

    property oobinline:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_oobinline(self._c_handle)
            if result == -1:
                io_error()
            return <bint>result

        def __set__(self, bint value):
            cdef int result
            with nogil:
                result = io_sock_set_oobinline(self._c_handle, value)
            if result == -1:
                io_error()

    property rcvbuf:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_rcvbuf(self._c_handle)
            if result == -1:
                io_error()
            return result

        def __set__(self, int value):
            cdef int result
            with nogil:
                result = io_sock_set_rcvbuf(self._c_handle, value)
            if result == -1:
                io_error()

    property rcvtimeo:
        def __set__(self, int value):
            cdef int result
            with nogil:
                result = io_sock_set_rcvtimeo(self._c_handle, value)
            if result == -1:
                io_error()

    property reuseaddr:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_reuseaddr(self._c_handle)
            if result == -1:
                io_error()
            return <bint>result

        def __set__(self, bint value):
            cdef int result
            with nogil:
                result = io_sock_set_reuseaddr(self._c_handle, value)
            if result == -1:
                io_error()

    property sndbuf:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_sndbuf(self._c_handle)
            if result == -1:
                io_error()
            return result

        def __set__(self, int value):
            cdef int result
            with nogil:
                result = io_sock_set_sndbuf(self._c_handle, value)
            if result == -1:
                io_error()

    property sndtimeo:
        def __set__(self, int value):
            cdef int result
            with nogil:
                result = io_sock_set_sndtimeo(self._c_handle, value)
            if result == -1:
                io_error()

    property tcp_nodelay:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_tcp_nodelay(self._c_handle)
            if result == -1:
                io_error()
            return <bint>result

        def __set__(self, bint value):
            cdef int result
            with nogil:
                result = io_sock_set_tcp_nodelay(self._c_handle, value)
            if result == -1:
                io_error()

    property nread:
        def __get__(self):
            cdef ssize_t result
            with nogil:
                result = io_sock_get_nread(self._c_handle)
            if result == -1:
                io_error()
            return result

    property mcast_loop:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_mcast_loop(self._c_handle)
            if result == -1:
                io_error()
            return <bint>result

        def __set__(self, bint value):
            cdef int result
            with nogil:
                result = io_sock_set_mcast_loop(self._c_handle, value)
            if result == -1:
                io_error()

    property mcast_ttl:
        def __get__(self):
            cdef int result
            with nogil:
                result = io_sock_get_mcast_ttl(self._c_handle)
            if result == -1:
                io_error()
            return <bint>result

        def __set__(self, bint value):
            cdef int result
            with nogil:
                result = io_sock_set_mcast_ttl(self._c_handle, value)
            if result == -1:
                io_error()

    def mcast_join(self, unsigned int index, IOAddr group not None,
                   IOAddr source = None):
        cdef int result
        if source is None:
            with nogil:
                result = io_sock_mcast_join_group(self._c_handle, index,
                                                  group._c_addr)
        else:
            with nogil:
                result = io_sock_mcast_join_source_group(self._c_handle, index,
                                                         group._c_addr,
                                                         source._c_addr)
        if result == -1:
            io_error()

    def mcast_block(self, unsigned int index, IOAddr group not None,
                    IOAddr source not None):
        cdef int result
        with nogil:
            result = io_sock_mcast_block_source(self._c_handle, index,
                                                group._c_addr, source._c_addr)
        if result == -1:
            io_error()

    def mcast_unblock(self, unsigned int index, IOAddr group not None,
                      IOAddr source not None):
        cdef int result
        with nogil:
            result = io_sock_mcast_unblock_source(self._c_handle, index,
                                                  group._c_addr, source._c_addr)
        if result == -1:
            io_error()

    def mcast_leave(self, unsigned int index, IOAddr group not None,
                    IOAddr source = None):
        cdef int result
        if source is None:
            with nogil:
                result = io_sock_mcast_leave_group(self._c_handle, index,
                                                   group._c_addr)
        else:
            with nogil:
                result = io_sock_mcast_leave_source_group(self._c_handle, index,
                                                          group._c_addr,
                                                          source._c_addr)
        if result == -1:
            io_error()


cdef IOSock IOSock_new(io_handle_t handle):
    cdef IOSock obj = IOSock()
    obj._c_handle = handle
    return obj

