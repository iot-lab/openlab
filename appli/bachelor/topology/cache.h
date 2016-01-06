#ifndef GSP_CACHE_H
#define GSP_CACHE_H

#include <stdint.h>
#include <stddef.h>

/**
 * Data Structures
 */

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

/**
 * Methods
 */

void init_cache(uint16_t me);

uint8_t cache_empty();

size_t cache_memory_footprint();

void set_cache(void* data, size_t len, uint16_t sender, uint16_t source);

void* get_cache_value();

data_cache* get_cache();

void send_cache(uint16_t addr, uint16_t me, uint8_t type, data_cache *cp);

#endif