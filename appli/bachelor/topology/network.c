#include "network.h"
#include "printf.h"
#include "iotlab_uid.h"
#include "mac_csma.h"
#include "phy.h"

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

// define the array of all neighbours and initialize it with a 0 first-element
// to allocate the memory
uint16_t neighbours[MAXNEIGHBOURS] = {0};
uint32_t neighboursCount = 0;

void lookup_neighbours(uint32_t channel, uint32_t transmission_power) {
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
	mac_csma_data_send(ADDR_BROADCAST, &pkg, 1);
}

static void handleReceivedHello(uint16_t src_addr, int8_t rssi) {
	// Check if received rssi above threshold
	// thus the signal strength is high enough
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
				printf("%04x: ADD: %d\n", iotlab_uid(), src_addr);
				neighbours[i] = src_addr;
				neighboursCount++;
				break;
			}

			// We have registered the node already!
			if (neighbours[i] == src_addr) {
				break;
			}
		}
	}
}

// Message reception handler
void mac_csma_data_received(uint16_t src_addr, const uint8_t *data,
				     uint8_t length, int8_t rssi, uint8_t lqi) {
	// The first byte of the received message is always the message type.
	// Thus fetch it and look at it!

	uint8_t type = data[0];

	switch (type) {
		case (MSG_HELLO):
			handleReceivedHello(src_addr, rssi);
			break;
		default:
			break;
	}
}