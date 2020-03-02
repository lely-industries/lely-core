#include "test.h"
#include <lely/libc/threads.h>
#include <lely/util/errnum.h>
#include <lely/util/spscring.h>
#include <lely/util/time.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define NUM_OP (1024ul * 1024ul)
#define RING_SIZE 49157
#define BUF_SIZE 32768ul

struct vfd {
	struct spscring ring;
	int nonblock;
	struct vfd_sig {
		mtx_t mtx;
		cnd_t cond;
		size_t nfast;
		size_t nwait;
	} read, write;
	char buf[1];
};

struct vfd *vfd_open(size_t size, int nonblock);
void vfd_close(struct vfd *fd);

ssize_t vfd_read(struct vfd *fd, void *buf, size_t len, int dontwait);
ssize_t vfd_write(struct vfd *fd, const void *buf, size_t len, int dontwait);

static void vfd_signal(struct spscring *ring, void *arg);

int
read_start(void *arg)
{
	struct vfd *fd = arg;
	assert(arg);

	char dst[BUF_SIZE] = { 0 };

	char cmp[BUF_SIZE] = { 0 };
	for (size_t i = 0; i < BUF_SIZE; i++)
		cmp[i] = (char)i;
	(void)cmp;

	unsigned int rand = 1;

	for (size_t i = 0; i < NUM_OP; i++) {
		size_t pos = 0;
		while (pos < BUF_SIZE) {
			rand = rand * 1103515245ul + 12345;
			size_t n = rand % BUF_SIZE;
			if (n > BUF_SIZE - pos)
				n = BUF_SIZE - pos;

			ssize_t r = vfd_read(fd, dst + pos, n, 0);
			if (r > 0)
				pos += r;
		}
		tap_assert(!memcmp(dst, cmp, BUF_SIZE));
		memset(dst, 0, BUF_SIZE);
	}

	return 0;
}

int
write_start(void *arg)
{
	struct vfd *fd = arg;
	assert(arg);

	char src[BUF_SIZE] = { 0 };
	for (size_t i = 0; i < BUF_SIZE; i++)
		src[i] = (char)i;

	unsigned int rand = 1;

	for (size_t i = 0; i < NUM_OP; i++) {
		size_t pos = 0;
		while (pos < BUF_SIZE) {
			rand = rand * 1103515245ul + 12345;
			size_t n = rand % BUF_SIZE;
			if (n > BUF_SIZE - pos)
				n = BUF_SIZE - pos;

			ssize_t r = vfd_write(fd, src + pos, n, 0);
			if (r > 0)
				pos += r;
		}
	}

	return 0;
}

int
main(void)
{
	tap_plan(1);

	struct vfd *fd = vfd_open(RING_SIZE, 0);
	tap_assert(fd);

	struct timespec t1 = { 0, 0 };
	tap_assert(!clock_gettime(CLOCK_MONOTONIC, &t1));

	thrd_t read_thr;
	tap_assert(thrd_create(&read_thr, &read_start, fd) == thrd_success);

	thrd_t write_thr;
	tap_assert(thrd_create(&write_thr, &write_start, fd) == thrd_success);

	tap_assert(thrd_join(write_thr, NULL) == thrd_success);
	tap_assert(thrd_join(read_thr, NULL) == thrd_success);

	struct timespec t2 = { 0, 0 };
	tap_assert(!clock_gettime(CLOCK_MONOTONIC, &t2));

	double kib = NUM_OP * (double)BUF_SIZE / 1024;
	double sec = timespec_diff_nsec(&t2, &t1) * 1e-9;

	tap_diag("r: %zu/%zu (%g KiB/r)", fd->read.nfast - fd->read.nwait,
			fd->read.nwait, kib / fd->read.nfast);
	tap_diag("w: %zu/%zu (%g KiB/w)", fd->write.nfast - fd->write.nwait,
			fd->write.nwait, kib / fd->write.nfast);
	tap_diag("%g GiB/s", kib / sec / 1024 / 1024);

	vfd_close(fd);

	tap_pass();

	return 0;
}

