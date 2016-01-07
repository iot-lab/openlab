#ifndef GSP_CACHE_H
#define GSP_CACHE_H

#include <stdint.h>
#include <stddef.h>

#define MAX_SEGMENT_SIZE 4
#define CACHE_SIZE 64

/**
 * Data Structures
 */

struct _cache
{
	// This will store the peer
	// that has sent the (last) value to this node
	uint16_t sender;						// 2 bytes
	// This is the issuer of this value
	uint16_t source;						// 2 bytes
	uint8_t empty;
	// The transferred value
	uint8_t value[CACHE_SIZE];
} __attribute__ ((packed));

typedef struct _cache data_cache;

struct _gossip_message
{
	uint8_t type;		// 1 byte
	uint16_t source;	// 2 bytes
	uint8_t value[6];	// 2+4 = 6 bytes => 9 bytes struct size
} __attribute__ ((packed));

typedef struct _gossip_message gossip_message;

struct _cache_segment
{
	uint8_t start;
	uint8_t len;
	uint8_t* value;
} __attribute__ ((packed));

typedef struct _cache_segment cache_segment;

/**
 * Methods
 */

void init_cache(uint16_t me);

uint8_t cache_empty();

size_t cache_memory_footprint();

void set_cache(void* data, size_t len, uint16_t sender, uint16_t source);

uint8_t* get_cache_value();

data_cache* get_cache();

//void send_cache(uint16_t addr, uint8_t type, data_cache *cp);

void set_cache_segment(cache_segment* data, uint16_t sender, uint16_t source);

uint8_t send_cache_segment(uint16_t addr, uint8_t type, cache_segment* cseg);

#endif