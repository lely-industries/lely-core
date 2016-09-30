Overview
========

Contrary to most other CANopen stacks, this implementation is completely
passive; the library does not perform any I/O (besides may reading some files
from disk), it does not create threads nor does it access the system clock.
Instead, it relies on the user to send and receive CAN frames and update the
clock. This allows the library to be easily embedded in a wide variety of
applications.

The library is also asynchronous. Issuing a request is always a non-blocking
operation. If the request is confirmed, the API accepts a callback function
which is invoked once the request completes (with success or failure). This
allows the stack to run in a single thread, even when processing dozens of
simultaneous requests (not uncommon for an NMT master).

CAN network object
------------------

The interface between the CANopen stack and the CAN bus (and system clock) is
provided by the CAN network object (`can_net_t`/`CANNet`) from the Lely CAN
library ([liblely-can]). When the CANopen stack needs to send a CAN frame, it
hands it over to a CAN network object, which in turn invokes a user-defined
callback function to write the frame to the bus. Similarly, when a user reads a
CAN frame from the bus, he gives it to a CAN network object, which in turn
distributes it to the registered receivers in the CANopen stack. Additionally,
the user periodically checks the current time and tells the CAN network object,
which then executes the actions for timers that have elapsed.

Sometimes a CANopen application runs on a device which does not have direct
access to a CAN bus. [CiA] 315 defines a protocol for tunneling CAN frames over
wired or Wireless Transmission Media (WTM). The interface for this protocol can
be found in lely/co/wtm.h (#co_wtm_t) and lely/co/wtm.hpp (#lely::COWTM).
Additionally, the `can2udp` tool can be run as a daemon or service to act as a
proxy between a CAN bus and UDP.

The first task when writing a CANopen application is connecting a CAN network
object to the CAN bus. The following example shows how to do this using the Lely
I/O library ([liblely-io]), which provides a platform-independent interface to
the CAN bus.

C example:
```{.c}
#include <lely/util/errnum.h>
#include <lely/can/net.h>
#include <lely/io/can.h>
#include <lely/io/poll.h>

struct my_can {
	io_handle_t can;
	io_poll_t *poll;
	can_net_t *net;
};

int my_can_init(struct my_can *can, const char *ifname);
void my_can_fini(struct my_can *can);

// This function can be called to perform a single step in an event loop.
void my_can_step(struct my_can *can, int timeout);

int my_can_on_send(const struct can_msg *msg, void *data);

int
my_can_init(struct my_can *can, const char *ifname)
{
	errc_t errc = 0;

	// Open a handle to the CAN bus.
	can->can = io_open_can(ifname);
	if (__unlikely(can->can == IO_HANDLE_ERROR)) {
		errc = get_errc();
		goto error_open_can;
	}

	// Create a new I/O polling interface.
	can->poll = io_poll_create();
	if (__unlikely(!can->poll)) {
		errc = get_errc();
		goto error_create_poll;
	}

	// Create a CAN network object.
	can->net = can_net_create();
	if (__unlikely(!can->net)) {
		errc = get_errc();
		goto error_create_net;
	}

	// Do not block when reading or writing CAN frames and make sure we
	// receive the frames we sent.
	io_set_flags(can->can, IO_FLAG_NONBLOCK | IO_FLAG_LOOPBACK);

	// Watch the CAN bus for incoming frames.
	struct io_event event = {
		.events = IO_EVENT_READ,
		.u.handle = can->can
	};
	io_poll_watch(can->poll, can->can, &event, 1);

	// Obtain the current time.
	struct timespec now = { 0, 0 };
	timespec_get(&now, TIME_UTC);
	// Initialize the CAN network clock.
	can_net_set_time(can->net, &now);

	// Register my_can_on_send() as the function to be invoked when a CAN
	// frame needs to be sent.
	can_net_set_send_func(can->net, &my_can_on_send, can);

	return 0;

	can_net_destroy(can->net);
	can->net = NULL;
error_create_net:
	io_poll_destroy(can->poll);
	can->poll = NULL;
error_create_poll:
	io_handle_release(can->can);
	can->can = IO_HANDLE_ERROR;
error_open_can:
	set_errc(errc);
	return -1;
}

void
my_can_fini(struct my_can *can)
{
	can_net_destroy(can->net);
	can->net = NULL;

	io_poll_destroy(can->poll);
	can->poll = NULL;

	io_handle_release(can->can);
	can->can = IO_HANDLE_ERROR;
}

void
my_can_step(struct my_can *can, int timeout)
{
	// Wait at most `timeout` milliseconds for an I/O event to occur.
	struct io_event event = IO_EVENT_INIT;
	int n = io_poll_wait(can->poll, 1, &event, timeout);

	// Update the CAN network clock.
	struct timespec now = { 0, 0 };
	timespec_get(&now, TIME_UTC);
	can_net_set_time(can->net, &now);

	// Handle the I/O event, if necessary.
	if (n == 1) {
		// If the CAN bus is ready for reading, process all waiting
		// frames.
		if ((event.events & IO_EVENT_READ) && event.u.handle
				== can->can) {
			struct can_msg msg = CAN_MSG_INIT;
			while (io_can_read(can->can, &msg) == 1)
				can_net_recv(can->net, &msg);
		}
	}
}

int
my_can_on_send(const struct can_msg *msg, void *data)
{
	struct my_can *can = data;

	// Send a single frame to the CAN bus.
	return io_can_write(can->can, msg) == 1 ? 0 : -1;
}
```

C++11 example:
```{.cpp}
#include <lely/can/net.hpp>
#include <lely/io/can.hpp>
#include <lely/io/poll.hpp>

using namespace lely;

class MyCAN {
public:
	explicit MyCAN(const char* ifname)
	: m_can(ifname) // Open a handle to the CAN bus.
	, m_poll(new IOPoll) // Create a new I/O polling interface.
	, m_net(new CANNet) // Create a CAN network object.
	{
		// Do not block when reading or writing CAN frames and make sure
		// we receive the frames we sent.
		m_can.setFlags(IO_FLAG_NONBLOCK | IO_FLAG_LOOPBACK);

		// Watch the CAN bus for incoming frames.
		io_event event = { IO_EVENT_READ, { m_can } };
		m_poll->watch(m_can, &event, true);

		// Obtain the current time.
		timespec now = { 0, 0 };
		timespec_get(&now, TIME_UTC);
		// Initialize the CAN network clock.
		m_net->setTime(now);

		// Register the onSend() member function as the function to be
		// invoked when a CAN frame needs to be sent.
		m_net->setSendFunc<MyCAN, &MyCAN::onSend>(this);
	}

	// Disable copying.
	MyCAN(const MyCAN&) = delete;
	MyCAN& operator=(const MyCAN&) = delete;

	CANNet* getNet() const noexcept { return m_net.get(); }

	// This function can be called to perform a single step in an event
	// loop.
	void
	step(int timeout = 0) noexcept
	{
		// Wait at most `timeout` milliseconds for an I/O event to
		// occur.
		io_event event = IO_EVENT_INIT;
		int n = m_poll->wait(1, &event, timeout);

		// Update the CAN network clock.
		timespec now = { 0, 0 };
		timespec_get(&now, TIME_UTC);
		m_net->setTime(now);

		// Handle the I/O event, if necessary.
		if (n == 1) {
			// If the CAN bus is ready for reading, process all
			// waiting frames.
			if ((event.events & IO_EVENT_READ) && event.u.handle
					== m_can) {
				can_msg msg = CAN_MSG_INIT;
				while (m_can.read(msg) == 1)
					m_net->recv(msg);
			}
		}
	}

private:
	int
	onSend(const can_msg* msg) noexcept
	{
		// Send a single frame to the CAN bus.
		return m_can.write(*msg) == 1 ? 0 : -1;
	}

	IOCAN m_can;
	unique_c_ptr<IOPoll> m_poll;
	unique_c_ptr<CANNet> m_net;
};
```

Object dictionary
-----------------

The object dictionary is the central concept of the CANopen protocol. It
contains almost the entire state of a CANopen device, including process data and
communication parameters. Communication between nodes consists primarily of
reading from and writing to each others object dictionary. Even writing CANopen
applications is mostly a matter of configuring the object dictionary.

Together with the Node-ID, the object dictionary is managed by a CANopen device
object (#co_dev_t/#lely::CODev). Although it is possible to construct the object
dictionary from scratch with the C and C++ API, it is more convenient to read it
from an Electronic Data Sheet (EDS) or Device Configuration File (DCF) (see
[CiA] 306). Functions to parse EDS/DCF files can be found in lely/co/dcf.h and
lely/co/dcf.hpp.

Embedded devices often do not have the resources to parse an EDS/DCF file at
runtime. Using the `dcf2c` tool it is possible to create a C file containing a
static object dictionary, which can be compiled with the application. The static
object dictionary can then be converted to a dynamic one at runtime (see
lely/co/sdev.h and lely/co/sdev.hpp).

Service objects
---------------

The CANopen stack implements the following services:
* Process data object (PDO)
  * Receive-PDO (RPDO):
    lely/co/rpdo.h (#co_rpdo_t) and lely/co/rpdo.hpp (#lely::CORPDO)
  * Transmit-PDO (TPDO):
    lely/co/tpdo.h (#co_tpdo_t) and lely/co/tpdo.hpp (#lely::COTPDO)
* Service data object (SDO)
  * Server-SDO (CSDO):
    lely/co/ssdo.h (#co_ssdo_t) and lely/co/ssdo.hpp (#lely::COSSDO)
  * Client-SDO (CSDO):
    lely/co/csdo.h (#co_csdo_t) and lely/co/csdo.hpp (#lely::COCSDO)
* Synchronization object (SYNC):
  lely/co/sync.h (#co_sync_t) and lely/co/sync.hpp (#lely::COSync)
* Time stamp object (TIME):
  lely/co/time.h (#co_time_t) and lely/co/time.hpp (#lely::COTime)
* Emergency object (EMCY):
  lely/co/emcy.h (#co_emcy_t) and lely/co/emcy.hpp (#lely::COEmcy)
* Network management (NMT):
  lely/co/nmt.h (#co_nmt_t) and lely/co/nmt.hpp (#lely::CONMT)
* Layer setting services (LSS):
  lely/co/lss.h (#co_lss_t) and lely/co/lss.hpp (#lely::COLSS)

While it is possible to create these services by hand, it is almost always more
convenient to let them be managed by a single Network Management (NMT) service
object (#co_nmt_t/#lely::CONMT). An NMT object manages the state of a CANopen
device and creates and destroys the other services as needed.

Like all CANopen services, the NMT service gets its configuration from the
object dictionary. A typical CANopen application therefore consists of
* creating the CAN network object, as we have seen above;
* loading the object dictionary form an EDS/DCF file;
* creating an NMT service;
* and, finally, processing CAN frames in an event loop.

Note, however, that a newly created NMT service starts out in the
'Initialisation' state and does not create any services or perform any
communication. This allows the application to register callback functions before
the node becomes operational. The NMT service, including the entire boot-up
sequence, can be started by giving it the RESET NODE command.

The following example is a minimal CANopen application. Depending on the EDS/DCF
file, it can be a simple slave that does nothing besides producing a heartbeat
message, or a full-fledged master managing a network of slaves (including
automatic firmware upgrades!).

C example:
~~~{.c}
#include <lely/util/diag.h>
#include <lely/co/dcf.h>
#include <lely/co/nmt.h>

#include <stdlib.h>

int
main(void)
{
	// Initialize the CAN network.
	struct my_can can;
	if (__unlikely(my_can_init(&can, "can0") == -1)) {
		diag(DIAG_ERROR, get_errc(), "unable to initialize CAN network");
		goto error_init_can;
	}

	// Load the object dictionary from a DCF file.
	co_dev_t *dev = co_dev_create_from_dcf_file("test.dcf");
	if (__unlikely(!dev)) {
		diag(DIAG_ERROR, get_errc(), "unable to load object dictionary");
		goto error_create_dev;
	}

	// Create the NMT service.
	co_nmt_t *nmt = co_nmt_create(can.net, dev);
	if (__unlikely(!nmt)) {
		diag(DIAG_ERROR, get_errc(), "unable to create NMT service");
		goto error_create_nmt;
	}
	// Start the NMT service. We do this by pretending to receive a RESET
	// NODE command from the master.
	co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);

	// The main event loop.
	for (;;) {
		// Process CAN frames in steps of 10 milliseconds.
		my_can_step(&can, 10);
		// TODO: do other useful stuff.
	}

	co_nmt_destroy(nmt);
	co_dev_destroy(dev);
	my_can_fini(&can);

	return EXIT_SUCCESS;

error_create_nmt:
	co_dev_destroy(dev);
error_create_dev:
	my_can_fini(&can);
error_init_can:
	return EXIT_FAILURE;
}
~~~

C++11 example:
~~~{.cpp}
#include <lely/co/dcf.hpp>
#include <lely/co/nmt.hpp>

int
main()
{
	// Initialize the CAN network.
	MyCAN can("can0");

	// Load the object dictionary from a DCF file.
	auto dev = make_unique_c<CODev>("test.dcf");

	// Create the NMT service.
	auto nmt = make_unique_c<CONMT>(can.getNet(), dev.get());
	// Start the NMT service. We do this by pretending to receive a RESET
	// NODE command from the master.
	nmt->csInd(CO_NMT_CS_RESET_NODE);

	// The main event loop.
	for (;;) {
		// Process CAN frames in steps of 10 milliseconds.
		can.step(10);
		// TODO: do other useful stuff.
	}

	return 0;
}
~~~

CANopen requests
----------------

As mentioned before, the CANopen stack is asynchronous. Requests return
immediately, but take a callback function which is invoked once the request
succeeds (or fails).

As an example, define the following callback function for reading a 32-bit
unsigned integer from a remote object dictionary:
~~~{.cpp}
#include <lely/util/diag.h>
#include <lely/co/csdo.hpp>

void
onUp(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, co_unsigned32_t val, void* data)
{
	__unused_var(sdo);
	__unused_var(data);

	if (ac)
		// Print an error message if an abort code was received.
		diag(DIAG_ERROR, 0, "received SDO abort code %08X (%s) when reading object %04X:%02X",
				ac, co_sdo_ac2str(ac), idx, subidx);
	else
		// Print the received value on success.
		diag(DIAG_INFO, 0, "received 0x%08X when reading object %04X:%02X",
				val, idx, subidx);
}
~~~

Before the main event loop we obtain a Client-SDO service object from the NMT
service and issue the request:
~~~{.cpp}
// Wait for the node to be operational.
while (nmt->getSt() != CO_NMT_ST_START)
	can.step(10);

// Obtain the first preconfigured Client-SDO connection.
auto csdo = nmt->getCSDO(1);
if (csdo)
	// Read the device type (object 1000) from the remote object dictionary.
	csdo->upReq<co_unsigned32_t, &onUp>(0x1000, 0x00, nullptr);
~~~

The `upReq()` function returns immediately; `onUp()` is called from the main
event loop when the request succeeds (or fails).

This is an example for a Client-SDO requests, but all services follow this
pattern. Note that by the time the callback function is called, the request has
been completed. It is therefore possible to issue a new request directly from
the callback function. This pattern is used extensively by the master when
booting slaves.

[CiA]: http://can-cia.org/
[liblely-can]: https://gitlab.com/lely_industries/can
[liblely-io]: https://gitlab.com/lely_industries/io

