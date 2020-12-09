/**@file
 * This is the internal header file of the I/O library.
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#ifndef LELY_IO_INTERN_IO_H_
#define LELY_IO_INTERN_IO_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lely/io/io.h>

#if _WIN32
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#define FD_SETSIZE 1024
// clang-format off
#include <winsock2.h>
#include <ws2bth.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
// clang-format on
#elif defined(_POSIX_C_SOURCE)
#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <termios.h>
#include <unistd.h>
#endif

#ifdef __linux__

#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
#include <bluetooth/bluetooth.h>
#endif
#ifdef HAVE_BLUETOOTH_RFCOMM_H
#include <bluetooth/rfcomm.h>
#endif

#ifdef HAVE_LINUX_CAN_H
#include <linux/can.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#endif

#if _WIN32

#ifndef MCAST_JOIN_GROUP
#define MCAST_JOIN_GROUP 41
#endif

#ifndef MCAST_LEAVE_GROUP
#define MCAST_LEAVE_GROUP 42
#endif

#ifndef MCAST_BLOCK_SOURCE
#define MCAST_BLOCK_SOURCE 43
#endif

#ifndef MCAST_UNBLOCK_SOURCE
#define MCAST_UNBLOCK_SOURCE 44
#endif

#ifndef MCAST_JOIN_SOURCE_GROUP
#define MCAST_JOIN_SOURCE_GROUP 45
#endif

#ifndef MCAST_LEAVE_SOURCE_GROUP
#define MCAST_LEAVE_SOURCE_GROUP 46
#endif

typedef USHORT sa_family_t;

#else

typedef int HANDLE;
#define INVALID_HANDLE_VALUE (-1)

typedef int SOCKET;
#define INVALID_SOCKET (-1)

#define SOCKET_ERROR (-1)

#define closesocket close

#endif // _WIN32

#endif // !LELY_IO_INTERN_IO_H_
