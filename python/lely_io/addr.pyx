from libc.stdlib cimport malloc, free

from lely_io.sock cimport *


cdef class __IOAddr(object):
    property rfcomm:
        def __get__(self):
            cdef char ba[_IO_ADDR_BTH_STRLEN]
            cdef int port = 0
            cdef int result = io_addr_get_rfcomm_a(self._c_addr, ba, &port)
            if result == -1:
                io_error()
            return (ba, port)

        def __set__(self, value):
            try:
                ba, port = value
                io_addr_set_rfcomm_a(self._c_addr, ba.encode('UTF-8'),
                                     int(port))
            except TypeError:
                io_addr_set_rfcomm_local(self._c_addr, int(value))

    property ipv4:
        def __get__(self):
            cdef char ip[_IO_ADDR_IPV4_STRLEN]
            cdef int port = 0
            cdef int result = io_addr_get_ipv4_a(self._c_addr, ip, &port)
            if result == -1:
                io_error()
            return (ip, port)

        def __set__(self, value):
            try:
                ip, port = value
                io_addr_set_ipv4_a(self._c_addr, ip.encode('UTF-8'), int(port))
            except TypeError:
                io_addr_set_ipv4_loopback(self._c_addr, int(value))

    property ipv6:
        def __get__(self):
            cdef char ip[_IO_ADDR_IPV6_STRLEN]
            cdef int port = 0
            cdef int result = io_addr_get_ipv6_a(self._c_addr, ip, &port)
            if result == -1:
                io_error()
            return (ip, port)

        def __set__(self, value):
            try:
                ip, port = value
                io_addr_set_ipv6_a(self._c_addr, ip.encode('UTF-8'), int(port))
            except TypeError:
                io_addr_set_ipv6_loopback(self._c_addr, int(value))

    property unix:
        def __get__(self):
            cdef char path[_IO_ADDR_UNIX_STRLEN]
            cdef int result = io_addr_get_unix(self._c_addr, path)
            if result == -1:
                io_error()
            return path

        def __set__(self, str value not None):
            io_addr_set_unix(self._c_addr, value.encode('UTF-8'))

    property domain:
        def __get__(self):
            cdef int value = io_addr_get_domain(self._c_addr)
            if value == -1:
                io_error()
            return value

    property port:
        def __get__(self):
            cdef int value
            if io_addr_get_port(self._c_addr, &value) == -1:
                io_error()
            return value

        def __set__(self, int value):
            if io_addr_set_port(self._c_addr, value) == -1:
                io_error()

    property loopback:
        def __get__(self):
            return <bint>io_addr_is_loopback(self._c_addr)

    property broadcast:
        def __get__(self):
            return <bint>io_addr_is_broadcast(self._c_addr)

        def __set__(self, int value):
            io_addr_set_ipv4_broadcast(self._c_addr, value)

    property multicast:
        def __get__(self):
            return <bint>io_addr_is_multicast(self._c_addr)


cdef __IOAddr __IOAddr_new(object owner, io_addr_t* addr):
    cdef __IOAddr obj = __IOAddr()
    obj._owner = owner
    obj._c_addr = addr
    return obj


cdef class IOAddr(__IOAddr):
    def __cinit__(self, __IOAddr addr = None):
        self._owner = None
        self._c_addr = &self.__addr
        if addr is not None:
            self._c_addr[0] = addr._c_addr[0]


cdef IOAddr IOAddr_new(const io_addr_t* addr):
    cdef IOAddr obj = IOAddr()
    obj._c_addr[0] = addr[0]
    return obj


cdef class IOAddrInfo(object):
    property domain:
        def __get__(self):
            return self._c_info.domain

        def __set__(self, int value):
            self._c_info.domain = value

    property type:
        def __get__(self):
            return self._c_info.type

        def __set__(self, int value):
            self._c_info.type = value

    property addr:
        def __get__(self):
            return __IOAddr_new(self, &self._c_info.addr)

        def __set__(self, __IOAddr value not None):
            self._c_info.addr = value._c_addr[0]

    @staticmethod
    def get(int maxinfo, str nodename = None, str servname = None,
            IOAddrInfo hints = None):
        cdef bytes _nodename
        cdef char* __nodename = NULL
        if nodename is not None:
            _nodename = nodename.encode('UTF-8')
            __nodename = _nodename
        cdef bytes _servname
        cdef char* __servname = NULL
        if servname is not None:
            _servname = servname.encode('UTF-8')
            __servname = _servname
        cdef io_addrinfo* _hints = NULL if hints is None else &hints._c_info
        cdef list info = []
        cdef io_addrinfo* _info = NULL
        cdef int result
        if maxinfo > 0:
            _info = <io_addrinfo*>malloc(maxinfo * sizeof(io_addrinfo))
            if not _info:
                raise MemoryError()
            with nogil:
                result = io_get_addrinfo(maxinfo, _info, __nodename, __servname,
                                         _hints)
            try:
                if result == -1:
                    io_error()
                for i in range(result):
                    info.append(IOAddrInfo_new(&_info[i]))
            except:
                free(_info)
                raise
            free(_info)
        return info


cdef IOAddrInfo IOAddrInfo_new(const io_addrinfo* info):
    cdef IOAddrInfo obj = IOAddrInfo()
    obj._c_info = info[0]
    return obj

