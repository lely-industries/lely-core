/**@file
 * This file is part of the I/O library; it contains the IXXAT CAN bus
 * implementation for Windows.
 *
 * @see lely/io2/win32/ixxat.h
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#if !LELY_NO_STDIO && _WIN32 && LELY_HAVE_IXXAT

// Rename error flags to avoid conflicts with definitions in <cantype.h>.
#define CAN_ERROR_BIT LELY_IO_CAN_ERROR_BIT
#define CAN_ERROR_STUFF LELY_IO_CAN_ERROR_STUFF
#define CAN_ERROR_CRC LELY_IO_CAN_ERROR_CRC
#define CAN_ERROR_FORM LELY_IO_CAN_ERROR_FORM
#define CAN_ERROR_ACK LELY_IO_CAN_ERROR_ACK
#define CAN_ERROR_OTHER LELY_IO_CAN_ERROR_OTHER

#include "../can.h"
#include <lely/io2/ctx.h>
#include <lely/io2/win32/ixxat.h>
#include <lely/util/diag.h>
#include <lely/util/endian.h>
#include <lely/util/util.h>

#undef CAN_ERROR_OTHER
#undef CAN_ERROR_ACK
#undef CAN_ERROR_FORM
#undef CAN_ERROR_CRC
#undef CAN_ERROR_STUFF
#undef CAN_ERROR_BIT

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <processthreadsapi.h>
#include <windows.h>

#ifdef __MINGW32__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#define _MSC_VER 1900
#endif

#if LELY_NO_CANFD
#include <vcinpl.h>
#else
#include <vcinpl2.h>
#endif
#ifdef __MINGW32__
#undef _MSC_VER
#pragma GCC diagnostic pop
#endif

#ifndef LELY_IO_IXXAT_RX_FIFO_SIZE
/**
 * The default size of the receive buffer of a CAN channel (in number of
 * frames).
 */
#define LELY_IO_IXXAT_RX_FIFO_SIZE 1024
#endif

#ifndef LELY_IO_IXXAT_TX_FIFO_SIZE
/**
 * The default size of the transmit buffer of a CAN channel (in number of
 * frames).
 */
#define LELY_IO_IXXAT_TX_FIFO_SIZE 128
#endif

static size_t io_ixxat_init_refcnt = 0;

static HMODULE hLibModule;

#define LELY_IO_DEFINE_IXXAT(name) static PF_##name p_##name;
#include "ixxat.inc"
#undef LELY_IO_DEFINE_IXXAT

static int io_ixxat_read(HANDLE hCanChn, struct can_msg *msg,
		struct can_err *err, PUINT32 pdwTime, PUINT32 pdwTimeOvr,
		int timeout);
static int io_ixxat_write(
		HANDLE hCanChn, const struct can_msg *msg, int timeout);
static DWORD io_ixxat_error(HRESULT hResult);
static int io_ixxat_state(UINT32 dwStatus);
static struct timespec io_ixxat_time(
		UINT64 qwTime, UINT32 dwTscClkFreq, UINT32 dwTscDivisor);

static int io_ixxat_ctrl_stop(io_can_ctrl_t *ctrl);
static int io_ixxat_ctrl_stopped(const io_can_ctrl_t *ctrl);
static int io_ixxat_ctrl_restart(io_can_ctrl_t *ctrl);
static int io_ixxat_ctrl_get_bitrate(
		const io_can_ctrl_t *ctrl, int *pnominal, int *pdata);
static int io_ixxat_ctrl_set_bitrate(
		io_can_ctrl_t *ctrl, int nominal, int data);
static int io_ixxat_ctrl_get_state(const io_can_ctrl_t *ctrl);

// clang-format off
static const struct io_can_ctrl_vtbl io_ixxat_ctrl_vtbl = {
	&io_ixxat_ctrl_stop,
	&io_ixxat_ctrl_stopped,
	&io_ixxat_ctrl_restart,
	&io_ixxat_ctrl_get_bitrate,
	&io_ixxat_ctrl_set_bitrate,
	&io_ixxat_ctrl_get_state
};
// clang-format on

struct io_ixxat_ctrl {
	const struct io_can_ctrl_vtbl *ctrl_vptr;
	HANDLE hDevice;
	UINT32 dwCanNo;
	HANDLE hCanCtl;
	int flags;
	UINT32 dwTscClkFreq;
	UINT32 dwTscDivisor;
};

static inline struct io_ixxat_ctrl *io_ixxat_ctrl_from_ctrl(
		const io_can_ctrl_t *ctrl);

static io_ctx_t *io_ixxat_chan_dev_get_ctx(const io_dev_t *dev);
static ev_exec_t *io_ixxat_chan_dev_get_exec(const io_dev_t *dev);
static size_t io_ixxat_chan_dev_cancel(io_dev_t *dev, struct ev_task *task);
static size_t io_ixxat_chan_dev_abort(io_dev_t *dev, struct ev_task *task);

// clang-format off
static const struct io_dev_vtbl io_ixxat_chan_dev_vtbl = {
	&io_ixxat_chan_dev_get_ctx,
	&io_ixxat_chan_dev_get_exec,
	&io_ixxat_chan_dev_cancel,
	&io_ixxat_chan_dev_abort
};
// clang-format on

static io_dev_t *io_ixxat_chan_get_dev(const io_can_chan_t *chan);
static int io_ixxat_chan_get_flags(const io_can_chan_t *chan);
static int io_ixxat_chan_read(io_can_chan_t *chan, struct can_msg *msg,
		struct can_err *err, struct timespec *tp, int timeout);
static void io_ixxat_chan_submit_read(
		io_can_chan_t *chan, struct io_can_chan_read *read);
static int io_ixxat_chan_write(
		io_can_chan_t *chan, const struct can_msg *msg, int timeout);
static void io_ixxat_chan_submit_write(
		io_can_chan_t *chan, struct io_can_chan_write *write);

// clang-format off
static const struct io_can_chan_vtbl io_ixxat_chan_vtbl = {
	&io_ixxat_chan_get_dev,
	&io_ixxat_chan_get_flags,
	&io_ixxat_chan_read,
	&io_ixxat_chan_submit_read,
	&io_ixxat_chan_write,
	&io_ixxat_chan_submit_write
};
// clang-format on

static void io_ixxat_chan_svc_shutdown(struct io_svc *svc);

// clang-format off
static const struct io_svc_vtbl io_ixxat_chan_svc_vtbl = {
	NULL,
	&io_ixxat_chan_svc_shutdown
};
// clang-format on

struct io_ixxat_chan {
	const struct io_dev_vtbl *dev_vptr;
	const struct io_can_chan_vtbl *chan_vptr;
	struct io_svc svc;
	io_ctx_t *ctx;
	ev_exec_t *exec;
	int rxtimeo;
	int txtimeo;
	struct ev_task read_task;
	struct ev_task write_task;
#if !LELY_NO_THREADS
	CRITICAL_SECTION CriticalSection;
#endif
	HANDLE hCanChn;
	int flags;
	UINT32 dwTscClkFreq;
	UINT32 dwTscDivisor;
	UINT32 dwTimeOvr;
	unsigned shutdown : 1;
	unsigned read_posted : 1;
	unsigned write_posted : 1;
	struct sllist read_queue;
	struct sllist write_queue;
	struct io_can_chan_read *current_read;
	struct io_can_chan_write *current_write;
};

static void io_ixxat_chan_read_task_func(struct ev_task *task);
static void io_ixxat_chan_write_task_func(struct ev_task *task);

static inline struct io_ixxat_chan *io_ixxat_chan_from_dev(const io_dev_t *dev);
static inline struct io_ixxat_chan *io_ixxat_chan_from_chan(
		const io_can_chan_t *chan);
static inline struct io_ixxat_chan *io_ixxat_chan_from_svc(
		const struct io_svc *svc);

static int io_ixxat_chan_read_impl(struct io_ixxat_chan *ixxat,
		struct can_msg *msg, struct can_err *err, struct timespec *tp,
		int timeout);
static int io_ixxat_chan_write_impl(struct io_ixxat_chan *ixxat,
		const struct can_msg *msg, int timeout);

static void io_ixxat_chan_pop(struct io_ixxat_chan *ixxat,
		struct sllist *read_queue, struct sllist *write_queue,
		struct ev_task *task);

static size_t io_ixxat_chan_do_abort_tasks(struct io_ixxat_chan *ixxat);

static HANDLE io_ixxat_chan_set_handle(struct io_ixxat_chan *ixxat,
		HANDLE hCanChn, int flags, UINT32 dwTscClkFreq,
		UINT32 dwTscDivisor);

