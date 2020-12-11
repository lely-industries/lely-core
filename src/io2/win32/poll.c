/**@file
 * This file is part of the I/O library; it contains the I/O polling
 * implementation for Windows.
 *
 * @see lely/io2/win32/poll.h
 *
 * @copyright 2018-2019 Lely Industries N.V.
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

#include "io.h"

#if !LELY_NO_STDIO && _WIN32

#include <lely/ev/task.h>
#include <lely/io2/win32/poll.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#ifndef LELY_IO_IOCP_COUNT
#define LELY_IO_IOCP_COUNT \
	MAX((LELY_VLA_SIZE_MAX / sizeof(OVERLAPPED_ENTRY)), 1)
#endif

struct io_poll_thrd {
	BOOL stopped;
#if !LELY_NO_THREADS
	LPDWORD lpdwThreadId;
#endif
};

#if !LELY_NO_THREADS
static void CALLBACK io_poll_kill_func(ULONG_PTR Parameter);
#endif

// clang-format off
static const struct io_svc_vtbl io_poll_svc_vtbl = {
	NULL,
	NULL
};
// clang-format on

static void *io_poll_poll_self(const ev_poll_t *poll);
static int io_poll_poll_wait(ev_poll_t *poll, int timeout);
static int io_poll_poll_kill(ev_poll_t *poll, void *thr);

// clang-format off
static const struct ev_poll_vtbl io_poll_poll_vtbl = {
	&io_poll_poll_self,
	&io_poll_poll_wait,
	&io_poll_poll_kill
};
// clang-format on

struct io_poll {
	struct io_svc svc;
	const struct ev_poll_vtbl *poll_vptr;
	io_ctx_t *ctx;
	HANDLE CompletionPort;
};

static inline io_poll_t *io_poll_from_svc(const struct io_svc *svc);
static inline io_poll_t *io_poll_from_poll(const ev_poll_t *poll);

void *
io_poll_alloc(void)
{
	void *ptr = malloc(sizeof(io_poll_t));
	if (!ptr)
		set_errc(errno2c(errno));
	return ptr;
}

void
io_poll_free(void *ptr)
{
	free(ptr);
}

io_poll_t *
io_poll_init(io_poll_t *poll, io_ctx_t *ctx)
{
	assert(poll);
	assert(ctx);

	DWORD dwErrCode = GetLastError();

	poll->svc = (struct io_svc)IO_SVC_INIT(&io_poll_svc_vtbl);
	poll->ctx = ctx;

	poll->poll_vptr = &io_poll_poll_vtbl;

	poll->CompletionPort = CreateIoCompletionPort(
			INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!poll->CompletionPort) {
		dwErrCode = GetLastError();
		goto error_CreateIoCompletionPort;
	}

	io_ctx_insert(poll->ctx, &poll->svc);

	return poll;

	// CloseHandle(poll->CompletionPort);
error_CreateIoCompletionPort:
	SetLastError(dwErrCode);
	return NULL;
}

void
io_poll_fini(io_poll_t *poll)
{
	assert(poll);

	io_ctx_remove(poll->ctx, &poll->svc);

	CloseHandle(poll->CompletionPort);
}

io_poll_t *
io_poll_create(io_ctx_t *ctx)
{
	DWORD dwErrCode = 0;

	io_poll_t *poll = io_poll_alloc();
	if (!poll) {
		dwErrCode = GetLastError();
		goto error_alloc;
	}

	io_poll_t *tmp = io_poll_init(poll, ctx);
	if (!tmp) {
		dwErrCode = GetLastError();
		goto error_init;
	}
	poll = tmp;

	return poll;

error_init:
	io_poll_free(poll);
error_alloc:
	SetLastError(dwErrCode);
	return NULL;
}

void
io_poll_destroy(io_poll_t *poll)
{
	if (poll) {
		io_poll_fini(poll);
		io_poll_free(poll);
	}
}

io_ctx_t *
io_poll_get_ctx(const io_poll_t *poll)
{
	assert(poll);

	return poll->ctx;
}

ev_poll_t *
io_poll_get_poll(const io_poll_t *poll)
{
	assert(poll);

	return &poll->poll_vptr;
}

int
io_poll_register_handle(io_poll_t *poll, HANDLE handle)
{
	assert(poll);

	// clang-format off
	return CreateIoCompletionPort(handle, poll->CompletionPort, 0, 0)
			? 0 : -1;
	// clang-format on
}

int
io_poll_post(io_poll_t *poll, size_t nbytes, struct io_cp *cp)
{
	assert(poll);
	assert(cp);

	// clang-format off
	return PostQueuedCompletionStatus(poll->CompletionPort, nbytes, 0,
			&cp->overlapped) ? 0 : -1;
	// clang-format on
}

#if !LELY_NO_THREADS
static void CALLBACK
io_poll_kill_func(ULONG_PTR Parameter)
{
	*(BOOL *)(PVOID)Parameter = TRUE;
}
#endif

static void *
io_poll_poll_self(const ev_poll_t *poll)
{
	(void)poll;

#if LELY_NO_THREADS
	static struct io_poll_thrd thr = { 0 };
#else
	static _Thread_local struct io_poll_thrd thr = { 0, NULL };
	if (!thr.lpdwThreadId) {
		static _Thread_local DWORD dwThreadId;
		dwThreadId = GetCurrentThreadId();
		thr.lpdwThreadId = &dwThreadId;
	}
#endif
	return &thr;
}

static int
io_poll_poll_wait(ev_poll_t *poll_, int timeout)
{
	io_poll_t *poll = io_poll_from_poll(poll_);
	void *thr_ = io_poll_poll_self(poll_);
	struct io_poll_thrd *thr = (struct io_poll_thrd *)thr_;

	DWORD dwMilliseconds = timeout < 0 ? INFINITE : (DWORD)timeout;

	int n = 0;
	DWORD dwErrCode = GetLastError();

	OVERLAPPED_ENTRY CompletionPortEntries[LELY_IO_IOCP_COUNT];
	ULONG ulNumEntriesRemoved = 0;

	do {
		if (thr->stopped)
			dwMilliseconds = 0;
		BOOL fSuccess = GetQueuedCompletionStatusEx(
				poll->CompletionPort, CompletionPortEntries,
				LELY_IO_IOCP_COUNT, &ulNumEntriesRemoved,
				dwMilliseconds, TRUE);
		// TODO: Check if we can detect an APC from the return code.
		if (thr->stopped)
			break;

		if (!fSuccess) {
			if (GetLastError() == WAIT_TIMEOUT) {
				if (dwMilliseconds && timeout < 0)
					continue;
			} else if (!n) {
				dwErrCode = GetLastError();
				n = -1;
			}
			break;
		}

		for (ULONG i = 0; i < ulNumEntriesRemoved; i++) {
			LPOVERLAPPED_ENTRY lpEntry = &CompletionPortEntries[i];
			LPOVERLAPPED lpOverlapped = lpEntry->lpOverlapped;
			if (!lpOverlapped)
				continue;
			struct io_cp *cp = structof(
					lpOverlapped, struct io_cp, overlapped);
			if (cp->func) {
				assert(lpfnRtlNtStatusToDosError);
				DWORD dwErrorCode = lpfnRtlNtStatusToDosError(
						lpOverlapped->Internal);
				cp->func(cp, lpEntry->dwNumberOfBytesTransferred,
						dwErrorCode);
			}
			n += n < INT_MAX;
		}

		dwMilliseconds = 0;
	} while (ulNumEntriesRemoved == LELY_IO_IOCP_COUNT);
	thr->stopped = FALSE;

	SetLastError(dwErrCode);
	return n;
}

static int
io_poll_poll_kill(ev_poll_t *poll, void *thr_)
{
	(void)poll;
#if LELY_NO_THREADS
	(void)thr_;
#else
	struct io_poll_thrd *thr = thr_;
	assert(thr);

	HANDLE hThread = OpenThread(
			THREAD_SET_CONTEXT, FALSE, *thr->lpdwThreadId);
	if (!hThread)
		return -1;

	if (!QueueUserAPC(&io_poll_kill_func, hThread,
			    (ULONG_PTR)(PVOID)&thr->stopped)) {
		DWORD dwErrCode = GetLastError();
		CloseHandle(hThread);
		SetLastError(dwErrCode);
		return -1;
	}
	CloseHandle(hThread);
#endif
	return 0;
}

static inline io_poll_t *
io_poll_from_svc(const struct io_svc *svc)
{
	assert(svc);

	return structof(svc, io_poll_t, svc);
}

static inline io_poll_t *
io_poll_from_poll(const ev_poll_t *poll)
{
	assert(poll);

	return structof(poll, io_poll_t, poll_vptr);
}

#endif // !LELY_NO_STDIO && _WIN32
