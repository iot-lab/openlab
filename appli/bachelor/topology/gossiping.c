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

static soft_timer_t alarm[1];

uint32_t roundCounter;

static unsigned int pickPeer(uint32_t numberOfPeers) {
	return random_rand32() % numberOfPeers;
}

static void handle_timer(handler_arg_t arg)
{
    // The thread is now active thus execute the active_thread method
    event_post(EVENT_QUEUE_APPLI, active_thread, NULL);
}

/**
 * This method returns a segment of the cache, that can be transmitted
 * It ensures that begin <= end
 * @return [description]
 */
cache_segment prepareMsg() {
	static uint8_t segment_id = 0;
	cache_segment seg;
	uint8_t* cache = get_cache_value();

	
	uint8_t start = (segment_id) * 4;
	uint8_t len = 4;

	seg.start = start;
	seg.len = len;
	seg.value = cache + start;

	// The cache is 64 bytes large
	// a message can hold 4 data bytes.
	// We want to achieve a data alignment of uint32_t so 4 bytes
	// that is to say we can transfer one of these values per package.
	// in 64 bytes we can store 64 / 4 = 16 different byte octets
	segment_id = (segment_id + 1) % (CACHE_SIZE/MAX_SEGMENT_SIZE);

	return seg;
}

/**
 * Initialize this node for gossiping
 * i.e. put the inital values into the gossip-cache
 */
void start_gossiping() {
	uint16_t me = iotlab_uid();

	roundCounter = 0;

	init_cache(me);

	// Delay a random interval to reduce nodes being active at the same time
	soft_timer_delay_ms(random_rand32() % 500);

	// The gossip algorithm states, that a node issues a gossip at a periodic interval
    // that is shared across all nodes thus we are initializing a timer here
    soft_timer_set_handler(alarm, handle_timer, (handler_arg_t) 0);

    // Initialize the active thread timer
    soft_timer_start(alarm, soft_timer_ms_to_ticks(GOSSIP_INTERVAL), 1);

	MESSAGE("INIT;%d;%d;\n", cache_memory_footprint(), sizeof(gossip_message));
}

/**
 * This will make the node infected with a new value
 * @param val [description]
 */
void inject_value(uint32_t val) {
	static int segment_id = 0;
	uint16_t me = iotlab_uid();

	uint8_t start = (segment_id) * 4;
	uint8_t len = 4;

	cache_segment seg;

	seg.start = start;
	seg.len = len;
	seg.value = (uint8_t*) &val;

	set_cache_segment(&seg, me, me);

	//set_cache(&val, sizeof(val), me, me);
	segment_id = (segment_id + 1) % (CACHE_SIZE/MAX_SEGMENT_SIZE);

	MESSAGE("INJECT;%u;\n", val);
	
}

/**
 * This method takes care of the actions of an active thread
 * This is issued every dt seconds and makes a data dissemination
 * to one randomly chosen peer.
 */
void active_thread(handler_arg_t arg) {
	roundCounter++;
	// if the thread has no neighbpurs, we cannot pick one...
	// additionally, if our cache value is 0, we are not infected
	// thus we do not send to reduce load on the network
	if (number_of_neighbours() == 0 || cache_empty()) {
		return;
	}
	// p <- RandomPeer()
	unsigned int id = pickPeer(number_of_neighbours());

	// sigma <- PrepareMsg()
	// Pick a segment of the cache that is to be disseminated
	// in our case the selection is random, but could be of circular order
	cache_segment sigma = prepareMsg();

	// send sigma to id
	// therefore translate begin end into pointer and length
	//send_package(id, cache + sigma.begin, sigma.end - sigma.begin + 1);

	//data_cache* cache = get_cache();
	uint32_t part;

	memcpy(&part, sigma.value, 4);

	if (send_cache_segment(uuid_of_neighbour(id), MSG_PUSH, &sigma) < 0) {
		ERROR("TOO-LARGE-SEGMENT");
	}

	MESSAGE("GOSSIP;%u;%04x;%u;\n", roundCounter, uuid_of_neighbour(id), part);
	
}

void update(uint16_t sender, gossip_message *received_message, cache_segment* cseg) {
	
	//int i = 0;
	//uint8_t updateFlag = 0;

	uint8_t* cache_value = get_cache_value();
	cache_value = cache_value + cseg->start;

	uint32_t myValue;
	uint32_t receivedValue;

	memcpy(&myValue, cache_value, sizeof(uint32_t));
	memcpy(&receivedValue, cseg->value, sizeof(uint32_t));

	/*for (i = 0; i < cseg->len; i++) {
		if (cseg->value[i] != cache_value[i]) {
			updateFlag = 1;
		}
	}*/
	if (/*updateFlag*/myValue < receivedValue) {
		// We received a new maximum, so refresh your cache!
		//cache.value = received_message->value;
		//cache.sender = sender;
		//cache.source = received_message->source;
		/*set_cache(&received_message->value,
					sizeof(received_message->value),
					sender,
					received_message->source
				);*/

		set_cache_segment(cseg, sender, received_message->source);

		/*uint32_t part;

		memcpy(&part, cseg->value, 4);*/

		MESSAGE("NEW-CACHE;%04x;%u;\n", sender, /*part*/ receivedValue);
	}
}

/**
 * This executes the step listed for the "passive thread" and is only invoked,
 * if a message was received.
 */
void passive_thread(uint16_t src_addr, const uint8_t *data, uint8_t length) {
	gossip_message received_message;

	cache_segment cseg;

	memcpy(&received_message, data, length);

	MESSAGE("RECV;%04x;%u;%u;%u;\n", src_addr, length, received_message.type, received_message.value[2]);

	
	cseg.start = received_message.value[0];
	cseg.len = received_message.value[1];
	cseg.value = received_message.value + 2;

	// On PUSH-Message answer with PULL - Message
	if (received_message.type == MSG_PUSH) {
		//send_cache(src_addr, MSG_PULL, get_cache());
		send_cache_segment(src_addr, MSG_PULL, &cseg);
	}

	update(src_addr, &received_message, &cseg);
}

void gossip_csma_data_received(uint16_t src_addr, const uint8_t *data,
				     uint8_t length, int8_t rssi, uint8_t lqi) {
	//MESSAGE("RECV;%u\n", length);
	if (length == sizeof(gossip_message)) {
		passive_thread(src_addr, data, length);
	}
}