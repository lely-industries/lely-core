/**@file
 * This file is part of the utilities library; it contains the daemon function
 * definitions.
 *
 * @see lely/util/daemon.h
 *
 * @copyright 2017-2020 Lely Industries N.V.
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

#include "util.h"

#if LELY_NO_STDIO
#undef LELY_NO_DAEMON
#define LELY_NO_DAEMON 1
#endif

#if !LELY_NO_DAEMON

#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/util/daemon.h>
#include <lely/util/diag.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef LELY_DAEMON_TIMEOUT
#define LELY_DAEMON_TIMEOUT 1000
#endif

static daemon_handler_t *daemon_handler = &default_daemon_handler;
static void *daemon_handle;

#if _WIN32

#include <winerror.h>

static void WINAPI ServiceMain(DWORD dwArgc, LPSTR *lpszArgv);
static void WINAPI Handler(DWORD fdwControl);
static int ReportStatus(DWORD dwCurrentState);

static const char *daemon_name;
static int (*daemon_init)(int, char **);
static void (*daemon_main)(void);
static void (*daemon_fini)(void);

static SERVICE_STATUS_HANDLE hServiceStatus;

static SERVICE_STATUS ServiceStatus = {
	.dwServiceType = SERVICE_WIN32_OWN_PROCESS,
	.dwControlsAccepted = SERVICE_ACCEPT_STOP
			| SERVICE_ACCEPT_PAUSE_CONTINUE
			| SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PARAMCHANGE,
	.dwWin32ExitCode = NO_ERROR,
	.dwWaitHint = 2 * LELY_DAEMON_TIMEOUT
};

int
daemon_start(const char *name, int (*init)(int, char **), void (*main)(void),
		void (*fini)(void), int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	daemon_name = name;
	daemon_init = init;
	daemon_main = main;
	daemon_fini = fini;

	SERVICE_TABLE_ENTRYA ServiceTable[] = {
		{ (LPSTR)daemon_name, &ServiceMain }, { NULL, NULL }
	};
	return StartServiceCtrlDispatcherA(ServiceTable) ? 0 : -1;
}

int
daemon_signal(int sig)
{
	if (sig < 0 || sig > DAEMON_USER_MAX) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return -1;
	}

	if (daemon_handler)
		daemon_handler(sig, daemon_handle);

	return 0;
}

int
daemon_status(int status)
{
	DWORD dwCurrentState = 0;
	switch (status) {
	case DAEMON_START:
	case DAEMON_CONTINUE: dwCurrentState = SERVICE_RUNNING; break;
	case DAEMON_STOP: dwCurrentState = SERVICE_STOPPED; break;
	case DAEMON_PAUSE: dwCurrentState = SERVICE_PAUSED; break;
	case DAEMON_RELOAD: break;
	default: SetLastError(ERROR_INVALID_PARAMETER); return -1;
	}

	return ReportStatus(dwCurrentState);
}

static void WINAPI
ServiceMain(DWORD dwArgc, LPSTR *lpszArgv)
{
	assert(lpszArgv);

	hServiceStatus = RegisterServiceCtrlHandlerA(daemon_name, Handler);
	if (!hServiceStatus)
		goto error_RegisterServiceCtrlHandlerA;
	ReportStatus(SERVICE_START_PENDING);

	diag_handler_t *diag_handler;
	void *diag_handle;
	diag_get_handler(&diag_handler, &diag_handle);
	diag_set_handler(&daemon_diag_handler, (void *)daemon_name);

	diag_at_handler_t *diag_at_handler;
	void *diag_at_handle;
	diag_at_get_handler(&diag_at_handler, &diag_at_handle);
	diag_at_set_handler(&daemon_diag_at_handler, (void *)daemon_name);

	if (daemon_init) {
		// Make sure the argument list is NULL-terminated.
		char **argv = malloc((dwArgc + 1) * sizeof(char *));
		if (!argv)
			goto error_init;
		for (DWORD i = 0; i < dwArgc; i++)
			argv[i] = lpszArgv[i];
		argv[dwArgc] = NULL;
		if (daemon_init(dwArgc, argv)) {
			free(argv);
			goto error_init;
		}
		free(argv);
	}

	ReportStatus(SERVICE_RUNNING);

	assert(daemon_main);
	daemon_main();

	if (daemon_fini)
		daemon_fini();

	diag_at_set_handler(diag_at_handler, diag_at_handle);
	diag_set_handler(diag_handler, diag_handle);

error_init:
	ReportStatus(SERVICE_STOPPED);
error_RegisterServiceCtrlHandlerA:
	daemon_fini = NULL;
	daemon_main = NULL;
	daemon_init = NULL;
	daemon_name = NULL;
}

static void WINAPI
Handler(DWORD fdwControl)
{
	int sig = -1;
	switch (fdwControl) {
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		ReportStatus(SERVICE_STOP_PENDING);
		sig = DAEMON_STOP;
		break;
	case SERVICE_CONTROL_PAUSE:
		ReportStatus(SERVICE_PAUSE_PENDING);
		sig = DAEMON_PAUSE;
		break;
	case SERVICE_CONTROL_CONTINUE:
		ReportStatus(SERVICE_CONTINUE_PENDING);
		sig = DAEMON_CONTINUE;
		break;
	case SERVICE_CONTROL_PARAMCHANGE: sig = DAEMON_RELOAD; break;
	default:
		if (fdwControl >= 128 && fdwControl <= 255)
			sig = DAEMON_USER_MIN + (fdwControl - 128);
		break;
	};

	if (sig != -1)
		daemon_signal(sig);

	ReportStatus(0);
}

static int
ReportStatus(DWORD dwCurrentState)
{
	static DWORD dwCheckPoint;
	if (dwCurrentState)
		ServiceStatus.dwCurrentState = dwCurrentState;
	switch (ServiceStatus.dwCurrentState) {
	case SERVICE_START_PENDING:
	case SERVICE_STOP_PENDING:
	case SERVICE_PAUSE_PENDING:
	case SERVICE_CONTINUE_PENDING:
		ServiceStatus.dwCheckPoint = ++dwCheckPoint;
		break;
	case SERVICE_STOPPED:
	case SERVICE_RUNNING:
	case SERVICE_PAUSED: ServiceStatus.dwCheckPoint = 0; break;
	default: break;
	}

	return SetServiceStatus(hServiceStatus, &ServiceStatus) ? 0 : -1;
}

#elif _POSIX_C_SOURCE >= 200112L

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

static int daemon_proc(void);
static void daemon_signal_func(int sig);
#if !LELY_NO_THREADS
static int daemon_thrd_start(void *arg);
#endif

static int daemon_pipe[2] = { -1, -1 };

int
daemon_start(const char *name, int (*init)(int, char **), void (*main)(void),
		void (*fini)(void), int argc, char *argv[])
{
	assert(main);
	assert(argc >= 0);
	assert(argv);

	int result = 0;
	int errsv = errno;

	if (init && init(argc, argv)) {
		result = -1;
		errsv = errno;
		goto error_init;
	}

	if (daemon_proc() == -1) {
		result = -1;
		errsv = errno;
		goto error_proc;
	}

	// Create a non-blocking self-pipe.
#if (defined(__CYGWIN__) || defined(__linux__)) && defined(_GNU_SOURCE)
	result = pipe2(daemon_pipe, O_NONBLOCK | O_CLOEXEC);
	if (result == -1) {
		result = -1;
		errsv = errno;
		goto error_pipe;
	}
#else
	result = pipe(daemon_pipe);
	if (result == -1) {
		result = -1;
		errsv = errno;
		goto error_pipe;
	}
	if (fcntl(daemon_pipe[0], F_SETFD, FD_CLOEXEC) == -1) {
		result = -1;
		errsv = errno;
		goto error_fcntl;
	}
	if (fcntl(daemon_pipe[1], F_SETFD, FD_CLOEXEC) == -1) {
		result = -1;
		errsv = errno;
		goto error_fcntl;
	}
	int flags;
	if ((flags = fcntl(daemon_pipe[0], F_GETFL, 0)) == -1) {
		result = -1;
		errsv = errno;
		goto error_fcntl;
	}
	if (fcntl(daemon_pipe[0], F_SETFL, flags | O_NONBLOCK) == -1) {
		result = -1;
		errsv = errno;
		goto error_fcntl;
	}
	if ((flags = fcntl(daemon_pipe[1], F_GETFL, 0)) == -1) {
		result = -1;
		errsv = errno;
		goto error_fcntl;
	}
	if (fcntl(daemon_pipe[1], F_SETFL, flags | O_NONBLOCK) == -1) {
		result = -1;
		errsv = errno;
		goto error_fcntl;
	}
#endif

	// SIGHUP is interpreted as DAEMON_RELOAD.
	struct sigaction new_hup, old_hup;
	new_hup.sa_handler = &daemon_signal_func;
	sigemptyset(&new_hup.sa_mask);
	new_hup.sa_flags = 0;
	if (sigaction(SIGHUP, &new_hup, &old_hup) == -1) {
		result = -1;
		errsv = errno;
		goto error_sighup;
	}

	// SIGTERM is interpreted as DAEMON_STOP.
	struct sigaction new_term, old_term;
	new_term.sa_handler = &daemon_signal_func;
	sigemptyset(&new_term.sa_mask);
	new_term.sa_flags = 0;
	if (sigaction(SIGTERM, &new_term, &old_term) == -1) {
		result = -1;
		errsv = errno;
		goto error_sigterm;
	}

#if !LELY_NO_THREADS
	thrd_t thr;
	if (thrd_create(&thr, &daemon_thrd_start, NULL) != thrd_success) {
		result = -1;
		goto error_thrd_create;
	}
#endif

	diag_handler_t *diag_handler;
	void *diag_handle;
	diag_get_handler(&diag_handler, &diag_handle);
	diag_set_handler(&daemon_diag_handler, (void *)name);

	diag_at_handler_t *diag_at_handler;
	void *diag_at_handle;
	diag_at_get_handler(&diag_at_handler, &diag_at_handle);
	diag_at_set_handler(&daemon_diag_at_handler, (void *)name);

	main();

	daemon_stop();
#if !LELY_NO_THREADS
	thrd_join(thr, NULL);
#endif

	diag_at_set_handler(diag_at_handler, diag_at_handle);
	diag_set_handler(diag_handler, diag_handle);

#if !LELY_NO_THREADS
error_thrd_create:
#endif
	sigaction(SIGTERM, &old_term, NULL);
error_sigterm:
	sigaction(SIGHUP, &old_hup, NULL);
error_sighup:
#if !defined(__CYGWIN__) && !defined(__linux__)
error_fcntl:
#endif
	close(daemon_pipe[1]);
	daemon_pipe[1] = -1;
	close(daemon_pipe[0]);
	daemon_pipe[0] = -1;
error_pipe:
error_proc:
	if (fini)
		fini();
error_init:
	errno = errsv;
	return result;
}

int
daemon_signal(int sig)
{
	if (sig < 0 || sig > DAEMON_USER_MAX) {
		errno = EINVAL;
		return -1;
	}

	int result;
	do
		result = write(daemon_pipe[1], &(unsigned char){ sig }, 1);
	while (result == -1 && errno == EINTR);
	return result;
}

int
daemon_status(int status)
{
	if (status < 0 || status >= DAEMON_USER_MIN) {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static int
daemon_proc(void)
{
	// Fork and exit the parent process to make the orphaned child a child
	// process of init.
	switch (fork()) {
	case 0: break;
	case -1: return -1;
	default: _Exit(EXIT_SUCCESS);
	}

	// Change working directory to a known path.
	if (chdir("/") == -1)
		return -1;

	// Prevent insecure file privileges.
	umask(0);

	// Detach the child process from the parent's tty.
	if (setsid() == -1)
		return -1;

	// Because of the call to setsid(), we are now session leader, which
	// means we can acquire another controlling tty. Prevent this by forking
	// again.
	switch (fork()) {
	case 0: break;
	case -1: return -1;
	default: _Exit(EXIT_SUCCESS);
	}

	// Ignore terminal signals we shouldn't be receiving anyway.
	struct sigaction act;
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGTSTP, &act, NULL);
	sigaction(SIGTTIN, &act, NULL);
	sigaction(SIGTTOU, &act, NULL);

	// Redirect the standard streams to /dev/null. Since the link between
	// file descriptors and streams is implementation-defined, we close and
	// open both, to be on the safe side.

	fsync(STDIN_FILENO);
	fclose(stdin);
	close(STDIN_FILENO);
#if defined(__CYGWIN__) || defined(__linux__)
	// cppcheck-suppress leakReturnValNotUsed
	if (open("/dev/null", O_RDONLY | O_CLOEXEC) != STDIN_FILENO)
		return -1;
#else
	// cppcheck-suppress leakReturnValNotUsed
	if (open("/dev/null", O_RDONLY) != STDIN_FILENO)
		return -1;
	if (fcntl(STDIN_FILENO, F_SETFD, FD_CLOEXEC) == -1)
		return -1;
#endif
	stdin = fdopen(STDIN_FILENO, "r");
	if (!stdin)
		return -1;

	fflush(stdout);
	fsync(STDOUT_FILENO);
	fclose(stdout);
	close(STDOUT_FILENO);
#if defined(__CYGWIN__) || defined(__linux__)
	// cppcheck-suppress leakReturnValNotUsed
	if (open("/dev/null", O_WRONLY | O_CLOEXEC) != STDOUT_FILENO)
		return -1;
#else
	// cppcheck-suppress leakReturnValNotUsed
	if (open("/dev/null", O_WRONLY) != STDOUT_FILENO)
		return -1;
	if (fcntl(STDOUT_FILENO, F_SETFD, FD_CLOEXEC) == -1)
		return -1;
#endif
	stdout = fdopen(STDOUT_FILENO, "w");
	if (!stdout)
		return -1;

	fflush(stderr);
	fsync(STDERR_FILENO);
	fclose(stderr);
	close(STDERR_FILENO);
#if defined(__CYGWIN__) || defined(__linux__)
	// cppcheck-suppress leakReturnValNotUsed
	if (open("/dev/null", O_RDWR | O_CLOEXEC) != STDERR_FILENO)
		return -1;
#else
	// cppcheck-suppress leakReturnValNotUsed
	if (open("/dev/null", O_RDWR) != STDERR_FILENO)
		return -1;
	if (fcntl(STDERR_FILENO, F_SETFD, FD_CLOEXEC) == -1)
		return -1;
#endif
	stderr = fdopen(STDERR_FILENO, "rw");
	if (!stderr)
		return -1;

	return 0;
}

static void
daemon_signal_func(int sig)
{
	switch (sig) {
	case SIGTERM: daemon_stop(); break;
	case SIGHUP: daemon_reload(); break;
	default: break;
	}
}

#if !LELY_NO_THREADS
static int
daemon_thrd_start(void *arg)
{
	(void)arg;

	for (;;) {
		int result;
		// Monitor the read end of the pipe for incoming data.
		struct pollfd fds = { .fd = daemon_pipe[0], .events = POLLIN };
		do
			result = poll(&fds, 1, LELY_DAEMON_TIMEOUT);
		while (result == -1 && errno == EINTR);
		if (result != 1)
			continue;
		// Read a single signal value.
		unsigned char uc = 0;
		do
			result = read(daemon_pipe[0], &uc, 1);
		while (result == -1 && errno == EINTR);
		if (result < 1)
			continue;
		int sig = uc;
		// Execute the signal handler.
		if (daemon_handler)
			daemon_handler(sig, daemon_handle);
		// Exit if we receive the stop signal.
		if (sig == DAEMON_STOP)
			break;
	}

	return 0;
}
#endif

#endif // _WIN32

int
daemon_stop(void)
{
	return daemon_signal(DAEMON_STOP);
}

int
daemon_reload(void)
{
	return daemon_signal(DAEMON_RELOAD);
}

int
daemon_pause(void)
{
	return daemon_signal(DAEMON_PAUSE);
}

int
daemon_continue(void)
{
	return daemon_signal(DAEMON_CONTINUE);
}

void
daemon_get_handler(daemon_handler_t **phandler, void **phandle)
{
	if (phandler)
		*phandler = daemon_handler;
	if (phandle)
		*phandle = daemon_handle;
}

void
daemon_set_handler(daemon_handler_t *handler, void *handle)
{
	daemon_handler = handler;
	daemon_handle = handle;
}

void
default_daemon_handler(int sig, void *handle)
{
	(void)handle;

	switch (sig) {
	case DAEMON_STOP: daemon_status(DAEMON_STOP); break;
	case DAEMON_PAUSE: daemon_status(DAEMON_PAUSE); break;
	case DAEMON_CONTINUE: daemon_status(DAEMON_CONTINUE); break;
	}
}

#endif // !LELY_NO_DAEMON
