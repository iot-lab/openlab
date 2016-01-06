#ifndef GSP_GOSSIP_H
#define GSP_GOSSIP_H

#include "network.h"
#include "feedback.h"
#include "cache.h"

/**
 * Constants
 */

#define MSG_PUSH 0
#define MSG_PULL 1

#define ACTIVE_THREAD 1
#define PASSIVE_THREAD 0

#define GOSSIP_INTERVAL 5

/**
 * Data Structures
 */

/**
 * Functions
 */

void start_gossiping();
void active_thread();
void inject_value(uint32_t val);

void gossip_csma_data_received(uint16_t src_addr, const uint8_t *data,
				     uint8_t length, int8_t rssi, uint8_t lqi);

#endif