struct vfd *
vfd_open(size_t size, int nonblock)
{
	struct vfd *fd = malloc(offsetof(struct vfd, buf) + size);
	if (!fd)
		goto error_alloc_fd;

	spscring_init(&fd->ring, size);

	fd->nonblock = nonblock;

	if (mtx_init(&fd->read.mtx, mtx_plain) != thrd_success)
		goto error_init_read_mtx;

	if (cnd_init(&fd->read.cond) != thrd_success)
		goto error_init_read_cond;

	fd->read.nfast = 0;
	fd->read.nwait = 0;

	if (mtx_init(&fd->write.mtx, mtx_plain) != thrd_success)
		goto error_init_write_mtx;

	if (cnd_init(&fd->write.cond) != thrd_success)
		goto error_init_write_cond;

	fd->write.nfast = 0;
	fd->write.nwait = 0;

	return fd;

	// cnd_destroy(&fd->write.cond);
error_init_write_cond:
	mtx_destroy(&fd->write.mtx);
error_init_write_mtx:
	cnd_destroy(&fd->read.cond);
error_init_read_cond:
	mtx_destroy(&fd->read.mtx);
error_init_read_mtx:
	free(fd);
error_alloc_fd:
	return NULL;
}

void
vfd_close(struct vfd *fd)
{
	if (fd) {
		cnd_destroy(&fd->write.cond);
		mtx_destroy(&fd->write.mtx);
		cnd_destroy(&fd->read.cond);
		mtx_destroy(&fd->read.mtx);
		free(fd);
	}
}

ssize_t
vfd_read(struct vfd *fd, void *buf, size_t len, int dontwait)
{
	assert(fd);

	if (!len)
		return 0;

	for (;;) {
		size_t n = len;
		size_t pos = spscring_c_alloc_no_wrap(&fd->ring, &n);
		// If any bytes are available for reading, copy them and return.
		if (n) {
			memcpy(buf, fd->buf + pos, n);
			spscring_c_commit(&fd->ring, n);
			fd->read.nfast++;
			return n;
		} else if (dontwait || fd->nonblock) {
			set_errnum(ERRNUM_AGAIN);
			return -1;
		}
		// Wait for at least 1 byte to become available.
		mtx_lock(&fd->read.mtx);
		// clang-format off
		if (spscring_c_submit_wait(
				&fd->ring, 1, &vfd_signal, &fd->read)) {
			// clang-format on
			fd->read.nwait++;
			cnd_wait(&fd->read.cond, &fd->read.mtx);
		}
		mtx_unlock(&fd->read.mtx);
	}
}

ssize_t
vfd_write(struct vfd *fd, const void *buf, size_t len, int dontwait)
{
	assert(fd);

	if (!len)
		return 0;

	for (;;) {
		size_t n = len;
		size_t pos = spscring_p_alloc_no_wrap(&fd->ring, &n);
		// If any bytes are available for writing, copy them and return.
		if (n) {
			memcpy(fd->buf + pos, buf, n);
			spscring_p_commit(&fd->ring, n);
			fd->write.nfast++;
			return n;
		} else if (dontwait || fd->nonblock) {
			set_errnum(ERRNUM_AGAIN);
			return -1;
		}
		// Wait for at least 1 byte to become available.
		mtx_lock(&fd->write.mtx);
		// clang-format off
		if (spscring_p_submit_wait(
				&fd->ring, 1, &vfd_signal, &fd->write)) {
			// clang-format on
			fd->write.nwait++;
			cnd_wait(&fd->write.cond, &fd->write.mtx);
		}
		mtx_unlock(&fd->write.mtx);
	}
}

static void
vfd_signal(struct spscring *ring, void *arg)
{
	(void)ring;
	struct vfd_sig *sig = arg;
	assert(arg);

	mtx_lock(&sig->mtx);
	cnd_signal(&sig->cond);
	mtx_unlock(&sig->mtx);
}