int
io_ixxat_init(void)
{
	if (io_ixxat_init_refcnt++)
		return 0;

	DWORD dwErrCode = 0;

#if LELY_NO_CANFD
	hLibModule = LoadLibraryA("vcinpl.dll");
#else
	hLibModule = LoadLibraryA("vcinpl2.dll");
#endif
	if (!hLibModule) {
		dwErrCode = GetLastError();
		goto error_LoadLibrary;
	}

#if defined(__MINGW32__) && __GNUC__ >= 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
#define LELY_IO_DEFINE_IXXAT(name) \
	if (!(p_##name = (PF_##name)GetProcAddress(hLibModule, #name))) { \
		dwErrCode = GetLastError(); \
		goto error_GetProcAddress; \
	}
#include "ixxat.inc"
#undef LELY_IO_DEFINE_IXXAT
#if defined(__MINGW32__) && __GNUC__ >= 8
#pragma GCC diagnostic pop
#endif

	return 0;

error_GetProcAddress:
#define LELY_IO_DEFINE_IXXAT(name) p_##name = NULL;
#include "ixxat.inc"
#undef LELY_IO_DEFINE_IXXAT
	FreeLibrary(hLibModule);
	hLibModule = NULL;
error_LoadLibrary:
	SetLastError(dwErrCode);
	io_ixxat_init_refcnt--;
	return -1;
}

void
io_ixxat_fini(void)
{
	assert(io_ixxat_init_refcnt);

	if (--io_ixxat_init_refcnt)
		return;

#define LELY_IO_DEFINE_IXXAT(name) p_##name = NULL;
#include "ixxat.inc"
#undef LELY_IO_DEFINE_IXXAT

	FreeLibrary(hLibModule);
	hLibModule = NULL;
}

void *
io_ixxat_ctrl_alloc(void)
{
	struct io_ixxat_ctrl *ixxat = malloc(sizeof(*ixxat));
	if (!ixxat) {
		set_errc(errno2c(errno));
		return NULL;
	}
	return &ixxat->ctrl_vptr;
}

void
io_ixxat_ctrl_free(void *ptr)
{
	if (ptr)
		free(io_ixxat_ctrl_from_ctrl(ptr));
}

io_can_ctrl_t *
io_ixxat_ctrl_init(io_can_ctrl_t *ctrl, const LUID *lpLuid, UINT32 dwCanNo,
		int flags, int nominal, int data)
{
	struct io_ixxat_ctrl *ixxat = io_ixxat_ctrl_from_ctrl(ctrl);
	assert(lpLuid);

	DWORD dwErrCode = 0;
	HRESULT hResult = VCI_OK;

	ixxat->ctrl_vptr = &io_ixxat_ctrl_vtbl;

	VCIID VciObjectId = { .AsLuid = *lpLuid };
	ixxat->hDevice = NULL;
	if ((hResult = p_vciDeviceOpen(&VciObjectId, &ixxat->hDevice))
			!= VCI_OK) {
		if (hResult == VCI_E_INVALIDARG)
			dwErrCode = ERROR_DEV_NOT_EXIST;
		else
			dwErrCode = io_ixxat_error(hResult);
		goto error_vciDeviceOpen;
	}

	ixxat->dwCanNo = dwCanNo;
	ixxat->hCanCtl = NULL;
	// clang-format off
	if ((hResult = p_canControlOpen(ixxat->hDevice, ixxat->dwCanNo,
			&ixxat->hCanCtl)) != VCI_OK) {
		// clang-format on
		if (hResult == VCI_E_INVALID_INDEX)
			dwErrCode = ERROR_DEV_NOT_EXIST;
		else
			dwErrCode = io_ixxat_error(hResult);
		goto error_canControlOpen;
	}

#if LELY_NO_CANFD
	CANCAPABILITIES sCanCaps;
#else
	CANCAPABILITIES2 sCanCaps;
#endif
	if ((hResult = p_canControlGetCaps(ixxat->hCanCtl, &sCanCaps))
			!= VCI_OK) {
		dwErrCode = io_ixxat_error(hResult);
		goto error_canControlGetCaps;
	}
	if (!(sCanCaps.dwFeatures & CAN_FEATURE_STDANDEXT)) {
		dwErrCode = io_ixxat_error(hResult);
		goto error_canControlGetCaps;
	}

	ixxat->flags = flags;
	if (ixxat->flags & ~IO_CAN_BUS_FLAG_MASK) {
		dwErrCode = ERROR_INVALID_PARAMETER;
		goto error_flags;
	}
	if ((ixxat->flags & IO_CAN_BUS_FLAG_ERR)
			&& !(sCanCaps.dwFeatures & CAN_FEATURE_ERRFRAME)) {
		dwErrCode = ERROR_INVALID_PARAMETER;
		goto error_flags;
	}
#if !LELY_NO_CANFD
	if ((ixxat->flags & IO_CAN_BUS_FLAG_FDF)
			&& !(sCanCaps.dwFeatures & CAN_FEATURE_EXTDATA)) {
		dwErrCode = ERROR_INVALID_PARAMETER;
		goto error_flags;
	}
	if ((ixxat->flags & IO_CAN_BUS_FLAG_BRS)
			&& !(sCanCaps.dwFeatures & CAN_FEATURE_FASTDATA)) {
		dwErrCode = ERROR_INVALID_PARAMETER;
		goto error_flags;
	}
#endif

#if LELY_NO_CANFD
	ixxat->dwTscClkFreq = sCanCaps.dwClockFreq;
#else
	ixxat->dwTscClkFreq = sCanCaps.dwTscClkFreq;
#endif
	ixxat->dwTscDivisor = sCanCaps.dwTscDivisor;

	if (io_can_ctrl_set_bitrate(ctrl, nominal, data) == -1) {
		dwErrCode = GetLastError();
		goto error_set_bitrate;
	}

	return ctrl;

error_set_bitrate:
error_flags:
error_canControlGetCaps:
	p_canControlClose(ixxat->hCanCtl);
error_canControlOpen:
	p_vciDeviceClose(ixxat->hDevice);
error_vciDeviceOpen:
	SetLastError(dwErrCode);
	return NULL;
}

void
io_ixxat_ctrl_fini(io_can_ctrl_t *ctrl)
{
	struct io_ixxat_ctrl *ixxat = io_ixxat_ctrl_from_ctrl(ctrl);

	p_canControlClose(ixxat->hCanCtl);
	p_vciDeviceClose(ixxat->hDevice);
}

io_can_ctrl_t *
io_ixxat_ctrl_create_from_index(UINT32 dwIndex, UINT32 dwCanNo, int flags,
		int nominal, int data)
{
	HRESULT hResult = VCI_OK;

	HANDLE hEnum = NULL;
	if ((hResult = p_vciEnumDeviceOpen(&hEnum)) != VCI_OK) {
		SetLastError(io_ixxat_error(hResult));
		return NULL;
	}

	VCIDEVICEINFO sInfo = { .VciObjectId = { .AsLuid = { 0, 0 } } };
	do {
		if ((hResult = p_vciEnumDeviceNext(hEnum, &sInfo)) != VCI_OK) {
			p_vciEnumDeviceClose(hEnum);
			if (hResult == VCI_E_NO_MORE_ITEMS)
				SetLastError(ERROR_DEV_NOT_EXIST);
			else
				SetLastError(io_ixxat_error(hResult));
			return NULL;
		}
	} while (dwIndex-- > 0);

	p_vciEnumDeviceClose(hEnum);

	return io_ixxat_ctrl_create_from_luid(&sInfo.VciObjectId.AsLuid,
			dwCanNo, flags, nominal, data);
}

io_can_ctrl_t *
io_ixxat_ctrl_create_from_luid(const LUID *lpLuid, UINT32 dwCanNo, int flags,
		int nominal, int data)
{
	DWORD dwErrCode = 0;

	io_can_ctrl_t *ctrl = io_ixxat_ctrl_alloc();
	if (!ctrl) {
		dwErrCode = GetLastError();
		goto error_alloc;
	}

	io_can_ctrl_t *tmp = io_ixxat_ctrl_init(
			ctrl, lpLuid, dwCanNo, flags, nominal, data);
	if (!tmp) {
		dwErrCode = GetLastError();
		goto error_init;
	}
	ctrl = tmp;

	return ctrl;

error_init:
	io_ixxat_ctrl_free((void *)ctrl);
error_alloc:
	SetLastError(dwErrCode);
	return NULL;
}

void
io_ixxat_ctrl_destroy(io_can_ctrl_t *ctrl)
{
	if (ctrl) {
		io_ixxat_ctrl_fini(ctrl);
		io_ixxat_ctrl_free((void *)ctrl);
	}
}

HANDLE
io_ixxat_ctrl_get_handle(const io_can_ctrl_t *ctrl)
{
	const struct io_ixxat_ctrl *ixxat = io_ixxat_ctrl_from_ctrl(ctrl);

	return ixxat->hCanCtl;
}

void *
io_ixxat_chan_alloc(void)
{
	struct io_ixxat_chan *ixxat = malloc(sizeof(*ixxat));
	if (!ixxat) {
		set_errc(errno2c(errno));
		return NULL;
	}
	return &ixxat->chan_vptr;
}

void
io_ixxat_chan_free(void *ptr)
{
	if (ptr)
		free(io_ixxat_chan_from_chan(ptr));
}

io_can_chan_t *
io_ixxat_chan_init(io_can_chan_t *chan, io_ctx_t *ctx, ev_exec_t *exec,
		int rxtimeo, int txtimeo)
{
	struct io_ixxat_chan *ixxat = io_ixxat_chan_from_chan(chan);
	assert(ctx);

	if (!rxtimeo)
		rxtimeo = LELY_IO_RX_TIMEOUT;
	if (!txtimeo)
		txtimeo = LELY_IO_TX_TIMEOUT;

	ixxat->dev_vptr = &io_ixxat_chan_dev_vtbl;
	ixxat->chan_vptr = &io_ixxat_chan_vtbl;

	ixxat->svc = (struct io_svc)IO_SVC_INIT(&io_ixxat_chan_svc_vtbl);
	ixxat->ctx = ctx;

	ixxat->exec = exec;

	ixxat->rxtimeo = rxtimeo;
	ixxat->txtimeo = txtimeo;

	ixxat->read_task = (struct ev_task)EV_TASK_INIT(
			ixxat->exec, &io_ixxat_chan_read_task_func);
	ixxat->write_task = (struct ev_task)EV_TASK_INIT(
			ixxat->exec, &io_ixxat_chan_write_task_func);

#if !LELY_NO_THREADS
	InitializeCriticalSection(&ixxat->CriticalSection);
#endif

	ixxat->hCanChn = NULL;

	ixxat->flags = 0;
	ixxat->dwTscClkFreq = 0;
	ixxat->dwTscDivisor = 0;
	ixxat->dwTimeOvr = 0;

	ixxat->shutdown = 0;
	ixxat->read_posted = 0;
	ixxat->write_posted = 0;

	sllist_init(&ixxat->read_queue);
	sllist_init(&ixxat->write_queue);

	ixxat->current_read = NULL;
	ixxat->current_write = NULL;

	io_ctx_insert(ixxat->ctx, &ixxat->svc);

	return chan;
}

void
io_ixxat_chan_fini(io_can_chan_t *chan)
{
	struct io_ixxat_chan *ixxat = io_ixxat_chan_from_chan(chan);

	io_ctx_remove(ixxat->ctx, &ixxat->svc);
	// Cancel all pending operations.
	io_ixxat_chan_svc_shutdown(&ixxat->svc);

	// Deactivate the channel.
	if (ixxat->hCanChn)
		p_canChannelActivate(ixxat->hCanChn, FALSE);

#if !LELY_NO_THREADS
	int warning = 0;
	EnterCriticalSection(&ixxat->CriticalSection);
	// If necessary, busy-wait until io_ixxat_chan_read_task_func() and
	// io_ixxat_chan_write_task_func() complete.
	while (ixxat->read_posted || ixxat->write_posted) {
		if (io_ixxat_chan_do_abort_tasks(ixxat))
			continue;
		LeaveCriticalSection(&ixxat->CriticalSection);
		if (!warning) {
			warning = 1;
			diag(DIAG_WARNING, 0,
					"io_ixxat_chan_fini() invoked with pending operations");
		}
		SwitchToThread();
		EnterCriticalSection(&ixxat->CriticalSection);
	}
	LeaveCriticalSection(&ixxat->CriticalSection);
#endif

	// Close the channel.
	if (ixxat->hCanChn)
		p_canChannelClose(ixxat->hCanChn);

#if !LELY_NO_THREADS
	DeleteCriticalSection(&ixxat->CriticalSection);
#endif
}

io_can_chan_t *
io_ixxat_chan_create(io_ctx_t *ctx, ev_exec_t *exec, int rxtimeo, int txtimeo)
{
	DWORD dwErrCode = 0;

	io_can_chan_t *chan = io_ixxat_chan_alloc();
	if (!chan) {
		dwErrCode = GetLastError();
		goto error_alloc;
	}

	io_can_chan_t *tmp =
			io_ixxat_chan_init(chan, ctx, exec, rxtimeo, txtimeo);
	if (!tmp) {
		dwErrCode = GetLastError();
		goto error_init;
	}
	chan = tmp;

	return chan;

error_init:
	io_ixxat_chan_free((void *)chan);
error_alloc:
	SetLastError(dwErrCode);
	return NULL;
}

void
io_ixxat_chan_destroy(io_can_chan_t *chan)
{
	if (chan) {
		io_ixxat_chan_fini(chan);
		io_ixxat_chan_free((void *)chan);
	}
}

HANDLE
io_ixxat_chan_get_handle(const io_can_chan_t *chan)
{
	const struct io_ixxat_chan *ixxat = io_ixxat_chan_from_chan(chan);

#if !LELY_NO_THREADS
	EnterCriticalSection((LPCRITICAL_SECTION)&ixxat->CriticalSection);
#endif
	HANDLE hCanChn = ixxat->hCanChn;
#if !LELY_NO_THREADS
	LeaveCriticalSection((LPCRITICAL_SECTION)&ixxat->CriticalSection);
#endif
	return hCanChn;
}

int
io_ixxat_chan_open(io_can_chan_t *chan, const io_can_ctrl_t *ctrl,
		UINT16 wRxFifoSize, UINT16 wTxFifoSize)
{
	struct io_ixxat_chan *ixxat_chan = io_ixxat_chan_from_chan(chan);
	struct io_ixxat_ctrl *ixxat_ctrl = io_ixxat_ctrl_from_ctrl(ctrl);

	if (!wRxFifoSize)
		wRxFifoSize = LELY_IO_IXXAT_RX_FIFO_SIZE;
	if (!wTxFifoSize)
		wTxFifoSize = LELY_IO_IXXAT_TX_FIFO_SIZE;

	DWORD dwErrCode = 0;
	HRESULT hResult;

	HANDLE hCanChn = NULL;
	// clang-format off
	if ((hResult = p_canChannelOpen(ixxat_ctrl->hDevice,
			ixxat_ctrl->dwCanNo, FALSE, &hCanChn)) != VCI_OK) {
		// clang-format on
		dwErrCode = io_ixxat_error(hResult);
		goto error_canChannelOpen;
	}

	// clang-format off
#if LELY_NO_CANFD
	if ((hResult = p_canChannelInitialize(hCanChn, wRxFifoSize, 1,
			wTxFifoSize, 1)) != VCI_OK) {
#else
	if ((hResult = p_canChannelInitialize(hCanChn, wRxFifoSize, 1,
			wTxFifoSize, 1, 0, CAN_FILTER_PASS)) != VCI_OK) {
#endif
		// clang-format on
		dwErrCode = io_ixxat_error(hResult);
		goto error_canChannelInitialize;
	}

	if ((hResult = p_canChannelActivate(hCanChn, TRUE)) != VCI_OK) {
		dwErrCode = io_ixxat_error(hResult);
		goto error_canChannelActivate;
	}

	hCanChn = io_ixxat_chan_set_handle(ixxat_chan, hCanChn,
			ixxat_ctrl->flags, ixxat_ctrl->dwTscClkFreq,
			ixxat_ctrl->dwTscDivisor);
	if (hCanChn)
		p_canChannelClose(hCanChn);

	return 0;

error_canChannelActivate:
error_canChannelInitialize:
	p_canChannelClose(hCanChn);
error_canChannelOpen:
	SetLastError(dwErrCode);
	return -1;
}

int
io_ixxat_chan_assign(io_can_chan_t *chan, HANDLE hCanChn, UINT32 dwTscClkFreq,
		UINT32 dwTscDivisor)
{
	struct io_ixxat_chan *ixxat = io_ixxat_chan_from_chan(chan);

	HRESULT hResult;

	if (!hCanChn) {
		SetLastError(ERROR_INVALID_HANDLE);
		return -1;
	}

#if LELY_NO_CANFD
	CANCHANSTATUS sStatus;
#else
	CANCHANSTATUS2 sStatus;
#endif
	if ((hResult = p_canChannelGetStatus(hCanChn, &sStatus)) != VCI_OK) {
		SetLastError(io_ixxat_error(hResult));
		return -1;
	}

	int flags = 0;
	if (sStatus.sLineStatus.bOpMode & CAN_OPMODE_ERRFRAME)
		flags |= IO_CAN_BUS_FLAG_ERR;
#if !LELY_NO_CANFD
	if (sStatus.sLineStatus.bExMode & CAN_EXMODE_EXTDATA)
		flags |= IO_CAN_BUS_FLAG_FDF;
	if (sStatus.sLineStatus.bExMode & CAN_EXMODE_FASTDATA)
		flags |= IO_CAN_BUS_FLAG_BRS;
#endif

	if (!sStatus.fActivated
			&& (hResult = p_canChannelActivate(hCanChn, TRUE))
					!= VCI_OK) {
		SetLastError(io_ixxat_error(hResult));
		return -1;
	}

	hCanChn = io_ixxat_chan_set_handle(
			ixxat, hCanChn, flags, dwTscClkFreq, dwTscDivisor);
	if (hCanChn)
		p_canChannelClose(hCanChn);

	return 0;
}

HANDLE
io_ixxat_chan_release(io_can_chan_t *chan)
{
	struct io_ixxat_chan *ixxat = io_ixxat_chan_from_chan(chan);

	return io_ixxat_chan_set_handle(ixxat, NULL, 0, 0, 0);
}

int
io_ixxat_chan_is_open(const io_can_chan_t *chan)
{
	return io_ixxat_chan_get_handle(chan) != NULL;
}

int
io_ixxat_chan_close(io_can_chan_t *chan)
{
	HANDLE hCanChn = io_ixxat_chan_release(chan);
	if (hCanChn) {
		HRESULT hResult = p_canChannelClose(hCanChn);
		if (hResult != VCI_OK) {
			SetLastError(io_ixxat_error(hResult));
			return -1;
		}
	}
	return 0;
}

static int
io_ixxat_read(HANDLE hCanChn, struct can_msg *msg, struct can_err *err,
		PUINT32 pdwTime, PUINT32 pdwTimeOvr, int timeout)
{
	if (pdwTimeOvr)
		*pdwTimeOvr = 0;

	if (!hCanChn) {
		SetLastError(ERROR_INVALID_HANDLE);
		return -1;
	}

#if LELY_NO_CANFD
	CANMSG CanMsg = { .dwMsgId = 0 };
#else
	CANMSG2 CanMsg = { .dwMsgId = 0 };
#endif
	UINT32 type = 0;
	DWORD dwTimeout = timeout < 0 ? INFINITE : (DWORD)timeout;
	for (;;) {
		HRESULT hResult = p_canChannelReadMessage(
				hCanChn, dwTimeout, &CanMsg);
		if (hResult != VCI_OK) {
			SetLastError(io_ixxat_error(hResult));
			return -1;
		}
		type = CanMsg.uMsgInfo.Bits.type;
		if (type == CAN_MSGTYPE_DATA || type == CAN_MSGTYPE_ERROR
				|| type == CAN_MSGTYPE_STATUS) {
			break;
		} else {
			if (type == CAN_MSGTYPE_TIMEOVR && pdwTimeOvr)
				// The number of occurred time stamp counter
				// overflows is stored in the CAN ID.
				*pdwTimeOvr += CanMsg.dwMsgId;
			if (dwTimeout != INFINITE) {
				SetLastError(ERROR_TIMEOUT);
				return -1;
			}
		}
	}

	int result = 1;
	if (type == CAN_MSGTYPE_ERROR) {
		result = 0;

		enum can_error error = err ? err->error : 0;
		switch (CanMsg.abData[0]) {
		case CAN_ERROR_STUFF: error |= LELY_IO_CAN_ERROR_STUFF; break;
		case CAN_ERROR_FORM: error |= LELY_IO_CAN_ERROR_FORM; break;
		case CAN_ERROR_ACK: error |= LELY_IO_CAN_ERROR_ACK; break;
		case CAN_ERROR_BIT: error |= LELY_IO_CAN_ERROR_BIT; break;
		case CAN_ERROR_CRC: error |= LELY_IO_CAN_ERROR_CRC; break;
		default: error |= LELY_IO_CAN_ERROR_OTHER; break;
		}

		enum can_state state = io_ixxat_state(CanMsg.abData[1]);

		if (err) {
			err->state = state;
			err->error = error;
		}
	} else if (type == CAN_MSGTYPE_STATUS) {
		result = 0;

		if (err)
			err->state = io_ixxat_state(CanMsg.abData[0]);
	} else if (msg) {
		assert(type == CAN_MSGTYPE_DATA);

		*msg = (struct can_msg)CAN_MSG_INIT;
		if (CanMsg.uMsgInfo.Bits.ext) {
			msg->id = letoh32(CanMsg.dwMsgId) & CAN_MASK_EID;
			msg->flags |= CAN_FLAG_IDE;
		} else {
			msg->id = letoh32(CanMsg.dwMsgId) & CAN_MASK_BID;
		}
		if (CanMsg.uMsgInfo.Bits.rtr)
			msg->flags |= CAN_FLAG_RTR;
#if !LELY_NO_CANFD
		if (CanMsg.uMsgInfo.Bits.edl)
			msg->flags |= CAN_FLAG_FDF;
		if (CanMsg.uMsgInfo.Bits.fdr)
			msg->flags |= CAN_FLAG_BRS;
		if (CanMsg.uMsgInfo.Bits.esi)
			msg->flags |= CAN_FLAG_ESI;
#endif
#if !LELY_NO_CANFD
		if (CanMsg.uMsgInfo.Bits.edl) {
			if (CanMsg.uMsgInfo.Bits.dlc <= 8) {
				msg->len = CanMsg.uMsgInfo.Bits.dlc;
			} else {
				switch (CanMsg.uMsgInfo.Bits.dlc) {
				case 0x9: msg->len = 12; break;
				case 0xa: msg->len = 16; break;
				case 0xb: msg->len = 20; break;
				case 0xc: msg->len = 24; break;
				case 0xd: msg->len = 32; break;
				case 0xe: msg->len = 48; break;
				case 0xf: msg->len = 64; break;
				}
			}
		} else
#endif
			msg->len = CanMsg.uMsgInfo.Bits.dlc;
		memcpy(msg->data, CanMsg.abData, msg->len);
	}

	if (pdwTime)
		*pdwTime = CanMsg.dwTime;

	return result;
}

static int
io_ixxat_write(HANDLE hCanChn, const struct can_msg *msg, int timeout)
{
	assert(msg);

	if (!hCanChn) {
		SetLastError(ERROR_INVALID_HANDLE);
		return -1;
	}

#if LELY_NO_CANFD
	CANMSG CanMsg = { .dwMsgId = 0 };
#else
	CANMSG2 CanMsg = { .dwMsgId = 0 };
#endif
	if (msg->flags & CAN_FLAG_IDE) {
		CanMsg.dwMsgId = htole32(msg->id & CAN_MASK_EID);
		CanMsg.uMsgInfo.Bits.ext = 1;
	} else {
		CanMsg.dwMsgId = htole32(msg->id & CAN_MASK_BID);
	}
	CanMsg.uMsgInfo.Bits.type = CAN_MSGTYPE_DATA;
	if (msg->flags & CAN_FLAG_RTR)
		CanMsg.uMsgInfo.Bits.rtr = 1;
#if !LELY_NO_CANFD
	if (msg->flags & CAN_FLAG_FDF)
		CanMsg.uMsgInfo.Bits.edl = 1;
	if (msg->flags & CAN_FLAG_BRS)
		CanMsg.uMsgInfo.Bits.fdr = 1;
	if (msg->flags & CAN_FLAG_ESI)
		CanMsg.uMsgInfo.Bits.esi = 1;
#endif
	int len;
#if !LELY_NO_CANFD
	if (msg->flags & CAN_FLAG_FDF) {
		len = MIN(msg->len, CANFD_MAX_LEN);
		if (len <= 8)
			CanMsg.uMsgInfo.Bits.dlc = len;
		else if (len <= 12)
			CanMsg.uMsgInfo.Bits.dlc = 0x9;
		else if (len <= 16)
			CanMsg.uMsgInfo.Bits.dlc = 0xa;
		else if (len <= 20)
			CanMsg.uMsgInfo.Bits.dlc = 0xb;
		else if (len <= 24)
			CanMsg.uMsgInfo.Bits.dlc = 0xc;
		else if (len <= 32)
			CanMsg.uMsgInfo.Bits.dlc = 0xd;
		else if (len <= 48)
			CanMsg.uMsgInfo.Bits.dlc = 0xe;
		else
			CanMsg.uMsgInfo.Bits.dlc = 0xf;
	} else
#endif
		len = CanMsg.uMsgInfo.Bits.dlc = MIN(msg->len, CAN_MAX_LEN);
	memcpy(CanMsg.abData, msg->data, len);

	DWORD dwTimeout = timeout < 0 ? INFINITE : (DWORD)timeout;
	HRESULT hResult = p_canChannelSendMessage(hCanChn, dwTimeout, &CanMsg);
	if (hResult != VCI_OK) {
		SetLastError(io_ixxat_error(hResult));
		return -1;
	}

	return 0;
}

static DWORD
io_ixxat_error(HRESULT hResult)
{
	if (GETSEVERITY(hResult) != SEV_ERROR)
		return 0;

	switch (GETSCODE(hResult)) {
	case 0x0001: // VCI_E_UNEXPECTED
		return ERROR_UNEXP_NET_ERR;
	case 0x0002: // VCI_E_NOT_IMPLEMENTED
		return ERROR_CALL_NOT_IMPLEMENTED;
	case 0x0003: // VCI_E_OUTOFMEMORY
		return ERROR_NOT_ENOUGH_MEMORY;
	case 0x0004: // VCI_E_INVALIDARG
		return ERROR_BAD_ARGUMENTS;
	case 0x0005: // VCI_E_NOINTERFACE
#ifndef ERROR_NOINTERFACE
#define ERROR_NOINTERFACE 632
#endif
		return ERROR_NOINTERFACE;
	case 0x0006: // VCI_E_INVPOINTER
		return ERROR_INVALID_ADDRESS;
	case 0x0007: // VCI_E_INVHANDLE
		return ERROR_INVALID_HANDLE;
	case 0x0008: // VCI_E_ABORT
		return ERROR_OPERATION_ABORTED;
	case 0x0009: // VCI_E_FAIL
		return ERROR_GEN_FAILURE;
	case 0x000a: // VCI_E_ACCESSDENIED
		return ERROR_ACCESS_DENIED;
	case 0x000b: // VCI_E_TIMEOUT
		return ERROR_TIMEOUT;
	case 0x000c: // VCI_E_BUSY
		return ERROR_BUSY;
	case 0x000d: // VCI_E_PENDING
		return ERROR_IO_PENDING;
	case 0x000e: // VCI_E_NO_DATA
		return ERROR_NO_DATA;
	case 0x000f: // VCI_E_NO_MORE_ITEMS
		return ERROR_NO_MORE_ITEMS;
	case 0x0010: // VCI_E_NOT_INITIALIZED
#ifndef PEERDIST_ERROR_NOT_INITIALIZED
#define PEERDIST_ERROR_NOT_INITIALIZED 4054
#endif
		return PEERDIST_ERROR_NOT_INITIALIZED;
	case 0x0011: // VCI_E_ALREADY_INITIALIZED
#ifndef PEERDIST_ERROR_ALREADY_INITIALIZED
#define PEERDIST_ERROR_ALREADY_INITIALIZED 4055
#endif
		return PEERDIST_ERROR_ALREADY_INITIALIZED;
	case 0x0012: // VCI_E_RXQUEUE_EMPTY
	case 0x0013: // VCI_E_TXQUEUE_FULL
		return ERROR_RETRY;
	case 0x0014: // VCI_E_BUFFER_OVERFLOW
		return ERROR_BUFFER_OVERFLOW;
	case 0x0015: // VCI_E_INVALID_STATE
#ifndef PEERDIST_ERROR_INVALIDATED
#define PEERDIST_ERROR_INVALIDATED 4057
#endif
		return PEERDIST_ERROR_INVALIDATED;
	case 0x0016: // VCI_E_OBJECT_ALREADY_EXISTS
		return ERROR_ALREADY_EXISTS;
	case 0x0017: // VCI_E_INVALID_INDEX
		return ERROR_INVALID_INDEX;
	case 0x0018: // VCI_E_END_OF_FILE
		return ERROR_HANDLE_EOF;
	case 0x0019: // VCI_E_DISCONNECTED
		return ERROR_GRACEFUL_DISCONNECT;
	case 0x001a: // VCI_E_INVALID_FIRMWARE
#ifndef PEERDIST_ERROR_VERSION_UNSUPPORTED
#define PEERDIST_ERROR_VERSION_UNSUPPORTED 4062
#endif
		return PEERDIST_ERROR_VERSION_UNSUPPORTED;
	case 0x001b: // VCI_E_INVALID_LICENSE
	case 0x001c: // VCI_E_NO_SUCH_LICENSE
	case 0x001d: // VCI_E_LICENSE_EXPIRED
#ifndef PEERDIST_ERROR_NOT_LICENSED
#define PEERDIST_ERROR_NOT_LICENSED 4064
#endif
		return PEERDIST_ERROR_NOT_LICENSED;
	case 0x001e: // VCI_E_LICENSE_QUOTA_EXCEEDED
		return ERROR_LICENSE_QUOTA_EXCEEDED;
	case 0x001f: // VCI_E_INVALID_TIMING
		return ERROR_INVALID_PARAMETER;
	case 0x0020: // VCI_E_IN_USE
		return ERROR_DEVICE_IN_USE;
	case 0x0021: // VCI_E_NO_SUCH_DEVICE
		return ERROR_DEV_NOT_EXIST;
	case 0x0022: // VCI_E_DEVICE_NOT_CONNECTED
		return ERROR_NOT_CONNECTED;
	case 0x0023: // VCI_E_DEVICE_NOT_READY
		return ERROR_NOT_READY;
	case 0x0024: // VCI_E_TYPE_MISMATCH
		return ERROR_BAD_DEV_TYPE;
	case 0x0025: // VCI_E_NOT_SUPPORTED
		return ERROR_NOT_SUPPORTED;
	case 0x0026: // VCI_E_DUPLICATE_OBJECTID
		return ERROR_ALREADY_ASSIGNED;
	case 0x0027: // VCI_E_OBJECTID_NOT_FOUND
		return ERROR_NOT_FOUND;
	case 0x0028: // VCI_E_WRONG_LEVEL
		return ERROR_INVALID_LEVEL;
	case 0x0029: // VCI_E_WRONG_DRV_VERSION
		return PEERDIST_ERROR_VERSION_UNSUPPORTED;
	case 0x002a: // VCI_E_LUIDS_EXHAUSTED
		return ERROR_LUIDS_EXHAUSTED;
	default: return ERROR_UNEXP_NET_ERR;
	}
}

static int
io_ixxat_state(UINT32 dwStatus)
{
	if (dwStatus & CAN_STATUS_BUSOFF)
		return CAN_STATE_BUSOFF;
	if (dwStatus & CAN_STATUS_ERRLIM)
		return CAN_STATE_PASSIVE;
	return CAN_STATE_ACTIVE;
}

static struct timespec
io_ixxat_time(UINT64 qwTime, UINT32 dwTscClkFreq, UINT32 dwTscDivisor)
{
	struct timespec ts = { 0, 0 };
	if (dwTscClkFreq && dwTscDivisor) {
		qwTime *= dwTscDivisor;
		ts.tv_sec = qwTime / dwTscClkFreq;
		qwTime -= ts.tv_sec * dwTscClkFreq;
		ts.tv_nsec = (qwTime * 1000000000ul) / dwTscClkFreq;
	}
	return ts;
}

static int
io_ixxat_ctrl_stop(io_can_ctrl_t *ctrl)
{
	struct io_ixxat_ctrl *ixxat = io_ixxat_ctrl_from_ctrl(ctrl);

	HRESULT hResult = p_canControlStart(ixxat->hCanCtl, FALSE);
	if (hResult != VCI_OK) {
		SetLastError(io_ixxat_error(hResult));
		return -1;
	}
	return 0;
}

static int
io_ixxat_ctrl_stopped(const io_can_ctrl_t *ctrl)
{
	const struct io_ixxat_ctrl *ixxat = io_ixxat_ctrl_from_ctrl(ctrl);

#if LELY_NO_CANFD
	CANLINESTATUS sStatus;
#else
	CANLINESTATUS2 sStatus;
#endif
	HRESULT hResult = p_canControlGetStatus(ixxat->hCanCtl, &sStatus);
	if (hResult != VCI_OK) {
		SetLastError(io_ixxat_error(hResult));
		return -1;
	}
	return !!(sStatus.dwStatus & CAN_STATUS_ININIT);
}

static int
io_ixxat_ctrl_restart(io_can_ctrl_t *ctrl)
{
	struct io_ixxat_ctrl *ixxat = io_ixxat_ctrl_from_ctrl(ctrl);

	HRESULT hResult = p_canControlStart(ixxat->hCanCtl, TRUE);
	if (hResult != VCI_OK) {
		SetLastError(io_ixxat_error(hResult));
		return -1;
	}
	return 0;
}

static int
io_ixxat_ctrl_get_bitrate(const io_can_ctrl_t *ctrl, int *pnominal, int *pdata)
{
	struct io_ixxat_ctrl *ixxat = io_ixxat_ctrl_from_ctrl(ctrl);

#if LELY_NO_CANFD
	CANLINESTATUS sStatus;
#else
	CANLINESTATUS2 sStatus;
#endif
	HRESULT hResult = p_canControlGetStatus(ixxat->hCanCtl, &sStatus);
	if (hResult != VCI_OK) {
		SetLastError(io_ixxat_error(hResult));
		return -1;
	}

#if LELY_NO_CANFD
	if (pnominal) {
		UINT8 bBtr0 = sStatus.bBtReg0;
		UINT8 bBtr1 = sStatus.bBtReg1;
		if (bBtr0 == CAN_BT0_5KB && bBtr1 == CAN_BT1_5KB) {
			*pnominal = 5000;
		} else if (bBtr0 == CAN_BT0_10KB && bBtr1 == CAN_BT1_10KB) {
			*pnominal = 10000;
		} else if (bBtr0 == CAN_BT0_20KB && bBtr1 == CAN_BT1_20KB) {
			*pnominal = 20000;
		} else if (bBtr0 == CAN_BT0_50KB && bBtr1 == CAN_BT1_50KB) {
			*pnominal = 50000;
		} else if (bBtr0 == CAN_BT0_100KB && bBtr1 == CAN_BT1_100KB) {
			*pnominal = 100000;
		} else if (bBtr0 == CAN_BT0_125KB && bBtr1 == CAN_BT1_125KB) {
			*pnominal = 125000;
		} else if (bBtr0 == CAN_BT0_250KB && bBtr1 == CAN_BT1_250KB) {
			*pnominal = 250000;
		} else if (bBtr0 == CAN_BT0_500KB && bBtr1 == CAN_BT1_500KB) {
			*pnominal = 500000;
		} else if (bBtr0 == CAN_BT0_800KB && bBtr1 == CAN_BT1_800KB) {
			*pnominal = 800000;
		} else if (bBtr0 == CAN_BT0_1000KB && bBtr1 == CAN_BT1_1000KB) {
			*pnominal = 1000000;
		} else {
			*pnominal = 0;
		}
	}
	if (pdata)
		*pdata = 0;
#else
	if (pnominal)
		*pnominal = sStatus.sBtpSdr.dwBPS;
	if (pdata)
		*pdata = (sStatus.bExMode & CAN_FEATURE_FASTDATA)
				? sStatus.sBtpFdr.dwBPS
				: 0;
#endif

	return 0;
}

static int
io_ixxat_ctrl_set_bitrate(io_can_ctrl_t *ctrl, int nominal, int data)
{
	struct io_ixxat_ctrl *ixxat = io_ixxat_ctrl_from_ctrl(ctrl);
#if LELY_NO_CANFD
	(void)data;
#endif

	UINT8 bOpMode = CAN_OPMODE_STANDARD | CAN_OPMODE_EXTENDED;
	if (ixxat->flags & IO_CAN_BUS_FLAG_ERR)
		bOpMode |= CAN_OPMODE_ERRFRAME;
#if LELY_NO_CANFD
	UINT8 bBtr0 = 0;
	UINT8 bBtr1 = 0;
	switch (nominal) {
	case 5000:
		bBtr0 = CAN_BT0_5KB;
		bBtr1 = CAN_BT1_5KB;
		break;
	case 10000:
		bBtr0 = CAN_BT0_10KB;
		bBtr1 = CAN_BT1_10KB;
		break;
	case 20000:
		bBtr0 = CAN_BT0_20KB;
		bBtr1 = CAN_BT1_20KB;
		break;
	case 50000:
		bBtr0 = CAN_BT0_50KB;
		bBtr1 = CAN_BT1_50KB;
		break;
	case 100000:
		bBtr0 = CAN_BT0_100KB;
		bBtr1 = CAN_BT1_100KB;
		break;
	case 125000:
		bBtr0 = CAN_BT0_125KB;
		bBtr1 = CAN_BT1_125KB;
		break;
	case 250000:
		bBtr0 = CAN_BT0_250KB;
		bBtr1 = CAN_BT1_250KB;
		break;
	case 500000:
		bBtr0 = CAN_BT0_500KB;
		bBtr1 = CAN_BT1_500KB;
		break;
	case 800000:
		bBtr0 = CAN_BT0_800KB;
		bBtr1 = CAN_BT1_800KB;
		break;
	case 1000000:
		bBtr0 = CAN_BT0_1000KB;
		bBtr1 = CAN_BT1_1000KB;
		break;
	default: SetLastError(ERROR_INVALID_PARAMETER); return -1;
	}
	HRESULT hResult = p_canControlInitialize(
			ixxat->hCanCtl, bOpMode, bBtr0, bBtr1);
#else
	UINT8 bExMode = CAN_EXMODE_DISABLED;
	if (ixxat->flags & IO_CAN_BUS_FLAG_FDF)
		bExMode |= CAN_EXMODE_EXTDATA;
	if (ixxat->flags & IO_CAN_BUS_FLAG_BRS)
		bExMode |= CAN_EXMODE_FASTDATA;
	CANBTP sBtpSDR, sBtpFDR;
	switch (nominal) {
	case 5000: sBtpSDR = (CANBTP)CAN_BTP_5KB; break;
	case 10000: sBtpSDR = (CANBTP)CAN_BTP_10KB; break;
	case 20000: sBtpSDR = (CANBTP)CAN_BTP_20KB; break;
	case 50000: sBtpSDR = (CANBTP)CAN_BTP_50KB; break;
	case 100000: sBtpSDR = (CANBTP)CAN_BTP_100KB; break;
	case 125000: sBtpSDR = (CANBTP)CAN_BTP_125KB; break;
	case 250000: sBtpSDR = (CANBTP)CAN_BTP_250KB; break;
	case 500000: sBtpSDR = (CANBTP)CAN_BTP_500KB; break;
	case 800000: sBtpSDR = (CANBTP)CAN_BTP_800KB; break;
	case 1000000: sBtpSDR = (CANBTP)CAN_BTP_1000KB; break;
	default: SetLastError(ERROR_INVALID_PARAMETER); return -1;
	}
	if (ixxat->flags & IO_CAN_BUS_FLAG_BRS) {
		switch (data) {
		case 5000: sBtpFDR = (CANBTP)CAN_BTP_5KB; break;
		case 10000: sBtpFDR = (CANBTP)CAN_BTP_10KB; break;
		case 20000: sBtpFDR = (CANBTP)CAN_BTP_20KB; break;
		case 50000: sBtpFDR = (CANBTP)CAN_BTP_50KB; break;
		case 100000: sBtpFDR = (CANBTP)CAN_BTP_100KB; break;
		case 125000: sBtpFDR = (CANBTP)CAN_BTP_125KB; break;
		case 250000: sBtpFDR = (CANBTP)CAN_BTP_250KB; break;
		case 500000: sBtpFDR = (CANBTP)CAN_BTP_500KB; break;
		case 800000: sBtpFDR = (CANBTP)CAN_BTP_800KB; break;
		case 1000000: sBtpFDR = (CANBTP)CAN_BTP_1000KB; break;
		default: SetLastError(ERROR_INVALID_PARAMETER); return -1;
		}
	}
	HRESULT hResult = p_canControlInitialize(ixxat->hCanCtl, bOpMode,
			bExMode, CAN_FILTER_PASS, CAN_FILTER_PASS, 0, 0,
			&sBtpSDR,
			(ixxat->flags & IO_CAN_BUS_FLAG_BRS) ? &sBtpFDR : NULL);
#endif
	if (hResult != VCI_OK) {
		SetLastError(io_ixxat_error(hResult));
		return -1;
	}

	return 0;
}

static int
io_ixxat_ctrl_get_state(const io_can_ctrl_t *ctrl)
{
	const struct io_ixxat_ctrl *ixxat = io_ixxat_ctrl_from_ctrl(ctrl);

#if LELY_NO_CANFD
	CANLINESTATUS sStatus;
#else
	CANLINESTATUS2 sStatus;
#endif
	HRESULT hResult = p_canControlGetStatus(ixxat->hCanCtl, &sStatus);
	if (hResult != VCI_OK) {
		SetLastError(io_ixxat_error(hResult));
		return -1;
	}

	return io_ixxat_state(sStatus.dwStatus);
}

static inline struct io_ixxat_ctrl *
io_ixxat_ctrl_from_ctrl(const io_can_ctrl_t *ctrl)
{
	assert(ctrl);

	return structof(ctrl, struct io_ixxat_ctrl, ctrl_vptr);
}

static io_ctx_t *
io_ixxat_chan_dev_get_ctx(const io_dev_t *dev)
{
	const struct io_ixxat_chan *ixxat = io_ixxat_chan_from_dev(dev);

	return ixxat->ctx;
}

static ev_exec_t *
io_ixxat_chan_dev_get_exec(const io_dev_t *dev)
{
	const struct io_ixxat_chan *ixxat = io_ixxat_chan_from_dev(dev);

	return ixxat->exec;
}

static size_t
io_ixxat_chan_dev_cancel(io_dev_t *dev, struct ev_task *task)
{
	struct io_ixxat_chan *ixxat = io_ixxat_chan_from_dev(dev);

	struct sllist read_queue, write_queue;
	sllist_init(&read_queue);
	sllist_init(&write_queue);

	io_ixxat_chan_pop(ixxat, &read_queue, &write_queue, task);

	size_t nread = io_can_chan_read_queue_post(
			&read_queue, -1, ERROR_OPERATION_ABORTED);
	size_t nwrite = io_can_chan_write_queue_post(
			&write_queue, ERROR_OPERATION_ABORTED);
	return nwrite < SIZE_MAX - nread ? nread + nwrite : SIZE_MAX;
}

static size_t
io_ixxat_chan_dev_abort(io_dev_t *dev, struct ev_task *task)
{
	struct io_ixxat_chan *ixxat = io_ixxat_chan_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

	io_ixxat_chan_pop(ixxat, &queue, &queue, task);

	return ev_task_queue_abort(&queue);
}

static io_dev_t *
io_ixxat_chan_get_dev(const io_can_chan_t *chan)
{
	const struct io_ixxat_chan *ixxat = io_ixxat_chan_from_chan(chan);

	return &ixxat->dev_vptr;
}

static int
io_ixxat_chan_get_flags(const io_can_chan_t *chan)
{
	const struct io_ixxat_chan *ixxat = io_ixxat_chan_from_chan(chan);

#if !LELY_NO_THREADS
	EnterCriticalSection((CRITICAL_SECTION *)&ixxat->CriticalSection);
#endif
	int flags = ixxat->flags;
#if !LELY_NO_THREADS
	LeaveCriticalSection((CRITICAL_SECTION *)&ixxat->CriticalSection);
#endif
	return flags;
}

static int
io_ixxat_chan_read(io_can_chan_t *chan, struct can_msg *msg,
		struct can_err *err, struct timespec *tp, int timeout)
{
	struct io_ixxat_chan *ixxat = io_ixxat_chan_from_chan(chan);

	return io_ixxat_chan_read_impl(ixxat, msg, err, tp, timeout);
}

static void
io_ixxat_chan_submit_read(io_can_chan_t *chan, struct io_can_chan_read *read)
{
	struct io_ixxat_chan *ixxat = io_ixxat_chan_from_chan(chan);
	assert(read);
	struct ev_task *task = &read->task;

	if (!task->exec)
		task->exec = ixxat->exec;
	assert(task->exec);
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	EnterCriticalSection(&ixxat->CriticalSection);
#endif
	if (ixxat->shutdown) {
#if !LELY_NO_THREADS
		LeaveCriticalSection(&ixxat->CriticalSection);
#endif
		io_can_chan_read_post(read, -1, ERROR_OPERATION_ABORTED);
	} else {
		int post_read = !ixxat->read_posted
				&& sllist_empty(&ixxat->read_queue);
		sllist_push_back(&ixxat->read_queue, &task->_node);
		if (post_read)
			ixxat->read_posted = 1;
#if !LELY_NO_THREADS
		LeaveCriticalSection(&ixxat->CriticalSection);
#endif
		assert(ixxat->read_task.exec);
		if (post_read)
			ev_exec_post(ixxat->read_task.exec, &ixxat->read_task);
	}
}

static int
io_ixxat_chan_write(io_can_chan_t *chan, const struct can_msg *msg, int timeout)
{
	struct io_ixxat_chan *ixxat = io_ixxat_chan_from_chan(chan);

	return io_ixxat_chan_write_impl(ixxat, msg, timeout);
}

static void
io_ixxat_chan_submit_write(io_can_chan_t *chan, struct io_can_chan_write *write)
{
	struct io_ixxat_chan *ixxat = io_ixxat_chan_from_chan(chan);
	assert(write);
	assert(write->msg);
	struct ev_task *task = &write->task;

#if !LELY_NO_CANFD
	int flags = 0;
	if (write->msg->flags & CAN_FLAG_FDF)
		flags |= IO_CAN_BUS_FLAG_FDF;
	if (write->msg->flags & CAN_FLAG_BRS)
		flags |= IO_CAN_BUS_FLAG_BRS;
#endif

	if (!task->exec)
		task->exec = ixxat->exec;
	assert(task->exec);
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	EnterCriticalSection(&ixxat->CriticalSection);
#endif
	if (ixxat->shutdown) {
#if !LELY_NO_THREADS
		LeaveCriticalSection(&ixxat->CriticalSection);
#endif
		io_can_chan_write_post(write, ERROR_OPERATION_ABORTED);
#if !LELY_NO_CANFD
	} else if ((flags & ixxat->flags) != flags) {
#if !LELY_NO_THREADS
		LeaveCriticalSection(&ixxat->CriticalSection);
#endif
		io_can_chan_write_post(write, ERROR_INVALID_PARAMETER);
#endif
	} else {
		int post_write = !ixxat->write_posted
				&& sllist_empty(&ixxat->write_queue);
		sllist_push_back(&ixxat->write_queue, &task->_node);
		if (post_write)
			ixxat->write_posted = 1;
#if !LELY_NO_THREADS
		LeaveCriticalSection(&ixxat->CriticalSection);
#endif
		assert(ixxat->write_task.exec);
		if (post_write)
			ev_exec_post(ixxat->write_task.exec,
					&ixxat->write_task);
	}
}

static void
io_ixxat_chan_svc_shutdown(struct io_svc *svc)
{
	struct io_ixxat_chan *ixxat = io_ixxat_chan_from_svc(svc);
	io_dev_t *dev = &ixxat->dev_vptr;

#if !LELY_NO_THREADS
	EnterCriticalSection(&ixxat->CriticalSection);
#endif
	int shutdown = !ixxat->shutdown;
	ixxat->shutdown = 1;
	if (shutdown)
		// Try to abort io_ixxat_chan_read_task_func() and
		// io_ixxat_chan_write_task_func().
		io_ixxat_chan_do_abort_tasks(ixxat);
#if !LELY_NO_THREADS
	LeaveCriticalSection(&ixxat->CriticalSection);
#endif

	if (shutdown)
		// Cancel all pending operations.
		io_ixxat_chan_dev_cancel(dev, NULL);
}

static void
io_ixxat_chan_read_task_func(struct ev_task *task)
{
	assert(task);
	struct io_ixxat_chan *ixxat =
			structof(task, struct io_ixxat_chan, read_task);

	DWORD dwErrCode = GetLastError();

	HANDLE hCanChn = NULL;
	struct io_can_chan_read *read = NULL;

#if !LELY_NO_THREADS
	EnterCriticalSection(&ixxat->CriticalSection);
#endif
	if (!ixxat->shutdown)
		hCanChn = ixxat->hCanChn;
	UINT32 dwTscClkFreq = ixxat->dwTscClkFreq;
	UINT32 dwTscDivisor = ixxat->dwTscDivisor;
	UINT64 qwTime = (UINT64)ixxat->dwTimeOvr << 32;
	if ((task = ev_task_from_node(sllist_pop_front(&ixxat->read_queue))))
		read = ixxat->current_read = io_can_chan_read_from_task(task);
#if !LELY_NO_THREADS
	LeaveCriticalSection(&ixxat->CriticalSection);
#endif

	int result = 0;
	int errc = 0;
	UINT32 dwTime = 0;
	UINT32 dwTimeOvr = 0;
	if (read) {
		result = io_ixxat_read(hCanChn, read->msg, read->err, &dwTime,
				&dwTimeOvr, ixxat->rxtimeo);
		if (result >= 0 && read->tp) {
			qwTime += ((UINT64)dwTimeOvr << 32) + dwTime;
			*read->tp = io_ixxat_time(
					qwTime, dwTscClkFreq, dwTscDivisor);
		}
		if (result == -1) {
			errc = GetLastError();
			if (errc == ERROR_TIMEOUT)
				errc = ERROR_RETRY;
		}
	}

#if !LELY_NO_THREADS
	EnterCriticalSection(&ixxat->CriticalSection);
#endif
	if (hCanChn == ixxat->hCanChn)
		ixxat->dwTimeOvr += dwTimeOvr;
	// Put the read operation back on the queue if it would block, unless
	// it was canceled.
	if (result == -1 && errc == ERROR_RETRY
			&& read == ixxat->current_read) {
		assert(read);
		sllist_push_front(&ixxat->read_queue, &task->_node);
		read = NULL;
	}
	ixxat->current_read = NULL;
	int post_read = ixxat->read_posted =
			!sllist_empty(&ixxat->read_queue) && !ixxat->shutdown;
#if !LELY_NO_THREADS
	LeaveCriticalSection(&ixxat->CriticalSection);
#endif
	if (read) {
		if (result == -1 && errc == ERROR_RETRY)
			errc = ERROR_OPERATION_ABORTED;
		io_can_chan_read_post(read, result, errc);
	}

	if (post_read)
		ev_exec_post(ixxat->read_task.exec, &ixxat->read_task);

	SetLastError(dwErrCode);
}

static void
io_ixxat_chan_write_task_func(struct ev_task *task)
{
	assert(task);
	struct io_ixxat_chan *ixxat =
			structof(task, struct io_ixxat_chan, write_task);

	DWORD dwErrCode = GetLastError();

	HANDLE hCanChn = NULL;
	struct io_can_chan_write *write = NULL;

#if !LELY_NO_THREADS
	EnterCriticalSection(&ixxat->CriticalSection);
#endif
	if (!ixxat->shutdown)
		hCanChn = ixxat->hCanChn;
	if ((task = ev_task_from_node(sllist_pop_front(&ixxat->write_queue))))
		write = ixxat->current_write =
				io_can_chan_write_from_task(task);
#if !LELY_NO_THREADS
	LeaveCriticalSection(&ixxat->CriticalSection);
#endif

	int result = 0;
	int errc = 0;
	if (write) {
		result = io_ixxat_write(hCanChn, write->msg, ixxat->txtimeo);
		if (result == -1) {
			errc = GetLastError();
			if (errc == ERROR_TIMEOUT)
				errc = ERROR_RETRY;
		}
	}

#if !LELY_NO_THREADS
	EnterCriticalSection(&ixxat->CriticalSection);
#endif
	// Put the write operation back on the queue if it would block, unless
	// it was canceled.
	if (result == -1 && errc == ERROR_RETRY
			&& write == ixxat->current_write) {
		assert(write);
		sllist_push_front(&ixxat->write_queue, &task->_node);
		write = NULL;
	}
	ixxat->current_write = NULL;
	int post_write = ixxat->write_posted =
			!sllist_empty(&ixxat->write_queue) && !ixxat->shutdown;
#if !LELY_NO_THREADS
	LeaveCriticalSection(&ixxat->CriticalSection);
#endif

	if (write) {
		if (result == -1 && errc == ERROR_RETRY)
			errc = ERROR_OPERATION_ABORTED;
		io_can_chan_write_post(write, errc);
	}

	if (post_write)
		ev_exec_post(ixxat->write_task.exec, &ixxat->write_task);

	SetLastError(dwErrCode);
}

static inline struct io_ixxat_chan *
io_ixxat_chan_from_dev(const io_dev_t *dev)
{
	assert(dev);

	return structof(dev, struct io_ixxat_chan, dev_vptr);
}

static inline struct io_ixxat_chan *
io_ixxat_chan_from_chan(const io_can_chan_t *chan)
{
	assert(chan);

	return structof(chan, struct io_ixxat_chan, chan_vptr);
}

static inline struct io_ixxat_chan *
io_ixxat_chan_from_svc(const struct io_svc *svc)
{
	assert(svc);

	return structof(svc, struct io_ixxat_chan, svc);
}

static int
io_ixxat_chan_read_impl(struct io_ixxat_chan *ixxat, struct can_msg *msg,
		struct can_err *err, struct timespec *tp, int timeout)
{
	assert(ixxat);

#if !LELY_NO_THREADS
	EnterCriticalSection(&ixxat->CriticalSection);
#endif
	HANDLE hCanChn = ixxat->hCanChn;
	UINT32 dwTscClkFreq = ixxat->dwTscClkFreq;
	UINT32 dwTscDivisor = ixxat->dwTscDivisor;
	UINT64 qwTime = (UINT64)ixxat->dwTimeOvr << 32;
#if !LELY_NO_THREADS
	LeaveCriticalSection(&ixxat->CriticalSection);
#endif

	UINT32 dwTime = 0;
	UINT32 dwTimeOvr = 0;
	int result = io_ixxat_read(
			hCanChn, msg, err, &dwTime, &dwTimeOvr, timeout);
	if (result >= 0 && tp) {
		qwTime += ((UINT64)dwTimeOvr << 32) + dwTime;
		*tp = io_ixxat_time(qwTime, dwTscClkFreq, dwTscDivisor);
	}

#if !LELY_NO_THREADS
	EnterCriticalSection(&ixxat->CriticalSection);
#endif
	if (hCanChn == ixxat->hCanChn)
		ixxat->dwTimeOvr += dwTimeOvr;
#if !LELY_NO_THREADS
	LeaveCriticalSection(&ixxat->CriticalSection);
#endif

	return result;
}

static int
io_ixxat_chan_write_impl(struct io_ixxat_chan *ixxat, const struct can_msg *msg,
		int timeout)
{
	assert(ixxat);
	assert(msg);

#if !LELY_NO_CANFD
	int flags = 0;
	if (msg->flags & CAN_FLAG_FDF)
		flags |= IO_CAN_BUS_FLAG_FDF;
	if (msg->flags & CAN_FLAG_BRS)
		flags |= IO_CAN_BUS_FLAG_BRS;
#endif

#if !LELY_NO_THREADS
	EnterCriticalSection(&ixxat->CriticalSection);
#endif
#if !LELY_NO_CANFD
	if ((flags & ixxat->flags) != flags) {
#if !LELY_NO_THREADS
		LeaveCriticalSection(&ixxat->CriticalSection);
#endif
		SetLastError(ERROR_INVALID_PARAMETER);
		return -1;
	}
#endif
	HANDLE hCanChn = ixxat->hCanChn;
#if !LELY_NO_THREADS
	LeaveCriticalSection(&ixxat->CriticalSection);
#endif

	return io_ixxat_write(hCanChn, msg, timeout);
}

static void
io_ixxat_chan_pop(struct io_ixxat_chan *ixxat, struct sllist *read_queue,
		struct sllist *write_queue, struct ev_task *task)
{
	assert(ixxat);
	assert(read_queue);
	assert(write_queue);

#if !LELY_NO_THREADS
	EnterCriticalSection(&ixxat->CriticalSection);
#endif
	if (!task) {
		sllist_append(read_queue, &ixxat->read_queue);
		sllist_append(write_queue, &ixxat->write_queue);
	} else if (sllist_remove(&ixxat->read_queue, &task->_node)) {
		sllist_push_back(read_queue, &task->_node);
	} else if (sllist_remove(&ixxat->write_queue, &task->_node)) {
		sllist_push_back(write_queue, &task->_node);
	}
#if !LELY_NO_THREADS
	LeaveCriticalSection(&ixxat->CriticalSection);
#endif
}

static size_t
io_ixxat_chan_do_abort_tasks(struct io_ixxat_chan *ixxat)
{
	assert(ixxat);

	size_t n = 0;

	// Try to abort io_ixxat_chan_read_task_func().
	// clang-format off
	if (ixxat->read_posted && ev_exec_abort(ixxat->read_task.exec,
			&ixxat->read_task)) {
		// clang-format on
		ixxat->read_posted = 0;
		n++;
	}

	// Try to abort io_ixxat_chan_write_task_func().
	// clang-format off
	if (ixxat->write_posted && ev_exec_abort(ixxat->write_task.exec,
			&ixxat->write_task)) {
		// clang-format on
		ixxat->write_posted = 0;
		n++;
	}

	return n;
}

static HANDLE
io_ixxat_chan_set_handle(struct io_ixxat_chan *ixxat, HANDLE hCanChn, int flags,
		UINT32 dwTscClkFreq, UINT32 dwTscDivisor)
{
	assert(ixxat);
	assert(!(flags & ~IO_CAN_BUS_FLAG_MASK));

	struct sllist read_queue, write_queue;
	sllist_init(&read_queue);
	sllist_init(&write_queue);

#if !LELY_NO_THREADS
	EnterCriticalSection(&ixxat->CriticalSection);
#endif

	HANDLE tmp = ixxat->hCanChn;
	ixxat->hCanChn = hCanChn;
	hCanChn = tmp;

	ixxat->flags = flags;
	ixxat->dwTscClkFreq = dwTscClkFreq;
	ixxat->dwTscDivisor = dwTscDivisor;
	ixxat->dwTimeOvr = 0;

	sllist_append(&read_queue, &ixxat->read_queue);
	sllist_append(&write_queue, &ixxat->write_queue);

	ixxat->current_read = NULL;
	ixxat->current_write = NULL;

#if !LELY_NO_THREADS
	LeaveCriticalSection(&ixxat->CriticalSection);
#endif

	io_can_chan_read_queue_post(&read_queue, -1, ERROR_OPERATION_ABORTED);
	io_can_chan_write_queue_post(&write_queue, ERROR_OPERATION_ABORTED);

	return hCanChn;
}

#endif // !LELY_NO_STDIO && _WIN32 && LELY_HAVE_IXXAT
