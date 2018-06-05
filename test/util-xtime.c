#include <lely/tap/tap.h>
#include <lely/util/time.h>
#include <lely/util/xtime.h>

xtimer_t *timer;
int count;

void __cdecl
test_notify(union sigval value)
{
	xclock_t *clock = value.sival_ptr;
	struct timespec now = { 0, 0 };
	xclock_gettime(clock, &now);
	int overrun = xtimer_getoverrun(timer);
	tap_pass("%ld.%ld (%d)", (long)now.tv_sec, now.tv_nsec, overrun);
	count++;
}

int
main(void)
{
	tap_plan(6);

	xclock_t *clock = xclock_create();
	tap_assert(clock);

	struct timespec now = { 0, 0 };
	xclock_settime(clock, &now);

	struct sigevent ev;
	ev.sigev_notify = SIGEV_THREAD;
	ev.sigev_value.sival_ptr = clock;
	ev.sigev_notify_function = &test_notify;

	timer = xtimer_create(clock, &ev);
	tap_assert(timer);

	struct itimerspec value = { { 1, 0 }, { 1, 0 } };
	tap_test(!xtimer_settime(timer, 0, &value, NULL));

	while (count < 5) {
		timespec_add_msec(&now, 10);
		xclock_settime(clock, &now);
	}

	xtimer_destroy(timer);
	xclock_destroy(clock);

	return 0;
}

