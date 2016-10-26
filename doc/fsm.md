Finite-state machines
=====================

All services in the CANopen stack are asynchronous and therefore implemented as
event-driven [finite-state machines] (FSMs). Each implementation follows the
pattern shown here. To ensure the maintainability and extensibility of these
state machines, the pattern was designed to have the following properties:

- Adding a new event does not affect existing states;
- Adding a new state is a local change; it does not modify existing functions.

Additionally, the implementation should support parametrized events and
transient states (states which transition to another state without an external
event). The latter greatly simplifies error handling.

The first property excludes implementations based on a [state transition table],
while the second excludes `enum`-based state machines. Additionally, transient
states are hard to implement in either approach.

The implementation shown here is an extension of the function-pointer-based
state machine and is similar to the object-oriented [state design pattern]. The
state machine is an object --- see the documentation of the Lely utilities
library ([liblely-util]) for the general object interface --- containing the
context and a pointer to the current state. Each state is a (constant) array of
pointers to transitions functions, one for each event. The transition functions
return a pointer to the next state, or `NULL` if no transition occurs. There are
two special (optional) transition functions, `on_enter()` and `on_leave()`,
called when a state is entered or left, respectively. A transient state is
simply a state whose `on_enter()` function exists and returns a pointer to
another state.

Example of a public C header (lely/lib/fsm.h):
~~~{.c}
// Check if the FSM is currently in the idle state.
LELY_LIB_EXTERN int fsm_is_idle(const fsm_t *fsm);

// Issue a request to the FSM. This function returns 0 on success, or -1 on
// error (e.g., because the FSM is not in the idle state).
LELY_LIB_EXTERN int fsm_req(fsm_t *fsm, Args... args);
~~~

Example of the C implementation (fsm.c):
~~~{.c}
#include <lely/libc/time.h>
#include <lely/util/errnum.h>

// A state is a constant collection of function pointers.
struct __fsm_state;
typedef const struct __fsm_state fsm_state_t;

struct __fsm {
	...
	// A pointer to the current state.
	fsm_state_t *state;
	...
};

// A helper function for entering a new state.
static void fsm_enter(fsm_t *fsm, fsm_state_t *next);

// A helper function for generating an event and invoking the corresponding
// transition function.
static inline void fsm_emit_event(fsm_t *fsm, Args... args);

// A helper function for generating a timeout event.
static inline void fsm_emit_time(fsm_t *fsm, const struct timespec *tp);

struct __fsm_state {
	// The function called when the state is entered. If this function is
	// specified, its return value is taken to be the next state and a
	// transition will automatically occur.
	fsm_state_t *(*on_enter)(fsm_t *fsm);
	// Events can carry parameters which are passed as arguments to the
	// transition function.
	fsm_state_t *(*on_event)(fsm_t *fsm, Args... args);
	// It is common for services to define a timeout event.
	fsm_state_t *(*on_time)(fsm_t *fsm, const struct timespec *tp);
	// The function called when a state is left. This function cannot return
	// another state, since the next state is already specified.
	void (*on_leave)(fsm_t *fsm);
};

// A macro designed to improve the readability of state definitions.
#define LELY_LIB_DEFINE_STATE(name, ...) \
	static fsm_state_t *const name = &(fsm_state_t){ __VA_ARGS__ };

// The idle state does not contain any transition functions.
LELY_LIB_DEFINE_STATE(fsm_idle_state, NULL)

// The initialization state is transient and only specifies on_enter().
static fsm_state_t *fsm_init_on_enter(fsm_t *fsm);
LELY_LIB_DEFINE_STATE(fsm_init_state,
	.on_enter = &fsm_init_on_enter
)

// The waiting state responds to events and timeouts.
static fsm_state_t *fsm_wait_on_event(fsm_t *fsm, Args... args);
static fsm_state_t *fsm_wait_on_time(fsm_t *fsm, const struct timespec *tp);
LELY_LIB_DEFINE_STATE(fsm_wait_state,
	.on_event = &fsm_wait_on_event,
	.on_time = &fsm_wait_on_time
)

// Like the initialization state, the finalization state is transient.
static fsm_state_t *fsm_fini_on_enter(fsm_t *fsm);
LELY_LIB_DEFINE_STATE(fsm_fini_state,
	.on_enter = &fsm_fini_on_enter
)

#undef LELY_LIB_DEFINE_STATE

struct __fsm *
__fsm_init(struct __fsm *fsm, Args... args)
{
	...

	fsm->state = NULL;
	// Enter the idle state and wait for the user to issue a request.
	fsm_enter(fsm, fsm_idle_state);

	...
}

LELY_LIB_EXPORT int
fsm_is_idle(const fsm_t *fsm)
{
	assert(fsm);

	// States are uniquely identified by their address and can be compared
	// directly.
	return fsm->state == fsm_idle_state;
}

LELY_LIB_EXPORT int
fsm_req(fsm_t *fsm, Args... args)
{
	assert(fsm);

	if (__unlikely(!fsm_is_idle(fsm))) {
		set_errnum(ERRNUM_INPROGRESS)
		return -1;
	}

	// Construct request based on args.
	...

	// Enter the initialization state.
	fsm_enter(fsm, fsm_init_state);
	return 0;
}

static void
fsm_enter(fsm_t *fsm, fsm_state_t *next)
{
	assert(fsm);

	// Use a loop to allow transient state sequences of arbitrary length.
	while (next) {
		// Invoke the on_leave() function if specified.
		if (fsm->state && fsm->state->on_leave)
			fsm->state->on_leave(fsm);
		// Enter the next state and invoke its on_enter() function, if
		// specified. If this function returns another state, enter it
		// in the next iteration.
		fsm->state = next;
		next = next->on_enter ? next->on_enter(fsm) : NULL;
	}
}

static inline void
fsm_emit_event(fsm_t *fsm, Args... args)
{
	assert(fsm);
	assert(fsm->state);
	// We could choose to do nothing if no transition function is specified,
	// but an assertion allows us to detect unexpected events.
	assert(fsm->state->on_event);

	fsm_enter(fsm, fsm->state->on_event(fsm, args...));
}

static inline void
fsm_emit_time(fsm_t *fsm, const struct timespec *tp)
{
	assert(fsm);
	assert(fsm->state);
	assert(fsm->state->on_time);

	fsm_enter(fsm, fsm->state->on_time(fsm, tp));
}

static fsm_state_t *
fsm_init_on_enter(fsm_t *fsm)
{
	assert(fsm);

	// Perform initialization.
	...

	// This is a transient state; it transitions to the waiting state
	// without waiting for an event.
	return fsm_wait_state;
}

static fsm_state_t *
fsm_wait_on_event(fsm_t *fsm, Args... args)
{
	assert(fsm);

	// Process the event.
	...

	// If the event completes the request, enter the finalization state.
	return fsm_fini_state;
}

static fsm_state_t *
fsm_wait_on_time(fsm_t *fsm, const struct timespec *tp)
{
	assert(fsm);

	// A timeout occurred. Abort and clean up.
	...

	// Abort the request by returning to the idle state.
	return fsm_idle_state;
}

static fsm_state_t *
fsm_fini_on_enter(fsm_t *fsm)
{
	assert(fsm);

	// Perform finalization.
	...

	// This is a transient state; it automatically transitions to the idle
	// state.
	return fsm_idle_state;
}
~~~

[finite-state machines]: https://en.wikipedia.org/wiki/Finite-state_machine
[liblely-util]: https://gitlab.com/lely_industries/util
[state design pattern]: https://en.wikipedia.org/wiki/State_pattern
[state transition table]: https://en.wikipedia.org/wiki/State_transition_table

