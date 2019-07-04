/**@file
 * This header file is part of the I/O library; it contains the C++ interface of
 * the network socket device handle. @see lely/io/sock.h for the C interface.
 *
 * @copyright 2017-2019 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LELY_IO_SOCK_HPP_
#define LELY_IO_SOCK_HPP_

#ifndef __cplusplus
#error "include <lely/io/sock.h> for the C interface"
#endif

#include <lely/io/io.hpp>
#include <lely/io/sock.h>

#include <utility>

namespace lely {

/// A sock I/O device handle.
class IOSock : public IOHandle {
 public:
  IOSock(int domain, int type) : IOHandle(io_open_socket(domain, type)) {
    if (!operator bool()) throw_or_abort(bad_init());
  }

  IOSock(const IOSock& sock) noexcept : IOHandle(sock) {}

  IOSock(IOSock&& sock) noexcept : IOHandle(::std::forward<IOSock>(sock)) {}

  IOSock&
  operator=(const IOSock& sock) noexcept {
    IOHandle::operator=(sock);
    return *this;
  }

  IOSock&
  operator=(IOSock&& sock) noexcept {
    IOHandle::operator=(::std::forward<IOSock>(sock));
    return *this;
  }

  static int
  open(int domain, int type, IOSock sock[2]) noexcept {
    io_handle_t handle_vector[2];
    if (io_open_socketpair(domain, type, handle_vector) == -1) return -1;
    sock[0] = IOSock(handle_vector[0]);
    sock[1] = IOSock(handle_vector[1]);
    return 0;
  }

  ssize_t
  recv(void* buf, size_t nbytes, io_addr_t* addr = 0, int flags = 0) noexcept {
    return io_recv(*this, buf, nbytes, addr, flags);
  }

  ssize_t
  send(const void* buf, size_t nbytes, const io_addr_t* addr = 0,
       int flags = 0) noexcept {
    return io_send(*this, buf, nbytes, addr, flags);
  }

  IOSock
  accept(io_addr_t* addr = 0) noexcept {
    return IOSock(io_accept(*this, addr));
  }

  int
  connect(const io_addr_t& addr) noexcept {
    return io_connect(*this, &addr);
  }

  int
  get_domain() const noexcept {
    return io_sock_get_domain(*this);
  }
  int
  get_type() const noexcept {
    return io_sock_get_type(*this);
  }

  int
  bind(const io_addr_t& addr) noexcept {
    return io_sock_bind(*this, &addr);
  }

  int
  listen(int backlog = 0) noexcept {
    return io_sock_listen(*this, backlog);
  }

  int
  shutdown(int how) noexcept {
    return io_sock_shutdown(*this, how);
  }

  int
  getSockname(io_addr_t& addr) const noexcept {
    return io_sock_get_sockname(*this, &addr);
  }

  int
  getPeername(io_addr_t& addr) const noexcept {
    return io_sock_get_peername(*this, &addr);
  }

  static int
  getMaxConn() noexcept {
    return io_sock_get_maxconn();
  }

  int
  getAcceptConn() const noexcept {
    return io_sock_get_acceptconn(*this);
  }

  int
  getBroadcast() const noexcept {
    return io_sock_get_broadcast(*this);
  }

  int
  setBroadcast(bool broadcast) noexcept {
    return io_sock_set_broadcast(*this, broadcast);
  }

  int
  getDebug() const noexcept {
    return io_sock_get_debug(*this);
  }

  int
  setDebug(bool debug) noexcept {
    return io_sock_set_debug(*this, debug);
  }

  int
  getDontRoute() const noexcept {
    return io_sock_get_dontroute(*this);
  }

  int
  setDontRoute(bool dontroute) noexcept {
    return io_sock_set_dontroute(*this, dontroute);
  }

  int
  getError(int& error) noexcept {
    return io_sock_get_error(*this, &error);
  }

  int
  getKeepAlive() const noexcept {
    return io_sock_get_keepalive(*this);
  }

  int
  setKeepAlive(bool keepalive, int time = 0, int interval = 0) noexcept {
    return io_sock_set_keepalive(*this, keepalive, time, interval);
  }

  int
  getLinger() const noexcept {
    return io_sock_get_linger(*this);
  }

  int
  setLinger(int time = 0) noexcept {
    return io_sock_set_linger(*this, time);
  }

  int
  getOOBInline() const noexcept {
    return io_sock_get_oobinline(*this);
  }

  int
  setOOBInline(bool oobinline) noexcept {
    return io_sock_set_oobinline(*this, oobinline);
  }

  int
  getRcvBuf() const noexcept {
    return io_sock_get_rcvbuf(*this);
  }

  int
  setRcvBuf(int size) noexcept {
    return io_sock_set_rcvbuf(*this, size);
  }

  int
  setRcvTimeo(int timeout) noexcept {
    return io_sock_set_rcvtimeo(*this, timeout);
  }

  int
  getReuseAddr() const noexcept {
    return io_sock_get_reuseaddr(*this);
  }

  int
  setReuseAddr(bool reuseaddr) noexcept {
    return io_sock_set_reuseaddr(*this, reuseaddr);
  }

  int
  getSndBuf() const noexcept {
    return io_sock_get_sndbuf(*this);
  }

  int
  setSndBuf(int size) noexcept {
    return io_sock_set_sndbuf(*this, size);
  }

  int
  setSndTimeo(int timeout) noexcept {
    return io_sock_set_sndtimeo(*this, timeout);
  }

  int
  getTCPNoDelay() const noexcept {
    return io_sock_get_tcp_nodelay(*this);
  }

  int
  setTCPNoDelay(bool nodelay) noexcept {
    return io_sock_set_tcp_nodelay(*this, nodelay);
  }

  ssize_t
  getNRead() const noexcept {
    return io_sock_get_nread(*this);
  }

  int
  getMcastLoop() const noexcept {
    return io_sock_get_mcast_loop(*this);
  }

  int
  setMcastLoop(bool loop) noexcept {
    return io_sock_set_mcast_loop(*this, loop);
  }

  int
  getMcastTTL() const noexcept {
    return io_sock_get_mcast_ttl(*this);
  }

  int
  setMcastTTL(int ttl) noexcept {
    return io_sock_set_mcast_ttl(*this, ttl);
  }

  int
  mcastJoinGroup(unsigned int index, const io_addr_t& group) noexcept {
    return io_sock_mcast_join_group(*this, index, &group);
  }

  int
  mcastBlockSource(unsigned int index, const io_addr_t& group,
                   const io_addr_t& source) noexcept {
    return io_sock_mcast_block_source(*this, index, &group, &source);
  }

  int
  mcastUnblockSource(unsigned int index, const io_addr_t& group,
                     const io_addr_t& source) noexcept {
    return io_sock_mcast_unblock_source(*this, index, &group, &source);
  }

  int
  mcastLeaveGroup(unsigned int index, const io_addr_t& group) noexcept {
    return io_sock_mcast_leave_group(*this, index, &group);
  }

  int
  mcastJoinSourceGroup(unsigned int index, const io_addr_t& group,
                       const io_addr_t& source) noexcept {
    return io_sock_mcast_join_source_group(*this, index, &group, &source);
  }

  int
  mcastLeaveSourceGroup(unsigned int index, const io_addr_t& group,
                        const io_addr_t& source) noexcept {
    return io_sock_mcast_leave_source_group(*this, index, &group, &source);
  }

 protected:
  IOSock(io_handle_t handle) noexcept : IOHandle(handle) {}
};

}  // namespace lely

#endif  // !LELY_IO_SOCK_HPP_
