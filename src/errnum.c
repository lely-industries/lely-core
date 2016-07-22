/*!\file
 * This file is part of the utilities library; it contains the implementation of
 * the error functions.
 *
 * \see lely/util/errnum.h
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

#include "util.h"
#include <lely/util/errnum.h>

#include <string.h>

#ifdef _WIN32
#ifndef ERRSTR_SIZE
#define ERRSTR_SIZE	256
#endif
static _Thread_local char errstr[ERRSTR_SIZE];
#endif

LELY_UTIL_EXTERN errc_t
errno2c(int errnum)
{
#ifdef _WIN32
	return errnum2c(errno2num(errnum));
#else
	return errnum;
#endif
}

LELY_UTIL_EXTERN errnum_t
errno2num(int errnum)
{
	switch (errnum) {
#ifdef E2BIG
	case E2BIG: return ERRNUM_2BIG;
#endif
#ifdef EACCES
	case EACCES: return ERRNUM_ACCES;
#endif
#ifdef EADDRINUSE
	case EADDRINUSE: return ERRNUM_ADDRINUSE;
#endif
#ifdef EADDRNOTAVAIL
	case EADDRNOTAVAIL: return ERRNUM_ADDRNOTAVAIL;
#endif
#ifdef EAFNOSUPPORT
	case EAFNOSUPPORT: return ERRNUM_AFNOSUPPORT;
#endif
#ifdef EAGAIN
	case EAGAIN: return ERRNUM_AGAIN;
#endif
#ifdef EALREADY
	case EALREADY: return ERRNUM_ALREADY;
#endif
#ifdef EBADF
	case EBADF: return ERRNUM_BADF;
#endif
#ifdef EBADMSG
	case EBADMSG: return ERRNUM_BADMSG;
#endif
#ifdef EBUSY
	case EBUSY: return ERRNUM_BUSY;
#endif
#ifdef ECANCELED
	case ECANCELED: return ERRNUM_CANCELED;
#endif
#ifdef ECHILD
	case ECHILD: return ERRNUM_CHILD;
#endif
#ifdef ECONNABORTED
	case ECONNABORTED: return ERRNUM_CONNABORTED;
#endif
#ifdef ECONNREFUSED
	case ECONNREFUSED: return ERRNUM_CONNREFUSED;
#endif
#ifdef ECONNRESET
	case ECONNRESET: return ERRNUM_CONNRESET;
#endif
#ifdef EDEADLK
	case EDEADLK: return ERRNUM_DEADLK;
#endif
#ifdef EDESTADDRREQ
	case EDESTADDRREQ: return ERRNUM_DESTADDRREQ;
#endif
	case EDOM: return ERRNUM_DOM;
//	case EDQUOT: return ERRNUM_DQUOT;
#ifdef EEXIST
	case EEXIST: return ERRNUM_EXIST;
#endif
#ifdef EFAULT
	case EFAULT: return ERRNUM_FAULT;
#endif
#ifdef EFBIG
	case EFBIG: return ERRNUM_FBIG;
#endif
#ifdef EHOSTUNREACH
	case EHOSTUNREACH: return ERRNUM_HOSTUNREACH;
#endif
#ifdef EIDRM
	case EIDRM: return ERRNUM_IDRM;
#endif
	case EILSEQ: return ERRNUM_ILSEQ;
#ifdef EINPROGRESS
	case EINPROGRESS: return ERRNUM_INPROGRESS;
#endif
#ifdef EINTR
	case EINTR: return ERRNUM_INTR;
#endif
#ifdef EINVAL
	case EINVAL: return ERRNUM_INVAL;
#endif
#ifdef EIO
	case EIO: return ERRNUM_IO;
#endif
#ifdef EISCONN
	case EISCONN: return ERRNUM_ISCONN;
#endif
#ifdef EISDIR
	case EISDIR: return ERRNUM_ISDIR;
#endif
#ifdef ELOOP
	case ELOOP: return ERRNUM_LOOP;
#endif
#ifdef EMFILE
	case EMFILE: return ERRNUM_MFILE;
#endif
#ifdef EMLINK
	case EMLINK: return ERRNUM_MLINK;
#endif
#ifdef EMSGSIZE
	case EMSGSIZE: return ERRNUM_MSGSIZE;
#endif
//	case EMULTIHOP: return ERRNUM_MULTIHOP;
#ifdef ENAMETOOLONG
	case ENAMETOOLONG: return ERRNUM_NAMETOOLONG;
#endif
#ifdef ENETDOWN
	case ENETDOWN: return ERRNUM_NETDOWN;
#endif
#ifdef ENETRESET
	case ENETRESET: return ERRNUM_NETRESET;
#endif
#ifdef ENETUNREACH
	case ENETUNREACH: return ERRNUM_NETUNREACH;
#endif
#ifdef ENFILE
	case ENFILE: return ERRNUM_NFILE;
#endif
#ifdef ENOBUFS
	case ENOBUFS: return ERRNUM_NOBUFS;
#endif
#ifdef ENODATA
	case ENODATA: return ERRNUM_NODATA;
#endif
#ifdef ENODEV
	case ENODEV: return ERRNUM_NODEV;
#endif
#ifdef ENOENT
	case ENOENT: return ERRNUM_NOENT;
#endif
#ifdef ENOEXEC
	case ENOEXEC: return ERRNUM_NOEXEC;
#endif
#ifdef ENOLCK
	case ENOLCK: return ERRNUM_NOLCK;
#endif
//	case ENOLINK: return ERRNUM_NOLINK;
#ifdef ENOMEM
	case ENOMEM: return ERRNUM_NOMEM;
#endif
#ifdef ENOMSG
	case ENOMSG: return ERRNUM_NOMSG;
#endif
#ifdef ENOPROTOOPT
	case ENOPROTOOPT: return ERRNUM_NOPROTOOPT;
#endif
#ifdef ENOSPC
	case ENOSPC: return ERRNUM_NOSPC;
#endif
#ifdef ENOSR
	case ENOSR: return ERRNUM_NOSR;
#endif
#ifdef ENOSTR
	case ENOSTR: return ERRNUM_NOSTR;
#endif
#ifdef ENOSYS
	case ENOSYS: return ERRNUM_NOSYS;
#endif
#ifdef ENOTCONN
	case ENOTCONN: return ERRNUM_NOTCONN;
#endif
#ifdef ENOTDIR
	case ENOTDIR: return ERRNUM_NOTDIR;
#endif
#ifdef ENOTEMPTY
	case ENOTEMPTY: return ERRNUM_NOTEMPTY;
#endif
#ifdef ENOTRECOVERABLE
	case ENOTRECOVERABLE: return ERRNUM_NOTRECOVERABLE;
#endif
#ifdef ENOTSOCK
	case ENOTSOCK: return ERRNUM_NOTSOCK;
#endif
#ifdef ENOTSUP
	case ENOTSUP: return ERRNUM_NOTSUP;
#endif
#ifdef ENOTTY
	case ENOTTY: return ERRNUM_NOTTY;
#endif
#ifdef ENXIO
	case ENXIO: return ERRNUM_NXIO;
#endif
#if defined(EOPNOTSUPP) && EOPNOTSUPP != ENOTSUP
	case EOPNOTSUPP: return ERRNUM_OPNOTSUPP;
#endif
#ifdef EOVERFLOW
	case EOVERFLOW: return ERRNUM_OVERFLOW;
#endif
#ifdef EOWNERDEAD
	case EOWNERDEAD: return ERRNUM_OWNERDEAD;
#endif
#ifdef EPERM
	case EPERM: return ERRNUM_PERM;
#endif
#ifdef EPIPE
	case EPIPE: return ERRNUM_PIPE;
#endif
#ifdef EPROTO
	case EPROTO: return ERRNUM_PROTO;
#endif
#ifdef EPROTONOSUPPORT
	case EPROTONOSUPPORT: return ERRNUM_PROTONOSUPPORT;
#endif
#ifdef EPROTOTYPE
	case EPROTOTYPE: return ERRNUM_PROTOTYPE;
#endif
	case ERANGE: return ERRNUM_RANGE;
#ifdef EROFS
	case EROFS: return ERRNUM_ROFS;
#endif
#ifdef ESPIPE
	case ESPIPE: return ERRNUM_SPIPE;
#endif
#ifdef ESRCH
	case ESRCH: return ERRNUM_SRCH;
#endif
//	case ESTALE: return ERRNUM_STALE;
#ifdef ETIME
	case ETIME: return ERRNUM_TIME;
#endif
#ifdef ETIMEDOUT
	case ETIMEDOUT: return ERRNUM_TIMEDOUT;
#endif
#ifdef ETXTBSY
	case ETXTBSY: return ERRNUM_TXTBSY;
#endif
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
	case EWOULDBLOCK: return ERRNUM_WOULDBLOCK;
#endif
#ifdef EXDEV
	case EXDEV: return ERRNUM_XDEV;
#endif
	default: return 0;
	}
}

LELY_UTIL_EXTERN int
errc2no(errc_t errc)
{
#ifdef _WIN32
	return errnum2no(errc2num(errc));
#else
	return errc > 0 ? errc : 0;
#endif
}

LELY_UTIL_EXPORT errnum_t
errc2num(errc_t errc)
{
#ifdef _WIN32
	switch (errc) {
	case ERROR_ACCESS_DENIED: return ERRNUM_ACCES;
	case ERROR_ACTIVE_CONNECTIONS: return ERRNUM_AGAIN;
	case ERROR_ALREADY_EXISTS: return ERRNUM_EXIST;
	case ERROR_ARENA_TRASHED: return ERRNUM_NOMEM;
	case ERROR_ARITHMETIC_OVERFLOW: return ERRNUM_RANGE;
	case ERROR_BAD_DEVICE: return ERRNUM_NODEV;
	case ERROR_BAD_ENVIRONMENT: return ERRNUM_2BIG;
	case ERROR_BAD_EXE_FORMAT: return ERRNUM_NOEXEC;
	case ERROR_BAD_FORMAT: return ERRNUM_NOEXEC;
	case ERROR_BAD_NET_NAME: return ERRNUM_NOENT;
	case ERROR_BAD_NETPATH: return ERRNUM_NOENT;
	case ERROR_BAD_NET_RESP: return ERRNUM_NOSYS;
	case ERROR_BAD_PATHNAME: return ERRNUM_NOENT;
	case ERROR_BAD_PIPE: return ERRNUM_INVAL;
	case ERROR_BAD_UNIT: return ERRNUM_NODEV;
	case ERROR_BAD_USERNAME: return ERRNUM_INVAL;
	case ERROR_BEGINNING_OF_MEDIA: return ERRNUM_IO;
	case ERROR_BROKEN_PIPE: return ERRNUM_PIPE;
	case ERROR_BUFFER_OVERFLOW: return ERRNUM_NAMETOOLONG;
	case ERROR_BUS_RESET: return ERRNUM_IO;
	case ERROR_BUSY: return ERRNUM_BUSY;
	case ERROR_BUSY_DRIVE: return ERRNUM_BUSY;
	case ERROR_CALL_NOT_IMPLEMENTED: return ERRNUM_NOSYS;
	case ERROR_CANCELLED: return ERRNUM_INTR;
	case ERROR_CANNOT_MAKE: return ERRNUM_ACCES;
//	case ERROR_CANNOT_MAKE: return ERRNUM_PERM; // Cygwin
	case ERROR_CANTOPEN: return ERRNUM_IO;
	case ERROR_CANTREAD: return ERRNUM_IO;
	case ERROR_CANTWRITE: return ERRNUM_IO;
//	case ERROR_CHILD_NOT_COMPLETE: return ERRNUM_BUSY; // Cygwin
	case ERROR_CHILD_NOT_COMPLETE: return ERRNUM_CHILD;
	case ERROR_COMMITMENT_LIMIT: return ERRNUM_AGAIN;
	case ERROR_CONNECTION_REFUSED: return ERRNUM_CONNREFUSED;
	case ERROR_CRC: return ERRNUM_IO;
	case ERROR_CURRENT_DIRECTORY: return ERRNUM_ACCES;
	case ERROR_DEVICE_DOOR_OPEN: return ERRNUM_IO;
	case ERROR_DEVICE_IN_USE: return ERRNUM_BUSY;
//	case ERROR_DEVICE_IN_USE: return ERRNUM_AGAIN; // Cygwin
	case ERROR_DEVICE_REQUIRES_CLEANING: return ERRNUM_IO;
	case ERROR_DEV_NOT_EXIST: return ERRNUM_NODEV;
//	case ERROR_DEV_NOT_EXIST: return ERRNUM_NOENT; // Cywgin
	case ERROR_DIRECTORY: return ERRNUM_NOTDIR;
//	case ERROR_DIRECTORY: return ERRNUM_INVAL; // Cygwin
	case ERROR_DIR_NOT_EMPTY: return ERRNUM_NOTEMPTY;
	case ERROR_DISK_CORRUPT: return ERRNUM_IO;
	case ERROR_DISK_FULL: return ERRNUM_NOSPC;
	case ERROR_DRIVE_LOCKED: return ERRNUM_ACCES;
	case ERROR_DS_GENERIC_ERROR: return ERRNUM_IO;
	case ERROR_EA_LIST_INCONSISTENT: return ERRNUM_INVAL;
	case ERROR_EAS_DIDNT_FIT: return ERRNUM_NOSPC;
	case ERROR_EAS_NOT_SUPPORTED: return ERRNUM_NOTSUP;
	case ERROR_EA_TABLE_FULL: return ERRNUM_NOSPC;
	case ERROR_END_OF_MEDIA: return ERRNUM_NOSPC;
	case ERROR_EOM_OVERFLOW: return ERRNUM_IO;
	case ERROR_EXE_MACHINE_TYPE_MISMATCH: return ERRNUM_NOEXEC;
	case ERROR_EXE_MARKED_INVALID: return ERRNUM_NOEXEC;
	case ERROR_FAIL_I24: return ERRNUM_ACCES;
	case ERROR_FILE_CORRUPT: return ERRNUM_EXIST;
	case ERROR_FILE_EXISTS: return ERRNUM_EXIST;
	case ERROR_FILE_INVALID: return ERRNUM_NXIO;
	case ERROR_FILEMARK_DETECTED: return ERRNUM_IO;
	case ERROR_FILENAME_EXCED_RANGE: return ERRNUM_NAMETOOLONG;
//	case ERROR_FILENAME_EXCED_RANGE: return ERRNUM_NOENT; // Wine
	case ERROR_FILE_NOT_FOUND: return ERRNUM_NOENT;
#ifdef ERROR_FLOAT_MULTIPLE_FAULTS
	case ERROR_FLOAT_MULTIPLE_FAULTS: return ERRNUM_DOM;
#endif
	case ERROR_HANDLE_DISK_FULL: return ERRNUM_NOSPC;
	case ERROR_HANDLE_EOF: return ERRNUM_NODATA;
#ifdef ERROR_ILLEGAL_CHARACTER
	case ERROR_ILLEGAL_CHARACTER: return ERRNUM_ILSEQ;
#endif
	case ERROR_INVALID_ACCESS: return ERRNUM_ACCES;
	case ERROR_INVALID_ADDRESS: return ERRNUM_INVAL;
	case ERROR_INVALID_AT_INTERRUPT_TIME: return ERRNUM_INTR;
	case ERROR_INVALID_BLOCK: return ERRNUM_NOMEM;
	case ERROR_INVALID_BLOCK_LENGTH: return ERRNUM_IO;
	case ERROR_INVALID_CATEGORY: return ERRNUM_NOTTY;
	case ERROR_INVALID_DATA: return ERRNUM_INVAL;
	case ERROR_INVALID_DRIVE: return ERRNUM_NODEV;
//	case ERROR_INVALID_DRIVE: return ERRNUM_NOENT; // Wine
	case ERROR_INVALID_EA_NAME: return ERRNUM_INVAL;
	case ERROR_INVALID_EXE_SIGNATURE: return ERRNUM_NOEXEC;
	case ERROR_INVALID_FUNCTION: return ERRNUM_NOSYS;
//	case ERROR_INVALID_HANDLE: return ERRNUM_INVAL; // Boost
	case ERROR_INVALID_HANDLE: return ERRNUM_BADF;
	case ERROR_INVALID_NAME: return ERRNUM_INVAL;
//	case ERROR_INVALID_NAME: return ERRNUM_NOENT; // Cygwin
	case ERROR_INVALID_PARAMETER: return ERRNUM_INVAL;
	case ERROR_INVALID_SIGNAL_NUMBER: return ERRNUM_INVAL;
	case ERROR_IOPL_NOT_ENABLED: return ERRNUM_NOEXEC;
	case ERROR_IO_DEVICE: return ERRNUM_IO;
	case ERROR_IO_INCOMPLETE: return ERRNUM_AGAIN;
	case ERROR_IO_PENDING: return ERRNUM_AGAIN;
	case ERROR_LOCK_FAILED: return ERRNUM_ACCES;
	case ERROR_LOCK_VIOLATION: return ERRNUM_NOLCK;
//	case ERROR_LOCK_VIOLATION: return ERRNUM_BUSY; // Cygwin
//	case ERROR_LOCK_VIOLATION: return ERRNUM_ACCES; // Wine
	case ERROR_LOCKED: return ERRNUM_NOLCK;
	case ERROR_MAX_THRDS_REACHED: return ERRNUM_AGAIN;
	case ERROR_META_EXPANSION_TOO_LONG: return ERRNUM_INVAL;
	case ERROR_MOD_NOT_FOUND: return ERRNUM_NOENT;
	case ERROR_MORE_DATA: return ERRNUM_MSGSIZE;
	case ERROR_NEGATIVE_SEEK: return ERRNUM_INVAL;
	case ERROR_NESTING_NOT_ALLOWED: return ERRNUM_AGAIN;
	case ERROR_NETNAME_DELETED: return ERRNUM_NOENT;
	case ERROR_NETWORK_ACCESS_DENIED: return ERRNUM_ACCES;
//	case ERROR_NOACCESS: return ERRNUM_ACCES; // Boost
	case ERROR_NOACCESS: return ERRNUM_FAULT;
	case ERROR_NONE_MAPPED: return ERRNUM_INVAL;
	case ERROR_NONPAGED_SYSTEM_RESOURCES: return ERRNUM_AGAIN;
//	case ERROR_NOT_CONNECTED: return ERRNUM_NOLINK;
	case ERROR_NOT_ENOUGH_MEMORY: return ERRNUM_NOMEM;
	case ERROR_NOT_ENOUGH_QUOTA: return ERRNUM_IO;
//	case ERROR_NOT_ENOUGH_QUOTA: return ERRNUM_NOMEM; // Wine
	case ERROR_NOT_LOCKED: return ERRNUM_ACCES;
	case ERROR_NOT_OWNER: return ERRNUM_PERM;
	case ERROR_NOT_READY: return ERRNUM_AGAIN;
	case ERROR_NOT_SAME_DEVICE: return ERRNUM_XDEV;
	case ERROR_NOT_SUPPORTED: return ERRNUM_NOSYS;
	case ERROR_NO_DATA_DETECTED: return ERRNUM_IO;
	case ERROR_NO_DATA: return ERRNUM_PIPE;
	case ERROR_NO_MORE_FILES: return ERRNUM_NOENT;
	case ERROR_NO_MORE_SEARCH_HANDLES: return ERRNUM_NFILE;
	case ERROR_NO_PROC_SLOTS: return ERRNUM_AGAIN;
	case ERROR_NO_SIGNAL_SENT: return ERRNUM_IO;
	case ERROR_NO_SYSTEM_RESOURCES: return ERRNUM_FBIG;
	case ERROR_NO_TOKEN: return ERRNUM_INVAL;
	case ERROR_OPEN_FAILED: return ERRNUM_IO;
	case ERROR_OPEN_FILES: return ERRNUM_AGAIN;
//	case ERROR_OPEN_FILES: return ERRNUM_BUSY; // Cygwin
	case ERROR_OPERATION_ABORTED: return ERRNUM_CANCELED;
	case ERROR_OUTOFMEMORY: return ERRNUM_NOMEM;
	case ERROR_PAGED_SYSTEM_RESOURCES: return ERRNUM_AGAIN;
	case ERROR_PAGEFILE_QUOTA: return ERRNUM_AGAIN;
	case ERROR_PATH_NOT_FOUND: return ERRNUM_NOENT;
	case ERROR_PIPE_BUSY: return ERRNUM_BUSY;
	case ERROR_PIPE_CONNECTED: return ERRNUM_BUSY;
	case ERROR_POSSIBLE_DEADLOCK: return ERRNUM_DEADLK;
	case ERROR_PRIVILEGE_NOT_HELD: return ERRNUM_PERM;
	case ERROR_PROCESS_ABORTED: return ERRNUM_FAULT;
	case ERROR_PROC_NOT_FOUND: return ERRNUM_SRCH;
	case ERROR_READ_FAULT: return ERRNUM_IO;
	case ERROR_RETRY: return ERRNUM_AGAIN;
	case ERROR_SECTOR_NOT_FOUND: return ERRNUM_INVAL;
	case ERROR_SEEK: return ERRNUM_IO;
//	case ERROR_SEEK: return ERRNUM_INVAL; // Cygwin
	case ERROR_SEEK_ON_DEVICE: return ERRNUM_ACCES;
	case ERROR_SERVICE_REQUEST_TIMEOUT: return ERRNUM_BUSY;
	case ERROR_SETMARK_DETECTED: return ERRNUM_IO;
	case ERROR_SHARING_BUFFER_EXCEEDED: return ERRNUM_NOLCK;
	case ERROR_SHARING_VIOLATION: return ERRNUM_ACCES;
//	case ERROR_SHARING_VIOLATION: return ERRNUM_BUSY; // Cygwin
	case ERROR_SIGNAL_PENDING: return ERRNUM_BUSY;
	case ERROR_SIGNAL_REFUSED: return ERRNUM_IO;
	case ERROR_THREAD_1_INACTIVE: return ERRNUM_INVAL;
//	case ERROR_TIMEOUT: return ERRNUM_BUSY; // Cygwin
	case ERROR_TIMEOUT: return ERRNUM_TIMEDOUT;
	case ERROR_TOO_MANY_LINKS: return ERRNUM_MLINK;
	case ERROR_TOO_MANY_OPEN_FILES: return ERRNUM_MFILE;
	case ERROR_UNEXP_NET_ERR: return ERRNUM_IO;
	case ERROR_WAIT_NO_CHILDREN: return ERRNUM_CHILD;
	case ERROR_WORKING_SET_QUOTA: return ERRNUM_AGAIN;
	case ERROR_WRITE_FAULT: return ERRNUM_IO;
//	case ERROR_WRITE_PROTECT: return ERRNUM_ACCES; // Boost
	case ERROR_WRITE_PROTECT: return ERRNUM_ROFS;
	case WSAEACCES: return ERRNUM_ACCES;
	case WSAEADDRINUSE: return ERRNUM_ADDRINUSE;
	case WSAEADDRNOTAVAIL: return ERRNUM_ADDRNOTAVAIL;
	case WSAEAFNOSUPPORT: return ERRNUM_AFNOSUPPORT;
	case WSAEALREADY: return ERRNUM_ALREADY;
	case WSAEBADF: return ERRNUM_BADF;
	case WSAECONNABORTED: return ERRNUM_CONNABORTED;
	case WSAECONNREFUSED: return ERRNUM_CONNREFUSED;
	case WSAECONNRESET: return ERRNUM_CONNRESET;
	case WSAEDESTADDRREQ: return ERRNUM_DESTADDRREQ;
	case WSAEFAULT: return ERRNUM_FAULT;
	case WSAEHOSTUNREACH: return ERRNUM_HOSTUNREACH;
	case WSAEINPROGRESS: return ERRNUM_INPROGRESS;
	case WSAEINTR: return ERRNUM_INTR;
	case WSAEINVAL: return ERRNUM_INVAL;
	case WSAEISCONN: return ERRNUM_ISCONN;
	case WSAEMFILE: return ERRNUM_MFILE;
	case WSAEMSGSIZE: return ERRNUM_MSGSIZE;
	case WSAENAMETOOLONG: return ERRNUM_NAMETOOLONG;
	case WSAENETDOWN: return ERRNUM_NETDOWN;
	case WSAENETRESET: return ERRNUM_NETRESET;
	case WSAENETUNREACH: return ERRNUM_NETUNREACH;
	case WSAENOBUFS: return ERRNUM_NOBUFS;
	case WSAENOPROTOOPT: return ERRNUM_NOPROTOOPT;
	case WSAENOTCONN: return ERRNUM_NOTCONN;
	case WSAENOTSOCK: return ERRNUM_NOTSOCK;
	case WSAEOPNOTSUPP: return ERRNUM_OPNOTSUPP;
	case WSAEPROTONOSUPPORT: return ERRNUM_PROTONOSUPPORT;
	case WSAEPROTOTYPE: return ERRNUM_PROTOTYPE;
	case WSAESOCKTNOSUPPORT: return ERRNUM_AI_SOCKTYPE;
	case WSAETIMEDOUT: return ERRNUM_TIMEDOUT;
	case WSAEWOULDBLOCK: return ERRNUM_WOULDBLOCK;
	case WSAHOST_NOT_FOUND: return ERRNUM_AI_NONAME;
	case WSANO_RECOVERY: return ERRNUM_AI_FAIL;
	case WSATRY_AGAIN: return ERRNUM_AI_AGAIN;
	case WSATYPE_NOT_FOUND: return ERRNUM_AI_SERVICE;
	default: return 0;
	}
#else
#if _POSIX_C_SOURCE >= 200112L
	switch (errc) {
	case -ABS(EAI_AGAIN): return ERRNUM_AI_AGAIN;
	case -ABS(EAI_BADFLAGS): return ERRNUM_AI_BADFLAGS;
	case -ABS(EAI_FAIL): return ERRNUM_AI_FAIL;
	case -ABS(EAI_FAMILY): return ERRNUM_AI_FAMILY;
	case -ABS(EAI_MEMORY): return ERRNUM_AI_MEMORY;
	case -ABS(EAI_NONAME): return ERRNUM_AI_NONAME;
	case -ABS(EAI_OVERFLOW): return ERRNUM_AI_OVERFLOW;
	case -ABS(EAI_SERVICE): return ERRNUM_AI_SERVICE;
	case -ABS(EAI_SOCKTYPE): return ERRNUM_AI_SOCKTYPE;
	default: break;
	}
#endif
	return errno2num(errc);
#endif // _WIN32
}

LELY_UTIL_EXPORT int
errnum2no(errnum_t errnum)
{
	switch (errnum) {
#ifdef E2BIG
	case ERRNUM_2BIG: return E2BIG;
#endif
#ifdef EACCES
	case ERRNUM_ACCES: return EACCES;
#endif
#ifdef EADDRINUSE
	case ERRNUM_ADDRINUSE: return EADDRINUSE;
#endif
#ifdef EADDRNOTAVAIL
	case ERRNUM_ADDRNOTAVAIL: return EADDRNOTAVAIL;
#endif
#ifdef EAFNOSUPPORT
	case ERRNUM_AFNOSUPPORT: return EAFNOSUPPORT;
#endif
#ifdef EAGAIN
	case ERRNUM_AGAIN: return EAGAIN;
#endif
#ifdef EALREADY
	case ERRNUM_ALREADY: return EALREADY;
#endif
#ifdef EBADF
	case ERRNUM_BADF: return EBADF;
#endif
#ifdef EBADMSG
	case ERRNUM_BADMSG: return EBADMSG;
#endif
#ifdef EBUSY
	case ERRNUM_BUSY: return EBUSY;
#endif
#ifdef ECANCELED
	case ERRNUM_CANCELED: return ECANCELED;
#endif
#ifdef ECHILD
	case ERRNUM_CHILD: return ECHILD;
#endif
#ifdef ECONNABORTED
	case ERRNUM_CONNABORTED: return ECONNABORTED;
#endif
#ifdef ECONNREFUSED
	case ERRNUM_CONNREFUSED: return ECONNREFUSED;
#endif
#ifdef ECONNRESET
	case ERRNUM_CONNRESET: return ECONNRESET;
#endif
#ifdef EDEADLK
	case ERRNUM_DEADLK: return EDEADLK;
#endif
#ifdef EDESTADDRREQ
	case ERRNUM_DESTADDRREQ: return EDESTADDRREQ;
#endif
	case ERRNUM_DOM: return EDOM;
//	case ERRNUM_DQUOT: return EDQUOT;
#ifdef EEXIST
	case ERRNUM_EXIST: return EEXIST;
#endif
#ifdef EFAULT
	case ERRNUM_FAULT: return EFAULT;
#endif
#ifdef EFBIG
	case ERRNUM_FBIG: return EFBIG;
#endif
#ifdef EHOSTUNREACH
	case ERRNUM_HOSTUNREACH: return EHOSTUNREACH;
#endif
#ifdef EIDRM
	case ERRNUM_IDRM: return EIDRM;
#endif
	case ERRNUM_ILSEQ: return EILSEQ;
#ifdef EINPROGRESS
	case ERRNUM_INPROGRESS: return EINPROGRESS;
#endif
#ifdef EINTR
	case ERRNUM_INTR: return EINTR;
#endif
#ifdef EINVAL
	case ERRNUM_INVAL: return EINVAL;
#endif
#ifdef EIO
	case ERRNUM_IO: return EIO;
#endif
#ifdef EISCONN
	case ERRNUM_ISCONN: return EISCONN;
#endif
#ifdef EISDIR
	case ERRNUM_ISDIR: return EISDIR;
#endif
#ifdef ELOOP
	case ERRNUM_LOOP: return ELOOP;
#endif
#ifdef EMFILE
	case ERRNUM_MFILE: return EMFILE;
#endif
#ifdef EMLINK
	case ERRNUM_MLINK: return EMLINK;
#endif
#ifdef EMSGSIZE
	case ERRNUM_MSGSIZE: return EMSGSIZE;
#endif
//	case ERRNUM_MULTIHOP: return EMULTIHOP;
#ifdef ENAMETOOLONG
	case ERRNUM_NAMETOOLONG: return ENAMETOOLONG;
#endif
#ifdef ENETDOWN
	case ERRNUM_NETDOWN: return ENETDOWN;
#endif
#ifdef ENETRESET
	case ERRNUM_NETRESET: return ENETRESET;
#endif
#ifdef ENETUNREACH
	case ERRNUM_NETUNREACH: return ENETUNREACH;
#endif
#ifdef ENFILE
	case ERRNUM_NFILE: return ENFILE;
#endif
#ifdef ENOBUFS
	case ERRNUM_NOBUFS: return ENOBUFS;
#endif
#ifdef ENODATA
	case ERRNUM_NODATA: return ENODATA;
#endif
#ifdef ENODEV
	case ERRNUM_NODEV: return ENODEV;
#endif
#ifdef ENOENT
	case ERRNUM_NOENT: return ENOENT;
#endif
#ifdef ENOEXEC
	case ERRNUM_NOEXEC: return ENOEXEC;
#endif
#ifdef ENOLCK
	case ERRNUM_NOLCK: return ENOLCK;
#endif
//	case ERRNUM_NOLINK: return ENOLINK;
#ifdef ENOMEM
	case ERRNUM_NOMEM: return ENOMEM;
#endif
#ifdef ENOMSG
	case ERRNUM_NOMSG: return ENOMSG;
#endif
#ifdef ENOPROTOOPT
	case ERRNUM_NOPROTOOPT: return ENOPROTOOPT;
#endif
#ifdef ENOSPC
	case ERRNUM_NOSPC: return ENOSPC;
#endif
#ifdef ENOSR
	case ERRNUM_NOSR: return ENOSR;
#endif
#ifdef ENOSTR
	case ERRNUM_NOSTR: return ENOSTR;
#endif
#ifdef ENOSYS
	case ERRNUM_NOSYS: return ENOSYS;
#endif
#ifdef ENOTCONN
	case ERRNUM_NOTCONN: return ENOTCONN;
#endif
#ifdef ENOTDIR
	case ERRNUM_NOTDIR: return ENOTDIR;
#endif
#ifdef ENOTEMPTY
	case ERRNUM_NOTEMPTY: return ENOTEMPTY;
#endif
#ifdef ENOTRECOVERABLE
	case ERRNUM_NOTRECOVERABLE: return ENOTRECOVERABLE;
#endif
#ifdef ENOTSOCK
	case ERRNUM_NOTSOCK: return ENOTSOCK;
#endif
#ifdef ENOTSUP
	case ERRNUM_NOTSUP: return ENOTSUP;
#endif
#ifdef ENOTTY
	case ERRNUM_NOTTY: return ENOTTY;
#endif
#ifdef ENXIO
	case ERRNUM_NXIO: return ENXIO;
#endif
#ifdef EOPNOTSUPP
	case ERRNUM_OPNOTSUPP: return EOPNOTSUPP;
#endif
#ifdef EOVERFLOW
	case ERRNUM_OVERFLOW: return EOVERFLOW;
#endif
#ifdef EOWNERDEAD
	case ERRNUM_OWNERDEAD: return EOWNERDEAD;
#endif
#ifdef EPERM
	case ERRNUM_PERM: return EPERM;
#endif
#ifdef EPIPE
	case ERRNUM_PIPE: return EPIPE;
#endif
#ifdef EPROTO
	case ERRNUM_PROTO: return EPROTO;
#endif
#ifdef EPROTONOSUPPORT
	case ERRNUM_PROTONOSUPPORT: return EPROTONOSUPPORT;
#endif
#ifdef EPROTOTYPE
	case ERRNUM_PROTOTYPE: return EPROTOTYPE;
#endif
	case ERRNUM_RANGE: return ERANGE;
#ifdef EROFS
	case ERRNUM_ROFS: return EROFS;
#endif
#ifdef ESPIPE
	case ERRNUM_SPIPE: return ESPIPE;
#endif
#ifdef ESRCH
	case ERRNUM_SRCH: return ESRCH;
#endif
//	case ERRNUM_STALE: return ESTALE;
#ifdef ETIME
	case ERRNUM_TIME: return ETIME;
#endif
#ifdef ETIMEDOUT
	case ERRNUM_TIMEDOUT: return ETIMEDOUT;
#endif
#ifdef ETXTBSY
	case ERRNUM_TXTBSY: return ETXTBSY;
#endif
#ifdef EWOULDBLOCK
	case ERRNUM_WOULDBLOCK: return EWOULDBLOCK;
#endif
#ifdef EXDEV
	case ERRNUM_XDEV: return EXDEV;
#endif
	default: return 0;
	}
}

LELY_UTIL_EXPORT errc_t
errnum2c(errnum_t errnum)
{
#ifdef _WIN32
	switch (errnum) {
	case ERRNUM_2BIG: return ERROR_BAD_ENVIRONMENT;
	case ERRNUM_ACCES: return ERROR_ACCESS_DENIED;
	case ERRNUM_ADDRINUSE: return WSAEADDRINUSE;
	case ERRNUM_ADDRNOTAVAIL: return WSAEADDRNOTAVAIL;
	case ERRNUM_AFNOSUPPORT: return WSAEAFNOSUPPORT;
	case ERRNUM_AGAIN: return ERROR_RETRY;
	case ERRNUM_ALREADY: return WSAEALREADY;
	case ERRNUM_BADF: return ERROR_INVALID_HANDLE;
//	case ERRNUM_BADMSG: return ...;
	case ERRNUM_BUSY: return ERROR_BUSY;
	case ERRNUM_CANCELED: return ERROR_OPERATION_ABORTED;
	case ERRNUM_CHILD: return ERROR_WAIT_NO_CHILDREN;
	case ERRNUM_CONNABORTED: return WSAECONNABORTED;
	case ERRNUM_CONNREFUSED: return WSAECONNREFUSED;
	case ERRNUM_CONNRESET: return WSAECONNRESET;
	case ERRNUM_DEADLK: return ERROR_POSSIBLE_DEADLOCK;
	case ERRNUM_DESTADDRREQ: return WSAEDESTADDRREQ;
#ifdef ERROR_FLOAT_MULTIPLE_FAULTS
	case ERRNUM_DOM: return ERROR_FLOAT_MULTIPLE_FAULTS;
#endif
//	case ERRNUM_DQUOT: return ...;
	case ERRNUM_EXIST: return ERROR_FILE_EXISTS;
	case ERRNUM_FAULT: return ERROR_NOACCESS;
	case ERRNUM_FBIG: return ERROR_NO_SYSTEM_RESOURCES;
	case ERRNUM_HOSTUNREACH: return WSAEHOSTUNREACH;
//	case ERRNUM_IDRM: return ...;
#ifdef ERROR_ILLEGAL_CHARACTER
	case ERRNUM_ILSEQ: return ERROR_ILLEGAL_CHARACTER;
#endif
	case ERRNUM_INPROGRESS: return WSAEINPROGRESS;
	case ERRNUM_INTR: return WSAEINTR;
	case ERRNUM_INVAL: return ERROR_INVALID_PARAMETER;
	case ERRNUM_IO: return ERROR_IO_DEVICE;
	case ERRNUM_ISCONN: return WSAEISCONN;
	case ERRNUM_ISDIR: return ERROR_FILE_EXISTS;
//	case ERRNUM_LOOP: return ...;
	case ERRNUM_MFILE: return ERROR_TOO_MANY_OPEN_FILES;
	case ERRNUM_MLINK: return ERROR_TOO_MANY_LINKS;
	case ERRNUM_MSGSIZE: return WSAEMSGSIZE;
//	case ERRNUM_MULTIHOP: return ...;
	case ERRNUM_NAMETOOLONG: return ERROR_FILENAME_EXCED_RANGE;
	case ERRNUM_NETDOWN: return WSAENETDOWN;
	case ERRNUM_NETRESET: return WSAENETRESET;
	case ERRNUM_NETUNREACH: return WSAENETUNREACH;
	case ERRNUM_NFILE: return ERROR_NO_MORE_SEARCH_HANDLES;
	case ERRNUM_NOBUFS: return WSAENOBUFS;
	case ERRNUM_NODATA: return ERROR_HANDLE_EOF;
	case ERRNUM_NOENT: return ERROR_PATH_NOT_FOUND;
	case ERRNUM_NOEXEC: return ERROR_BAD_FORMAT;
	case ERRNUM_NOLCK: return ERROR_LOCK_VIOLATION;
//	case ERRNUM_NOLINK: return ERROR_NOT_CONNECTED;
	case ERRNUM_NOMEM: return ERROR_NOT_ENOUGH_MEMORY;
//	case ERRNUM_NOMSG: return ...;
	case ERRNUM_NOPROTOOPT: return WSAENOPROTOOPT;
	case ERRNUM_NOSPC: return ERROR_DISK_FULL;
//	case ERRNUM_NOSR: return ...;
//	case ERRNUM_NOSTR: return ...;
	case ERRNUM_NOSYS: return ERROR_CALL_NOT_IMPLEMENTED;
	case ERRNUM_NOTCONN: return WSAENOTCONN;
	case ERRNUM_NOTDIR: return ERROR_DIRECTORY;
	case ERRNUM_NOTEMPTY: return ERROR_DIR_NOT_EMPTY;
//	case ERRNUM_NOTRECOVERABLE: return ...;
	case ERRNUM_NOTSOCK: return WSAENOTSOCK;
	case ERRNUM_NOTSUP: return ERROR_EAS_NOT_SUPPORTED;
	case ERRNUM_NOTTY: return ERROR_INVALID_CATEGORY;
	case ERRNUM_NXIO: return ERROR_FILE_INVALID;
	case ERRNUM_OPNOTSUPP: return WSAEOPNOTSUPP;
	case ERRNUM_OVERFLOW: return ERROR_INVALID_PARAMETER;
//	case ERRNUM_OWNERDEAD: return ...;
	case ERRNUM_PERM: return ERROR_PRIVILEGE_NOT_HELD;
	case ERRNUM_PIPE: return ERROR_BROKEN_PIPE;
//	case ERRNUM_PROTO: return ...;
	case ERRNUM_PROTONOSUPPORT: return WSAEPROTONOSUPPORT;
	case ERRNUM_PROTOTYPE: return WSAEPROTOTYPE;
	case ERRNUM_RANGE: return ERROR_ARITHMETIC_OVERFLOW;
	case ERRNUM_ROFS: return ERROR_WRITE_PROTECT;
	case ERRNUM_SPIPE: return ERROR_SEEK;
	case ERRNUM_SRCH: return ERROR_PROC_NOT_FOUND;
//	case ERRNUM_STALE: return ...;
//	case ERRNUM_TIME: return ...;
	case ERRNUM_TIMEDOUT: return ERROR_TIMEOUT;
//	case ERRNUM_TXTBSY: return ...;
	case ERRNUM_WOULDBLOCK: return WSAEWOULDBLOCK;
	case ERRNUM_XDEV: return ERROR_NOT_SAME_DEVICE;
	case ERRNUM_AI_AGAIN: return WSATRY_AGAIN;
	case ERRNUM_AI_BADFLAGS: return WSAEINVAL;
	case ERRNUM_AI_FAIL: return WSANO_RECOVERY;
	case ERRNUM_AI_FAMILY: return WSAEAFNOSUPPORT;
	case ERRNUM_AI_MEMORY: return ERROR_NOT_ENOUGH_MEMORY;
	case ERRNUM_AI_NONAME: return WSAHOST_NOT_FOUND;
	case ERRNUM_AI_SERVICE: return WSATYPE_NOT_FOUND;
	case ERRNUM_AI_SOCKTYPE: return WSAESOCKTNOSUPPORT;
	case ERRNUM_AI_OVERFLOW: return WSAEFAULT;
	default: return 0;
	}
#else
#if _POSIX_C_SOURCE >= 200112L
	switch (errnum) {
	case ERRNUM_AI_AGAIN: return -ABS(EAI_AGAIN);
	case ERRNUM_AI_BADFLAGS: return -ABS(EAI_BADFLAGS);
	case ERRNUM_AI_FAIL: return -ABS(EAI_FAIL);
	case ERRNUM_AI_FAMILY: return -ABS(EAI_FAMILY);
	case ERRNUM_AI_MEMORY: return -ABS(EAI_MEMORY);
	case ERRNUM_AI_NONAME: return -ABS(EAI_NONAME);
	case ERRNUM_AI_OVERFLOW: return -ABS(EAI_OVERFLOW);
	case ERRNUM_AI_SERVICE: return -ABS(EAI_SERVICE);
	case ERRNUM_AI_SOCKTYPE: return -ABS(EAI_SOCKTYPE);
	default: break;
	}
#endif
	return errnum2no(errnum);
#endif // _WIN32
}

LELY_UTIL_EXPORT errc_t
get_errc(void)
{
#ifdef _WIN32
	return GetLastError();
#else
	return errno;
#endif
}

LELY_UTIL_EXPORT void
set_errc(errc_t errc)
{
#ifdef _WIN32
	SetLastError(errc);
#else
	errno = errc;
#endif
}

LELY_UTIL_EXPORT const char *
errno2str(int errnum)
{
	return strerror(errnum);
}

LELY_UTIL_EXPORT const char *
errc2str(errc_t errc)
{
#ifdef _WIN32
	if (__unlikely(!FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER
			| FORMAT_MESSAGE_FROM_SYSTEM
			| FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errc,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			errstr, sizeof(errstr), NULL)))
		return NULL;
	// Remove terminating line-break ("\r\n") from error message.
	size_t n = strlen(errstr);
	if (__likely(n >= 2))
		errstr[n - 2] = '\0';
	return errstr;
#else
#if _POSIX_C_SOURCE >= 200112L
	switch (errc) {
	case -ABS(EAI_AGAIN): return gai_strerror(EAI_AGAIN);
	case -ABS(EAI_BADFLAGS): return gai_strerror(EAI_BADFLAGS);
	case -ABS(EAI_FAIL): return gai_strerror(EAI_FAIL);
	case -ABS(EAI_FAMILY): return gai_strerror(EAI_FAMILY);
	case -ABS(EAI_MEMORY): return gai_strerror(EAI_MEMORY);
	case -ABS(EAI_NONAME): return gai_strerror(EAI_NONAME);
	case -ABS(EAI_OVERFLOW): return gai_strerror(EAI_OVERFLOW);
	case -ABS(EAI_SERVICE): return gai_strerror(EAI_SERVICE);
	case -ABS(EAI_SOCKTYPE): return gai_strerror(EAI_SOCKTYPE);
	}
#endif
	return errno2str(errc);
#endif
}

