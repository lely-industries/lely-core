/*!\file
 * This header file is part of the utilities library; it contains the native and
 * platform-independent error number declarations.
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

#ifndef LELY_UTIL_ERRNUM_H
#define LELY_UTIL_ERRNUM_H

#include <lely/util/util.h>

#include <errno.h>

#ifdef _WIN32
#include <winerror.h>
#elif _POSIX_C_SOURCE >= 200112L
#include <netdb.h>
#endif

//! The native error code type.
#ifdef _WIN32
typedef DWORD errc_t;
#else
typedef int errc_t;
#endif

//! The platform-independent error numbers.
enum errnum {
	//! Mathematics argument out of domain of function.
	ERRNUM_DOM = EDOM,
	//! Illegal byte sequence.
	ERRNUM_ILSEQ = EILSEQ,
	//! Result too large.
	ERRNUM_RANGE = ERANGE,
#ifdef E2BIG
	//! Argument list too long.
	ERRNUM_2BIG = E2BIG,
#endif
#ifdef EACCES
	//! Permission denied.
	ERRNUM_ACCES = EACCES,
#endif
#ifdef EADDRINUSE
	//! Address in use.
	ERRNUM_ADDRINUSE = EADDRINUSE,
#endif
#ifdef EADDRNOTAVAIL
	//! Address not available.
	ERRNUM_ADDRNOTAVAIL = EADDRNOTAVAIL,
#endif
#ifdef EAFNOSUPPORT
	//! Address family not supported.
	ERRNUM_AFNOSUPPORT = EAFNOSUPPORT,
#endif
#ifdef EAGAIN
	//! Resource unavailable, try again.
	ERRNUM_AGAIN = EAGAIN,
#endif
#ifdef EALREADY
	//! Connection already in progress.
	ERRNUM_ALREADY = EALREADY,
#endif
#ifdef EBADF
	//! Bad file descriptor.
	ERRNUM_BADF = EBADF,
#endif
#ifdef EBADMSG
	//! Bad message.
	ERRNUM_BADMSG = EBADMSG,
#endif
#ifdef EBUSY
	//! Device or resource busy.
	ERRNUM_BUSY = EBUSY,
#endif
#ifdef ECANCELED
	//! Operation canceled.
	ERRNUM_CANCELED = ECANCELED,
#endif
#ifdef ECHILD
	//! No child process.
	ERRNUM_CHILD = ECHILD,
#endif
#ifdef ECONNABORTED
	//! Connection aborted.
	ERRNUM_CONNABORTED = ECONNABORTED,
#endif
#ifdef ECONNREFUSED
	//! Connection refused.
	ERRNUM_CONNREFUSED = ECONNREFUSED,
#endif
#ifdef ECONNRESET
	//! Connection reset.
	ERRNUM_CONNRESET = ECONNRESET,
#endif
#ifdef EDEADLK
	//! Resource deadlock would occur.
	ERRNUM_DEADLK = EDEADLK,
#endif
#ifdef EDESTADDRREQ
	//! Destination address required.
	ERRNUM_DESTADDRREQ = EDESTADDRREQ,
#endif
#ifdef EEXIST
	//! File exists.
	ERRNUM_EXIST = EEXIST,
#endif
#ifdef EFAULT
	//! Bad address.
	ERRNUM_FAULT = EFAULT,
#endif
#ifdef EFBIG
	//! File too large.
	ERRNUM_FBIG = EFBIG,
#endif
#ifdef EHOSTUNREACH
	//! Host is unreachable.
	ERRNUM_HOSTUNREACH = EHOSTUNREACH,
#endif
#ifdef EIDRM
	//! Identifier removed.
	ERRNUM_IDRM = EIDRM,
#endif
#ifdef EINPROGRESS
	//! Operation in progress.
	ERRNUM_INPROGRESS = EINPROGRESS,
#endif
#ifdef EINTR
	//! Interrupted function.
	ERRNUM_INTR = EINTR,
#endif
#ifdef EINVAL
	//! Invalid argument.
	ERRNUM_INVAL = EINVAL,
#endif
#ifdef EIO
	//! I/O error.
	ERRNUM_IO = EIO,
#endif
#ifdef EISCONN
	//! Socket is connected.
	ERRNUM_ISCONN = EISCONN,
#endif
#ifdef EISDIR
	//! Is a directory.
	ERRNUM_ISDIR = EISDIR,
#endif
#ifdef ELOOP
	//! Too many levels of symbolic links.
	ERRNUM_LOOP = ELOOP,
#endif
#ifdef EMFILE
	//! File descriptor value too large.
	ERRNUM_MFILE = EMFILE,
#endif
#ifdef EMLINK
	//! Too many links.
	ERRNUM_MLINK = EMLINK,
#endif
#ifdef EMSGSIZE
	//! Message too large.
	ERRNUM_MSGSIZE = EMSGSIZE,
#endif
#ifdef ENAMETOOLONG
	//! Filename too long.
	ERRNUM_NAMETOOLONG = ENAMETOOLONG,
#endif
#ifdef ENETDOWN
	//! Network is down.
	ERRNUM_NETDOWN = ENETDOWN,
#endif
#ifdef ENETRESET
	//! Connection aborted by network.
	ERRNUM_NETRESET = ENETRESET,
#endif
#ifdef ENETUNREACH
	//! Network unreachable.
	ERRNUM_NETUNREACH = ENETUNREACH,
#endif
#ifdef ENFILE
	//! Too many files open in system.
	ERRNUM_NFILE = ENFILE,
#endif
#ifdef ENOBUFS
	//! No buffer space available.
	ERRNUM_NOBUFS = ENOBUFS,
#endif
#ifdef ENODATA
	//! No message is available on the STREAM head read queue.
	ERRNUM_NODATA = ENODATA,
#endif
#ifdef ENODEV
	//! No such device.
	ERRNUM_NODEV = ENODEV,
#endif
#ifdef ENOENT
	//! No such file or directory.
	ERRNUM_NOENT = ENOENT,
#endif
#ifdef ENOEXEC
	//! Executable file format error.
	ERRNUM_NOEXEC = ENOEXEC,
#endif
#ifdef ENOLCK
	//! No locks available.
	ERRNUM_NOLCK = ENOLCK,
#endif
#ifdef ENOLINK
	//! Reserved.
	ERRNUM_NOLINK = ENOLINK,
#endif
#ifdef ENOMEM
	//! Not enough space.
	ERRNUM_NOMEM = ENOMEM,
#endif
#ifdef ENOMSG
	//! No message of the desired type.
	ERRNUM_NOMSG = ENOMSG,
#endif
#ifdef ENOPROTOOPT
	//! Protocol not available.
	ERRNUM_NOPROTOOPT = ENOPROTOOPT,
#endif
#ifdef ENOSPC
	//! No space left on device.
	ERRNUM_NOSPC = ENOSPC,
#endif
#ifdef ENOSR
	//! No STREAM resources.
	ERRNUM_NOSR = ENOSR,
#endif
#ifdef ENOSTR
	//! Not a STREAM.
	ERRNUM_NOSTR = ENOSTR,
#endif
#ifdef ENOSYS
	//! Function not supported.
	ERRNUM_NOSYS = ENOSYS,
#endif
#ifdef ENOTCONN
	//! The socket is not connected.
	ERRNUM_NOTCONN = ENOTCONN,
#endif
#ifdef ENOTDIR
	//! Not a directory or a symbolic link to a directory.
	ERRNUM_NOTDIR = ENOTDIR,
#endif
#ifdef ENOTEMPTY
	//! Directory not empty.
	ERRNUM_NOTEMPTY = ENOTEMPTY,
#endif
#ifdef ENOTRECOVERABLE
	//! State not recoverable.
	ERRNUM_NOTRECOVERABLE = ENOTRECOVERABLE,
#endif
#ifdef ENOTSOCK
	//! Not a socket.
	ERRNUM_NOTSOCK = ENOTSOCK,
#endif
#ifdef ENOTSUP
	//! Not supported.
	ERRNUM_NOTSUP = ENOTSUP,
#endif
#ifdef ENOTTY
	//! Inappropriate I/O control operation.
	ERRNUM_NOTTY = ENOTTY,
#endif
#ifdef ENXIO
	//! No such device or address.
	ERRNUM_NXIO = ENXIO,
#endif
#ifdef EOPNOTSUPP
	//! Operation not supported on socket.
	ERRNUM_OPNOTSUPP = EOPNOTSUPP,
#endif
#ifdef EOVERFLOW
	//! Value too large to be stored in data type.
	ERRNUM_OVERFLOW = EOVERFLOW,
#endif
#ifdef EOWNERDEAD
	//! Previous owner died.
	ERRNUM_OWNERDEAD = EOWNERDEAD,
#endif
#ifdef EPERM
	//! Operation not permitted.
	ERRNUM_PERM = EPERM,
#endif
#ifdef EPIPE
	//! Broken pipe.
	ERRNUM_PIPE = EPIPE,
#endif
#ifdef EPROTO
	//! Protocol error.
	ERRNUM_PROTO = EPROTO,
#endif
#ifdef EPROTONOSUPPORT
	//! Protocol not supported.
	ERRNUM_PROTONOSUPPORT = EPROTONOSUPPORT,
#endif
#ifdef EPROTOTYPE
	//! Protocol wrong type for socket.
	ERRNUM_PROTOTYPE = EPROTOTYPE,
#endif
#ifdef EROFS
	//! Read-only file system.
	ERRNUM_ROFS = EROFS,
#endif
#ifdef ESPIPE
	//! Invalid seek.
	ERRNUM_SPIPE = ESPIPE,
#endif
#ifdef ESRCH
	//! No such process.
	ERRNUM_SRCH = ESRCH,
#endif
#ifdef ETIME
	//! Stream ioctl() timeout.
	ERRNUM_TIME = ETIME,
#endif
#ifdef ETIMEDOUT
	//! Connection timed out.
	ERRNUM_TIMEDOUT = ETIMEDOUT,
#endif
#ifdef ETXTBSY
	//! Text file busy.
	ERRNUM_TXTBSY = ETXTBSY,
#endif
#ifdef EWOULDBLOCK
	//! Operation would block.
	ERRNUM_WOULDBLOCK = EWOULDBLOCK,
#endif
#ifdef EXDEV
	//! Cross-device link.
	ERRNUM_XDEV = EXDEV,
#endif
#ifdef EAI_AGAIN
	/*!
	 * The name could not be resolved at this time. Future attempts may
	 * succeed.
	 */
	ERRNUM_AI_AGAIN = EAI_AGAIN,
