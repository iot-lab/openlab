#include "cache.h"
#include <string.h>
#include "network.h"

data_cache cache;

void init_cache(uint16_t me) {
	// Prefill cache
	cache.value = 0;
	cache.sender = me;
	cache.source = me;
}

uint8_t cache_empty() {
	return cache.value == 0;
}

size_t cache_memory_footprint() {
	return sizeof(data_cache);
}

void set_cache(void* data, size_t len, uint16_t sender, uint16_t source) {
	memcpy(&cache.value, data, len);
	cache.sender = sender;
	cache.source = source;
}

void* get_cache_value() {
	return &cache.value;
}

data_cache* get_cache() {
	return &cache;
}

void send_cache(uint16_t addr, uint16_t me, uint8_t type, data_cache *cp) {
	static gossip_message msg;
	msg.type = type;

	msg.value = cp->value;
	msg.sender = me;
	msg.source = cp->source;

	send_package_uuid(addr, &msg, sizeof(gossip_message));
}