#ifndef GSP_NETWORK_H
#define GSP_NETWORK_H

#include <stdint.h>
#include "printf.h"
#include "iotlab_uid.h"

void lookup_neighbours(uint32_t channel, uint32_t transmission_power);

void print_neighbours();

#endif