#endif
#ifdef EAI_BADFLAGS
	//! The flags had an invalid value.
	ERRNUM_AI_BADFLAGS = EAI_BADFLAGS,
#endif
#ifdef EAI_FAIL
	//! A non-recoverable error occurred.
	ERRNUM_AI_FAIL = EAI_FAIL,
#endif
#ifdef EAI_FAMILY
	/*!
	 * The address family was not recognized or the address length was
	 * invalid for the specified family.
	 */
	ERRNUM_AI_FAMILY = EAI_FAMILY,
#endif
#ifdef EAI_MEMORY
	//! There was a memory allocation failure.
	ERRNUM_AI_MEMORY = EAI_MEMORY,
#endif
#ifdef EAI_NONAME
	//! The name does not resolve for the supplied parameters.
	ERRNUM_AI_NONAME = EAI_NONAME,
#endif
#ifdef EAI_OVERFLOW
	//! An argument buffer overflowed.
	ERRNUM_AI_OVERFLOW = EAI_OVERFLOW,
#endif
#ifdef EAI_SERVICE
	//! The service passed was not recognized for the specified socket type.
	ERRNUM_AI_SERVICE = EAI_SERVICE,
#endif
#ifdef EAI_SOCKTYPE
	//! The intended socket type was not recognized.
	ERRNUM_AI_SOCKTYPE = EAI_SOCKTYPE,
