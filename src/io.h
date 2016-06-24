/*!\file
 * This is the internal header file of the I/O library.
 *
 * \copyright 2016 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#ifndef LELY_IO_INTERN_IO_H
#define LELY_IO_INTERN_IO_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define LELY_IO_INTERN	1
#include <lely/io/io.h>

#ifndef LELY_IO_EXPORT
#ifdef DLL_EXPORT
#define LELY_IO_EXPORT	__dllexport
#else
#define LELY_IO_EXPORT
#endif
#endif

#ifdef _WIN32
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#include <ws2bth.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(_POSIX_C_SOURCE)
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
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
#endif

#endif

