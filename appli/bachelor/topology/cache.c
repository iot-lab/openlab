#include "cache.h"
#include <string.h>
#include "network.h"

data_cache cache;

void init_cache(uint16_t me) {
	// Prefill cache
	memset(cache.value, 0, CACHE_SIZE);
	//cache.value = {0};
	cache.sender = me;
	cache.source = me;
	cache.empty = 1;
}

uint8_t cache_empty() {
	return cache.empty;
}

size_t cache_memory_footprint() {
	return sizeof(data_cache);
}

void set_cache(void* data, size_t len, uint16_t sender, uint16_t source) {
	memcpy(&cache.value, data, len);
	cache.sender = sender;
	cache.source = source;
	cache.empty = 0;
}

void set_cache_segment(cache_segment* data, uint16_t sender, uint16_t source) {
	cache.sender = sender;
	cache.source = source;
	cache.empty = 0;
	memcpy(&cache.value + data->start, data->value, data->len);
	
}

uint8_t* get_cache_value() {
	return cache.value;
}

data_cache* get_cache() {
	return &cache;
}

/*void send_cache(uint16_t addr, uint8_t type, data_cache *cp) {
	static gossip_message msg;
	msg.type = type;

	msg.value = cp->value;
	//msg.sender = me;
	msg.source = cp->source;

	send_package_uuid(addr, &msg, sizeof(gossip_message));
}*/

uint8_t send_cache_segment(uint16_t addr, uint8_t type, cache_segment* cseg) {
	static gossip_message msg;
	int i;

	msg.type = type;

	// Cache segment is too large!
	if (cseg->len > MAX_SEGMENT_SIZE) {
		return -1;
	}

	msg.source = cache.source;
	msg.value[0] = cseg->start;
	msg.value[1] = cseg->len;

	for (i = 0; i < 4; ++i)
	{
		msg.value[2+i] = cache.value[cseg->start + i];
	}
	//memcpy(&msg.value + 2, cache.value + cseg->start, 4);

	send_package_uuid(addr, &msg, sizeof(gossip_message));

	return 0;
}