#endif
	// To prevent duplicate error numbers, this value should be at least as
	// large as the largest explicitly defined error number, and small
	// enough that the last implicitly defined error number does not exceed
	// INT_MAX.
	__ERRNUM_MAX_DEFINED = INT_MAX - 84 - 1,
#ifndef E2BIG
	//! Argument list too long.
	ERRNUM_2BIG,
#endif
#ifndef EACCES
	//! Permission denied.
	ERRNUM_ACCES,
#endif
#ifndef EADDRINUSE
	//! Address in use.
	ERRNUM_ADDRINUSE,
#endif
#ifndef EADDRNOTAVAIL
	//! Address not available.
	ERRNUM_ADDRNOTAVAIL,
#endif
#ifndef EAFNOSUPPORT
	//! Address family not supported.
	ERRNUM_AFNOSUPPORT,
#endif
#ifndef EAGAIN
	//! Resource unavailable, try again.
	ERRNUM_AGAIN,
#endif
#ifndef EALREADY
	//! Connection already in progress.
	ERRNUM_ALREADY,
#endif
#ifndef EBADF
	//! Bad file descriptor.
	ERRNUM_BADF,
#endif
#ifndef EBADMSG
	//! Bad message.
	ERRNUM_BADMSG,
#endif
#ifndef EBUSY
	//! Device or resource busy.
	ERRNUM_BUSY,
