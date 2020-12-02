/**@file
 * This is the internal header file of the I/O handle declarations.
 *
 * @copyright 2017-2018 Lely Industries N.V.
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

#ifndef LELY_IO_INTERN_HANDLE_H_
#define LELY_IO_INTERN_HANDLE_H_

#include "io.h"
#include <lely/libc/stdatomic.h>
#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif

struct io_handle_vtab;

/// An I/O device handle.
struct io_handle {
	/// A pointer to the virtual table.
	const struct io_handle_vtab *vtab;
	/// The reference count.
#if !LELY_NO_ATOMICS
	atomic_size_t ref;
#elif !LELY_NO_THREADS && _WIN32
	volatile LONG ref;
#else
	size_t ref;
#endif
	/// The native file descriptor.
#if _WIN32
	HANDLE fd;
#else
	int fd;
#endif
	/**
	 * The I/O device flags (any combination of #IO_FLAG_NO_CLOSE and
	 * #IO_FLAG_NONBLOCK).
	 */
	int flags;
#if !LELY_NO_THREADS
	/// The mutex protecting #flags (and other device-specific fields).
	mtx_t mtx;
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

/// The virtual table of an I/O device handle.
struct io_handle_vtab {
	/**
	 * The type of the device (one of #IO_TYPE_CAN, #IO_TYPE_FILE,
	 * #IO_TYPE_PIPE, #IO_TYPE_SERIAL or #IO_TYPE_SOCK).
	 */
	int type;
	/// The size (in bytes) of the handle struct.
	size_t size;
	/// A pointer to the <b>fini</b> method.
	void (*fini)(struct io_handle *handle);
	/// A pointer to the static <b>flags</b> method.
	int (*flags)(struct io_handle *handle, int flags);
	/// A pointer to the <b>read</b> method. @see io_read()
	ssize_t (*read)(struct io_handle *handle, void *buf, size_t nbytes);
	/// A pointer to the <b>write</b> method. @see io_write()
	ssize_t (*write)(struct io_handle *handle, const void *buf,
			size_t nbytes);
	/// A pointer to the <b>flush</b> method. @see io_flush()
	int (*flush)(struct io_handle *handle);
	/// A pointer to the <b>seek</b> method. @see io_seek()
	io_off_t (*seek)(struct io_handle *handle, io_off_t offset, int whence);
	/// A pointer to the <b>pread</b> method. @see io_pread()
	ssize_t (*pread)(struct io_handle *handle, void *buf, size_t nbytes,
			io_off_t offset);
	/// A pointer to the <b>pwrite</b> method. @see io_pwrite()
	ssize_t (*pwrite)(struct io_handle *handle, const void *buf,
			size_t nbytes, io_off_t offset);
	/// A pointer to the <b>purge</b> method. @see io_purge()
	int (*purge)(struct io_handle *handle, int flags);
	/// A pointer to the <b>recv</b> method. @see io_recv()
	ssize_t (*recv)(struct io_handle *handle, void *buf, size_t nbytes,
			io_addr_t *addr, int flags);
	/// A pointer to the <b>send</b> method. @see io_send()
	ssize_t (*send)(struct io_handle *handle, const void *buf,
			size_t nbytes, const io_addr_t *addr, int flags);
	/// A pointer to the <b>accept</b> method. @see io_accept()
	struct io_handle *(*accept)(struct io_handle *handle, io_addr_t *addr);
	/// A pointer to the <b>connect</b> method. @see io_connect()
	int (*connect)(struct io_handle *handle, const io_addr_t *addr);
};

/**
 * Allocates a new I/O device handle from a virtual table. On success, the
 * reference count is initialized to zero.
 *
 * @returns a pointer to a new device handle, or NULL on error.
 *
 * @see io_handle_free().
 */
struct io_handle *io_handle_alloc(const struct io_handle_vtab *vtab);

/// Frees an I/O device handle. @see io_handle_alloc()
void io_handle_free(struct io_handle *handle);

/**
 * Finalizes an I/O device handle by invoking its <b>fini</b> method, if
 * available.
 */
void io_handle_fini(struct io_handle *handle);

/**
 * Destroys an I/O device handle. This function SHOULD never be called directly.
 * Call io_handle_release() instead.
 */
void io_handle_destroy(struct io_handle *handle);

/**
 * Locks an unlocked I/O device handle, so the flags (and other device-specific
 * fields) can safely be accessed.
 *
 * @see io_handle_unlock()
 */
#if LELY_NO_THREADS
#define io_handle_lock(handle)
#else
void io_handle_lock(struct io_handle *handle);
#endif

/// Unlocks a locked I/O device handle. @see io_handle_lock()
#if LELY_NO_THREADS
#define io_handle_unlock(handle)
#else
void io_handle_unlock(struct io_handle *handle);
#endif

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_INTERN_HANDLE_H_
