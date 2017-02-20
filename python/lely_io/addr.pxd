from lely_io.io cimport *

cdef extern from "lely/io/addr.h":
    enum:
        _IO_ADDR_BTH_STRLEN "IO_ADDR_BTH_STRLEN"
        _IO_ADDR_IPV4_STRLEN "IO_ADDR_IPV4_STRLEN"
        _IO_ADDR_IPV6_STRLEN "IO_ADDR_IPV6_STRLEN"
        _IO_ADDR_UNIX_STRLEN "IO_ADDR_UNIX_STRLEN"

    cdef struct io_addrinfo:
        int domain
        int type
        io_addr_t addr

    int io_addr_get_rfcomm_a(const io_addr_t* addr, char* ba, int* port) nogil
    int io_addr_set_rfcomm_a(io_addr_t* addr, const char* ba, int port) nogil
    int io_addr_get_rfcomm_n(const io_addr_t* addr, uint8_t ba[6],
                             int* port) nogil
    void io_addr_set_rfcomm_n(io_addr_t* addr, const uint8_t ba[6],
                              int port) nogil
    void io_addr_set_rfcomm_local(io_addr_t* addr, int port) nogil

    int io_addr_get_ipv4_a(const io_addr_t* addr, char* ip, int* port) nogil
    int io_addr_set_ipv4_a(io_addr_t* addr, const char* ip, int port) nogil
    int io_addr_get_ipv4_n(const io_addr_t* addr, uint8_t ip[4],
                           int* port) nogil
    void io_addr_set_ipv4_n(io_addr_t* addr, const uint8_t ip[4],
                            int port) nogil
    void io_addr_set_ipv4_loopback(io_addr_t* addr, int port) nogil
    void io_addr_set_ipv4_broadcast(io_addr_t* addr, int port) nogil

    int io_addr_get_ipv6_a(const io_addr_t* addr, char* ip, int* port) nogil
    int io_addr_set_ipv6_a(io_addr_t* addr, const char* ip, int port) nogil
    int io_addr_get_ipv6_n(const io_addr_t* addr, uint8_t ip[16],
                           int* port) nogil
    void io_addr_set_ipv6_n(io_addr_t* addr, const uint8_t ip[16],
                            int port) nogil
    void io_addr_set_ipv6_loopback(io_addr_t* addr, int port) nogil

    int io_addr_get_unix(const io_addr_t* addr, char* path) nogil
    void io_addr_set_unix(io_addr_t* addr, const char* path) nogil

    int io_addr_get_domain(const io_addr_t* addr) nogil
    int io_addr_get_port(const io_addr_t* addr, int* port) nogil
    int io_addr_set_port(io_addr_t* addr, int port) nogil

    int io_addr_is_loopback(const io_addr_t* addr) nogil
    int io_addr_is_broadcast(const io_addr_t* addr) nogil
    int io_addr_is_multicast(const io_addr_t* addr) nogil

    int io_get_addrinfo(int maxinfo, io_addrinfo* info, const char* nodename,
                        const char* servname, const io_addrinfo* hints) nogil


cdef class __IOAddr(object):
    cdef object _owner
    cdef io_addr_t* _c_addr


cdef __IOAddr __IOAddr_new(object owner, io_addr_t* addr)


cdef class IOAddr(__IOAddr):
    cdef io_addr_t __addr


cdef IOAddr IOAddr_new(const io_addr_t* addr)


cdef class IOAddrInfo(object):
    cdef io_addrinfo _c_info


cdef IOAddrInfo IOAddrInfo_new(const io_addrinfo* info)

