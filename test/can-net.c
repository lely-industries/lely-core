#include "test.h"
#include <lely/can/net.h>

#define MSG_ID 0x123

int can_recv(const struct can_msg *msg, void *data);

int
main(void)
{
	tap_plan(8);

	can_net_t *net = can_net_create();
	tap_assert(net);

	can_recv_t *r1 = can_recv_create();
	tap_assert(r1);
	can_recv_set_func(r1, &can_recv, (void *)(uintptr_t)1);

	can_recv_t *r2 = can_recv_create();
	tap_assert(r2);
	can_recv_set_func(r2, &can_recv, (void *)(uintptr_t)2);

	struct can_msg msg = CAN_MSG_INIT;
	msg.id = MSG_ID;

	can_recv_start(r1, net, MSG_ID, 0);
	can_net_recv(net, &msg);

	can_recv_start(r2, net, MSG_ID, 0);
	can_net_recv(net, &msg);

	can_recv_stop(r2);
	can_net_recv(net, &msg);

	can_recv_stop(r1);
	can_net_recv(net, &msg);

	can_recv_start(r1, net, MSG_ID, 0);
	can_net_recv(net, &msg);

	can_recv_start(r2, net, MSG_ID, 0);
	can_net_recv(net, &msg);

	can_recv_stop(r1);
	can_net_recv(net, &msg);

	can_recv_stop(r2);
	can_net_recv(net, &msg);

	can_recv_destroy(r2);
	can_recv_destroy(r1);

	can_net_destroy(net);

	return 0;
}

int
can_recv(const struct can_msg *msg, void *data)
{
	tap_pass("#%d received 0x%03x", (int)(uintptr_t)data, msg->id);

	return 0;
}