#endif
#ifndef ECANCELED
	//! Operation canceled.
	ERRNUM_CANCELED,
#endif
#ifndef ECHILD
	//! No child process.
	ERRNUM_CHILD,
#endif
#ifndef ECONNABORTED
	//! Connection aborted.
	ERRNUM_CONNABORTED,
#endif
#ifndef ECONNREFUSED
	//! Connection refused.
	ERRNUM_CONNREFUSED,
#endif
#ifndef ECONNRESET
	//! Connection reset.
	ERRNUM_CONNRESET,
#endif
#ifndef EDEADLK
	//! Resource deadlock would occur.
	ERRNUM_DEADLK,
#endif
#ifndef EDESTADDRREQ
	//! Destination address required.
	ERRNUM_DESTADDRREQ,
#endif
#ifndef EEXIST
	//! File exists.
	ERRNUM_EXIST,
#endif
#ifndef EFAULT
	//! Bad address.
	ERRNUM_FAULT,
#endif
#ifndef EFBIG
	//! File too large.
	ERRNUM_FBIG,
#endif
#ifndef EHOSTUNREACH
	//! Host is unreachable.
	ERRNUM_HOSTUNREACH,
#endif
#ifndef EIDRM
	//! Identifier removed.
	ERRNUM_IDRM,
#endif
#ifndef EINPROGRESS
	//! Operation in progress.
	ERRNUM_INPROGRESS,
#endif
#ifndef EINTR
	//! Interrupted function.
	ERRNUM_INTR,
#endif
#ifndef EINVAL
	//! Invalid argument.
	ERRNUM_INVAL,
#endif
#ifndef EIO
	//! I/O error.
	ERRNUM_IO,
#endif
#ifndef EISCONN
	//! Socket is connected.
	ERRNUM_ISCONN,
#endif
#ifndef EISDIR
	//! Is a directory.
	ERRNUM_ISDIR,
#endif
#ifndef ELOOP
	//! Too many levels of symbolic links.
	ERRNUM_LOOP,
#endif
#ifndef EMFILE
	//! File descriptor value too large.
	ERRNUM_MFILE,
#endif
#ifndef EMLINK
	//! Too many links.
	ERRNUM_MLINK,
#endif
#ifndef EMSGSIZE
	//! Message too large.
	ERRNUM_MSGSIZE,
#endif
#ifndef ENAMETOOLONG
	//! Filename too long.
	ERRNUM_NAMETOOLONG,
#endif
#ifndef ENETDOWN
	//! Network is down.
	ERRNUM_NETDOWN,
#endif
#ifndef ENETRESET
	//! Connection aborted by network.
	ERRNUM_NETRESET,
#endif
#ifndef ENETUNREACH
	//! Network unreachable.
	ERRNUM_NETUNREACH,
