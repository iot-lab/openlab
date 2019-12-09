/*
 * Generated from scripts/generate_uid_dict.py
 */

#include <stdint.h>
#include <stddef.h>

#define CC1101 0x1
#define CC2420 0x2
#define M3 0x3
#define A8 0x8

struct node_entry {
    uint16_t uid;
    uint32_t node;
};

struct node {
    uint8_t type;
    uint32_t num;
    char *type_str;
};

extern const struct node_entry nodes_uid_dict[1307];

struct node node_from_uid(uint16_t uid);
