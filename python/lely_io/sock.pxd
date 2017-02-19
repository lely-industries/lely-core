from lely_io.io cimport *

cdef extern from "lely/io/sock.h":
    enum:
        _IO_SOCK_BTH "IO_SOCK_BTH"
        _IO_SOCK_IPV4 "IO_SOCK_IPV4"
        _IO_SOCK_IPV6 "IO_SOCK_IPV6"
        _IO_SOCK_UNIX "IO_SOCK_UNIX"

    enum:
        _IO_SOCK_STREAM "IO_SOCK_STREAM"
        _IO_SOCK_DGRAM "IO_SOCK_DGRAM"

    enum:
        _IO_MSG_PEEK "IO_MSG_PEEK"
        _IO_MSG_OOB "IO_MSG_OOB"
        _IO_MSG_WAITALL "IO_MSG_WAITALL"

    enum:
        _IO_SHUT_RD "IO_SHUT_RD"
        _IO_SHUT_WR "IO_SHUT_WR"
        _IO_SHUT_RDWR "IO_SHUT_RDWR"

    io_handle_t io_open_socket(int domain, int type) nogil
    int io_open_socketpair(int domain, int type,
                            io_handle_t handle_vector[2]) nogil

    ssize_t io_recv(io_handle_t handle, void* buf, size_t nbytes,
                    io_addr_t* addr, int flags) nogil
    ssize_t io_send(io_handle_t handle, const void* buf, size_t nbytes,
                    const io_addr_t* addr, int flags) nogil

    io_handle_t io_accept(io_handle_t handle, io_addr_t* addr) nogil
    int io_connect(io_handle_t handle, const io_addr_t* addr) nogil

    int io_sock_get_domain(io_handle_t handle) nogil
    int io_sock_get_type(io_handle_t handle) nogil

    int io_sock_bind(io_handle_t handle, const io_addr_t* addr) nogil
    int io_sock_listen(io_handle_t handle, int backlog) nogil

    int io_sock_shutdown(io_handle_t handle, int how) nogil

    int io_sock_get_sockname(io_handle_t handle, io_addr_t* addr) nogil
    int io_sock_get_peername(io_handle_t handle, io_addr_t* addr) nogil

    int io_sock_get_maxconn() nogil

    int io_sock_get_acceptconn(io_handle_t handle) nogil

    int io_sock_get_broadcast(io_handle_t handle) nogil
    int io_sock_set_broadcast(io_handle_t handle, int broadcast) nogil

    int io_sock_get_debug(io_handle_t handle) nogil
    int io_sock_set_debug(io_handle_t handle, int debug) nogil

    int io_sock_get_dontroute(io_handle_t handle) nogil
    int io_sock_set_dontroute(io_handle_t handle, int dontroute) nogil

    int io_sock_get_error(io_handle_t handle, int* perror) nogil

    int io_sock_get_keepalive(io_handle_t handle) nogil
    int io_sock_set_keepalive(io_handle_t handle, int keepalive, int time,
                              int interval) nogil

    int io_sock_get_linger(io_handle_t handle) nogil
    int io_sock_set_linger(io_handle_t handle, int time) nogil

    int io_sock_get_oobinline(io_handle_t handle) nogil
    int io_sock_set_oobinline(io_handle_t handle, int oobinline) nogil

    int io_sock_get_rcvbuf(io_handle_t handle) nogil
    int io_sock_set_rcvbuf(io_handle_t handle, int size) nogil

    int io_sock_set_rcvtimeo(io_handle_t handle, int timeout) nogil

    int io_sock_get_reuseaddr(io_handle_t handle) nogil
    int io_sock_set_reuseaddr(io_handle_t handle, int reuseaddr) nogil

    int io_sock_get_sndbuf(io_handle_t handle) nogil
    int io_sock_set_sndbuf(io_handle_t handle, int size) nogil

    int io_sock_set_sndtimeo(io_handle_t handle, int timeout) nogil

    int io_sock_get_tcp_nodelay(io_handle_t handle) nogil
    int io_sock_set_tcp_nodelay(io_handle_t handle, int nodelay) nogil

    ssize_t io_sock_get_nread(io_handle_t handle) nogil

    int io_sock_get_mcast_loop(io_handle_t handle) nogil
    int io_sock_set_mcast_loop(io_handle_t handle, int loop) nogil

    int io_sock_get_mcast_ttl(io_handle_t handle) nogil
    int io_sock_set_mcast_ttl(io_handle_t handle, int ttl) nogil

    int io_sock_mcast_join_group(io_handle_t handle, unsigned int index,
                                 const io_addr_t* group) nogil

    int io_sock_mcast_block_source(io_handle_t handle, unsigned int index,
                                   const io_addr_t* group,
                                   const io_addr_t* source) nogil
    int io_sock_mcast_unblock_source(io_handle_t handle, unsigned int index,
                                     const io_addr_t* group,
                                     const io_addr_t* source) nogil

    int io_sock_mcast_leave_group(io_handle_t handle, unsigned int index,
                                  const io_addr_t* group) nogil

    int io_sock_mcast_join_source_group(io_handle_t handle, unsigned int index,
                                        const io_addr_t* group,
                                        const io_addr_t* source) nogil
    int io_sock_mcast_leave_source_group(io_handle_t handle, unsigned int index,
                                         const io_addr_t* group,
                                         const io_addr_t* source) nogil


cdef class IOSock(IOHandle):
    pass


cdef IOSock IOSock_new(io_handle_t handle)