#endif
#ifndef ENFILE
	//! Too many files open in system.
	ERRNUM_NFILE,
#endif
#ifndef ENOBUFS
	//! No buffer space available.
	ERRNUM_NOBUFS,
#endif
#ifndef ENODATA
	//! No message is available on the STREAM head read queue.
	ERRNUM_NODATA,
#endif
#ifndef ENODEV
	//! No such device.
	ERRNUM_NODEV,
#endif
#ifndef ENOENT
	//! No such file or directory.
	ERRNUM_NOENT,
#endif
#ifndef ENOEXEC
	//! Executable file format error.
	ERRNUM_NOEXEC,
#endif
#ifndef ENOLCK
	//! No locks available.
	ERRNUM_NOLCK,
#endif
#ifndef ENOLINK
	//! Reserved.
	ERRNUM_NOLINK,
#endif
#ifndef ENOMEM
	//! Not enough space.
	ERRNUM_NOMEM,
#endif
#ifndef ENOMSG
	//! No message of the desired type.
	ERRNUM_NOMSG,
#endif
#ifndef ENOPROTOOPT
	//! Protocol not available.
	ERRNUM_NOPROTOOPT,
#endif
#ifndef ENOSPC
	//! No space left on device.
	ERRNUM_NOSPC,
#endif
#ifndef ENOSR
	//! No STREAM resources.
	ERRNUM_NOSR,
#endif
#ifndef ENOSTR
	//! Not a STREAM.
	ERRNUM_NOSTR,
#endif
#ifndef ENOSYS
	//! Function not supported.
	ERRNUM_NOSYS,
#endif
#ifndef ENOTCONN
	//! The socket is not connected.
	ERRNUM_NOTCONN,
#endif
#ifndef ENOTDIR
	//! Not a directory or a symbolic link to a directory.
	ERRNUM_NOTDIR,
#endif
#ifndef ENOTEMPTY
	//! Directory not empty.
	ERRNUM_NOTEMPTY,
#endif
#ifndef ENOTRECOVERABLE
	//! State not recoverable.
	ERRNUM_NOTRECOVERABLE,
#endif
#ifndef ENOTSOCK
	//! Not a socket.
	ERRNUM_NOTSOCK,
#endif
#ifndef ENOTSUP
	//! Not supported.
	ERRNUM_NOTSUP,
#endif
#ifndef ENOTTY
	//! Inappropriate I/O control operation.
	ERRNUM_NOTTY,
#endif
#ifndef ENXIO
	//! No such device or address.
	ERRNUM_NXIO,
#endif
#ifndef EOPNOTSUPP
	//! Operation not supported on socket.
	ERRNUM_OPNOTSUPP,
#endif
#ifndef EOVERFLOW
	//! Value too large to be stored in data type.
	ERRNUM_OVERFLOW,
#endif
#ifndef EOWNERDEAD
	//! Previous owner died.
	ERRNUM_OWNERDEAD,
#endif
#ifndef EPERM
	//! Operation not permitted.
	ERRNUM_PERM,
#endif
#ifndef EPIPE
	//! Broken pipe.
	ERRNUM_PIPE,
#endif
#ifndef EPROTO
	//! Protocol error.
	ERRNUM_PROTO,
#endif
#ifndef EPROTONOSUPPORT
	//! Protocol not supported.
	ERRNUM_PROTONOSUPPORT,
#endif
#ifndef EPROTOTYPE
	//! Protocol wrong type for socket.
	ERRNUM_PROTOTYPE,
#endif
#ifndef EROFS
	//! Read-only file system.
	ERRNUM_ROFS,
#endif
#ifndef ESPIPE
	//! Invalid seek.
	ERRNUM_SPIPE,
#endif
#ifndef ESRCH
	//! No such process.
	ERRNUM_SRCH,
#endif
#ifndef ETIME
	//! Stream ioctl() timeout.
	ERRNUM_TIME,
#endif
#ifndef ETIMEDOUT
	//! Connection timed out.
	ERRNUM_TIMEDOUT,
#endif
#ifndef ETXTBSY
	//! Text file busy.
	ERRNUM_TXTBSY,
