#ifndef GSP_NETWORK_H
#define GSP_NETWORK_H

#include <stdint.h>
#include "printf.h"
#include "iotlab_uid.h"

void lookup_neighbours(uint32_t channel, uint32_t transmission_power);

#define MSG(fmt, ...) printf("%04x;" fmt, iotlab_uid(), ##__VA_ARGS__)

#define ERROR(fmt, ...) printf("ERROR:" fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) printf("INFO:" fmt, ##__VA_ARGS__)
#if 0
#define DEBUG(fmt, ...) printf("DEBUG:" fmt, ##__VA_ARGS__)
#else
#define DEBUG(fmt, ...)
#endif

#endif