#include "network.h"
#include <string.h>
#include <stddef.h>
#include "mac_csma.h"
#include "phy.h"
#include "event.h"

#include "feedback.h"

// Type of the broadcast HELLO Message
#define MSG_HELLO 	  1

#define ADDR_BROADCAST 0xFFFF

#define CHANNEL 15

// The maximum package size is available in
// the constant PHY_MAX_TX_LENGTH.
// This constant does not take the CSMA Mac Layer
// into account thus we subtract that size it adds to a package
#define MAXPKGSIZE PHY_MAX_TX_LENGTH - 4

// Define an upper limit to the number of neighbours
// it is desirable that all neighbours fit into one package
// thus we make this number dependent on the MAXPKGSIZE
// we add two bytes to transmit a message type
// One adress is of type uint16_t
#define MAXNEIGHBOURS ((MAXPKGSIZE - 2)/ sizeof(uint16_t))

// Define a minimum signal quality
#define RSSI_THRESHOLD  -70

// How often the algorithm will try to spread the information
#define NUM_TRIES 5

// define the array of all neighbours and initialize it with a 0 first-element
// to allocate the memory
uint16_t neighbours[MAXNEIGHBOURS] = {0};
uint32_t neighboursCount = 0;

struct msg_send
{
    int try;
    uint16_t addr;
    uint8_t pkt[MAXPKGSIZE];
    size_t length;
};


static void do_send(handler_arg_t arg)
{
    struct msg_send *send_cfg = (struct msg_send *)arg;
    int ret;

    ret = mac_csma_data_send(send_cfg->addr, send_cfg->pkt, send_cfg->length);
    if (ret != 0) {
        MESSAGE("Sent to %04x try %u\n", send_cfg->addr, send_cfg->try);
    } else {
        ERROR("Send to %04x failed, try %u. Retrying\n",
                send_cfg->addr, send_cfg->try);
        if (send_cfg->try++ < NUM_TRIES)
            event_post(EVENT_QUEUE_APPLI, do_send, arg);
        //soft_timer_ms_to_ticks(20);
    }

}

static void send(uint16_t addr, void *packet, size_t length)
{
    static struct msg_send send_cfg;
    send_cfg.try = 0;
    send_cfg.addr = addr;
    send_cfg.length = length;
    memcpy(&send_cfg.pkt, packet, length);

    event_post(EVENT_QUEUE_APPLI, do_send, &send_cfg);
}

void lookup_neighbours(uint32_t channel, uint32_t transmission_power) {
	// Remove any existing neighbours
	memset(neighbours, 0, sizeof(neighbours));
	neighboursCount = 0;

	// Initialize radio communication for lookup
	mac_csma_init(channel, transmission_power);

	// Procedure:
	// 1. Each node broadcasts a HELLO-message
	// 2. Upon reception of a broadcast message,
	// register the sender node as a neighbour
	// This assumes a symmetric wireless link.
	// i.e. that if (u,v) \in E => (v,u) \in E

	// 1.
	// Prepare message
	uint8_t pkg = MSG_HELLO;
	//printf("Send HELLO message!\n");
	//mac_csma_data_send(ADDR_BROADCAST, &pkg, 1);
	send(ADDR_BROADCAST, &pkg, 1);
}

// This method prints all currently known neighbours
// Attention: This is not protected against race conditions
// So use at your own risk!
void print_neighbours() {
	int i;
	for (i = 0; i < neighboursCount; i++) {
		MESSAGE("NGB;%04x;\n", neighbours[i]);
	}
}

static void handleReceivedHello(uint16_t src_addr, int8_t rssi) {
	// Check if received rssi above threshold
	// thus the signal strength is high enough
	// printf("%04x: REC HELLO;%d;%d\n", iotlab_uid(), src_addr, rssi);
	if (rssi > RSSI_THRESHOLD) {
		// We assume, that we will not loose any nodes
		// i.e. all nodes will be running until the end of the experiment
		// As we are filling up the list of nodes from the left,
		// we can conclude, that as soon as we find the first 0 value,
		// we know, that we did not know about the node before.
		int i = 0;
		for (i = 0; i < MAXNEIGHBOURS; ++i)
		{
			if (neighbours[i] == 0) {
				MESSAGE("ADD;%04x;\n", src_addr);
				//printf("%04x: ADD: %d\n", iotlab_uid(), src_addr);
				neighbours[i] = src_addr;
				neighboursCount++;
				break;
			}

			// We have registered the node already!
			if (neighbours[i] == src_addr) {
				break;
			}
		}
	} else {
		MESSAGE("REJECT;%04x;%d\n", src_addr, rssi);
	}
}

// Message reception handler
void mac_csma_data_received(uint16_t src_addr, const uint8_t *data,
				     uint8_t length, int8_t rssi, uint8_t lqi) {
	// The first byte of the received message is always the message type.
	// Thus fetch it and look at it!

	uint8_t type = data[0];

	//printf("Message received!\n");

	switch (type) {
		case (MSG_HELLO):
			handleReceivedHello(src_addr, rssi);
			break;
		default:
			break;
	}
}