#endif
#ifndef EWOULDBLOCK
	//! Operation would block.
	ERRNUM_WOULDBLOCK,
#endif
#ifndef EXDEV
	//! Cross-device link.
	ERRNUM_XDEV,
#endif
//	ERRNUM_DQUOT,
//	ERRNUM_MULTIHOP,
//	ERRNUM_STALE,
#ifndef EAI_AGAIN
	/*!
	 * The name could not be resolved at this time. Future attempts may
	 * succeed.
	 */
	ERRNUM_AI_AGAIN,
#endif
#ifndef EAI_BADFLAGS
	//! The flags had an invalid value.
	ERRNUM_AI_BADFLAGS,
#endif
#ifndef EAI_FAIL
	//! A non-recoverable error occurred.
	ERRNUM_AI_FAIL,
#endif
#ifndef EAI_FAMILY
	/*!
	 * The address family was not recognized or the address length was
	 * invalid for the specified family.
	 */
	ERRNUM_AI_FAMILY,
#endif
#ifndef EAI_MEMORY
	//! There was a memory allocation failure.
	ERRNUM_AI_MEMORY,
#endif
#ifndef EAI_NONAME
	//! The name does not resolve for the supplied parameters.
	ERRNUM_AI_NONAME,
#endif
#ifndef EAI_OVERFLOW
	//! An argument buffer overflowed.
	ERRNUM_AI_OVERFLOW,
#endif
#ifndef EAI_SERVICE
	//! The service passed was not recognized for the specified socket type.
	ERRNUM_AI_SERVICE,
#endif
#ifndef EAI_SOCKTYPE
	//! The intended socket type was not recognized.
	ERRNUM_AI_SOCKTYPE,
#endif
	__ERRNUM_MAX
};

//! The platform-independent error number type.
#ifdef __cplusplus
typedef int errnum_t;
#else
typedef enum errnum errnum_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Transforms a native error code to a platform-independent error number.
 *
 * \see errnum2c()
 */
LELY_UTIL_EXTERN errnum_t errc2num(errc_t errc);

/*!
 * Transforms a platform-independent error number to a native error code.
 *
 * \see err2num()
 */
LELY_UTIL_EXTERN errc_t errnum2c(errnum_t errnum);

/*!
 * Returns the last (thread-specific) native error code set by a system call or
 * library function. This is equivalent to `GetLastError()` on Microsoft
 * Windows, or `errno` on other platforms.
 *
 * This function returns the thread-specific error number.
 *
 * \see set_errc()
 */
LELY_UTIL_EXTERN errc_t get_errc(void);

/*!
 * Sets the current (thread-specific) native error code to \a errc. This is
 * equivalent to `SetLastError(errnum)` on Microsoft Windows, or `errno = errc`
 * on other platforms.
 *
 * \see get_errc()
 */
LELY_UTIL_EXTERN void set_errc(errc_t errc);

/*!
 * Returns the last (thread-specific) platform-independent error number set by a
 * system call or library function. This is equivalent to
 * `errc2num(get_errc())`.
 *
 * \see set_errnum()
 */
static inline errnum_t get_errnum(void);

/*!
 * Sets the current (thread-specific) platform-independent error number to
 * \a errnum. This is equivalent to `set_errc(errnum2c(errnum))`.
 *
 * \see get_errnum()
 */
static inline void set_errnum(errnum_t errnum);

//! Returns a string describing a native error code.
LELY_UTIL_EXTERN const char *errc2str(errc_t errc);

/*!
 * Returns a string describing a platform-independent error number.This is
 * equivalent to `errc2str(errnum2c(errnum))`.
 */
static inline const char *errnum2str(errnum_t errnum);

static inline errnum_t
get_errnum(void)
{
	return errc2num(get_errc());
}

static inline void
set_errnum(errnum_t errnum)
{
	set_errc(errnum2c(errnum));
}

static inline const char *
errnum2str(errnum_t errnum)
{
	return errc2str(errnum2c(errnum));
}

#ifdef __cplusplus
}
#endif

#endif

