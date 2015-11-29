/**
 * Gossiping Version 1
 * This version implements the dissemination of
 * a single natural. This will be the maximum.
 */
#include "gossiping.h"

#include "soft_timer.h"

#include "iotlab_uid.h"

#include "random.h"
#include <string.h>

#define CACHE_SIZE 512

static soft_timer_t alarm[1];

//uint8_t state = 0;

//char cache[CACHE_SIZE];

struct _cache
{
	// The transferred value
	uint32_t value;						// 4 bytes
	// This will store the peer
	// that has sent the value to this node
	uint16_t sender;						// 2 bytes
	// This is the issuer of this value
	uint16_t source;						// 2 bytes
} __attribute__ ((packed));

typedef struct _cache data_cache;

struct _gossip_message
{
	uint8_t type;
	uint32_t value;
	uint16_t sender;
	uint16_t source;
} __attribute__ ((packed));

typedef struct _gossip_message gossip_message;

data_cache cache;
//data_state state = { 0, 0, 0 };

static unsigned int pickPeer(uint32_t numberOfPeers) {
	return random_rand32() % numberOfPeers;
}

static void handle_timer(handler_arg_t arg)
{
    // The thread is now active thus execute the active_thread method
    //active_thread();
    event_post(EVENT_QUEUE_APPLI, active_thread, NULL);
}

/**
 * This method returns a segment of the cache, that can be transmitted
 * It ensures that begin <= end
 * @return [description]
 */
/*cache_section pick_cache_selection() {
	cache_section a = {0, 0};

	return a;
}*/

/**
 * Initialize this node for gossiping
 * i.e. put the inital values into the gossip-cache
 */
void start_gossiping() {
	uint16_t me = iotlab_uid();
	// Prefill cache
	cache.value = 0;
	cache.sender = me;
	cache.source = me;

	// The gossip algorithm states, that a node issues a gossip at a periodic interval
    // that is shared across all nodes thus we are initializing a timer here
    soft_timer_set_handler(alarm, handle_timer, (handler_arg_t) 0);

    // Initialize the active thread timer
    soft_timer_start(alarm, soft_timer_s_to_ticks(GOSSIP_INTERVAL), 1);

	MESSAGE("INIT;%d;%d;\n", sizeof(data_cache), sizeof(gossip_message));
}

/**
 * This will make the node infected with a new value
 * @param val [description]
 */
void inject_value(uint32_t val) {
	cache.value = val;
	cache.sender = iotlab_uid();
	cache.source = iotlab_uid();

	MESSAGE("INJECT;%u;\n", val);
	
}

void send_cache(uint16_t addr, uint8_t type, data_cache *cp) {
	static gossip_message msg;
	msg.type = type;

	msg.value = cp->value;
	msg.sender = iotlab_uid();
	msg.source = cp->source;

	send_package_uuid(addr, &msg, sizeof(gossip_message));
}

/**
 * This method takes care of the actions of an active thread
 * This is issued every dt seconds and makes a data dissemination
 * to one randomly chosen peer.
 */
void active_thread(handler_arg_t arg) {
	// if the thread has no neighbpurs, we cannot pick one...
	// additionally, if our cache value is 0, we are not infected
	// thus we do not send to reduce load on the network
	if (number_of_neighbours() == 0 || cache.value == 0) {
		return;
	}
	// p <- RandomPeer()
	unsigned int id = pickPeer(number_of_neighbours());

	// sigma <- PrepareMsg()
	// This step is unneccessary for this implementation
	// as there is always just one cache object that can be
	// disseminated
	// cache_section sigma = pick_cache_selection();

	// send sigma to id
	// therefore translate begin end into pointer and length
	//send_package(id, cache + sigma.begin, sigma.end - sigma.begin + 1);

	MESSAGE("GOSSIP;%04x;%u;\n", uuid_of_neighbour(id), cache.value);
	send_cache(uuid_of_neighbour(id), MSG_PUSH, &cache);

	// wait for receive
	//block_node_for_response(id);
}

void update(uint16_t sender, gossip_message *received_message) {
	if (received_message->value > cache.value) {
		// We received a new maximum, so refresh your cache!
		cache.value = received_message->value;
		cache.sender = sender;
		cache.source = received_message->source;
		MESSAGE("NEW-CACHE;%04x;%u;\n", sender, cache.value);
	}
}

/**
 * This executes the step listed for the "passive thread" and is only invoked,
 * if a message was received.
 */
void passive_thread(uint16_t src_addr, const uint8_t *data, uint8_t length) {
	gossip_message received_message;

	memcpy(&received_message, data, length);

	MESSAGE("RECV;%04x;%u;%u;%u;\n", src_addr, length, received_message.type, received_message.value);

	// On PUSH-Message answer with PULL - Message
	if (received_message.type == MSG_PUSH) {
		send_cache(src_addr, MSG_PULL, &cache);
	}

	update(src_addr, &received_message);
}

void gossip_csma_data_received(uint16_t src_addr, const uint8_t *data,
				     uint8_t length, int8_t rssi, uint8_t lqi) {
	//uint8_t *p = (uint8_t *)data;
	if (length == sizeof(gossip_message)) {
		passive_thread(src_addr, data, length);
	}
}