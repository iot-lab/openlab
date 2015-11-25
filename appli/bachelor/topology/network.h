#ifndef GSP_NETWORK_H
#define GSP_NETWORK_H

#include <stdint.h>
#include <stddef.h>
#include "printf.h"
#include "iotlab_uid.h"

void lookup_neighbours(uint32_t channel, uint32_t transmission_power);

void print_neighbours();

/**
 * Returns the number of neighbours currently registered.
 * As there is no way to reliably detect when the neighbour lookup is finished,
 * there is no security that the number won't change after a call to this method.
 * @return The number of neighbours
 */
uint32_t number_of_neighbours();

/**
 * This returns a pointer to the list of all neighbours.
 * As there is no way to reliably detect when the neighbour lookup is finished,
 * there is no security that the list won't change after a call to this method.
 * 
 * @return Pointer to the neighbour list.
 */
uint16_t* get_neighbours();

/**
 * Send a package to a peer
 * @param peer_id The id of the peer in the neighbours list
 * @param packet  The package itself
 * @param length  The length of the package
 */
void send_package(int peer_id, void *packet, size_t length);

#endif