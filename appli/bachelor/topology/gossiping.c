/**
 * Gossiping Version 1
 * This version implements the dissemination of
 * a single natural. This will be the maximum.
 */
#include "gossiping.h"
#include "network.h"
#include "feedback.h"

#include "iotlab_uid.h"

#include "random.h"

#define ACTIVE_THREAD 1
#define PASSIVE_THREAD 0

#define CACHE_SIZE 512

uint8_t state = 0;

//char cache[CACHE_SIZE];

typedef struct _cache
{
	// The transferred value
	unsigned int value;						// 4 bytes
	// This will store the peer
	// that has sent the value to this node
	uint16_t sender;						// 2 bytes
	// This is the issuer of this value
	uint16_t source;						// 2 bytes
} data_cache;

data_cache cache;

typedef struct
{
	int begin;
	int end;
} cache_section;

static unsigned int pickPeer(uint32_t numberOfPeers) {
	return random_rand32() % numberOfPeers;
}

/**
 * This method returns a segment of the cache, that can be transmitted
 * It ensures that begin <= end
 * @return [description]
 */
cache_section pick_cache_selection() {
	cache_section a = {0, 0};

	return a;
}

/*
 * This blocks the node until a response of peerId arrives
 */
void block_node_for_response(unsigned int peerId) {
	state = peerId;
}

void start_gossiping() {
	uint16_t me = iotlab_uid();
	// Prefill cache
	cache.value = 0;
	cache.sender = me;
	cache.source = me;
}

/**
 * This method takes care of the actions of an active thread
 * This is issued every dt seconds and makes a data dissemination
 * to one randomly chosen peer.
 */
void active_thread() {
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
	send_package(id, &cache, sizeof(data_cache));

	// wait for receive
	block_node_for_response(id);
}