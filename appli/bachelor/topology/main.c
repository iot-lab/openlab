#include <platform.h>
#include <stdint.h>
#include <stdlib.h>
#include <printf.h>
#include <string.h>

#include "phy.h"
#include "soft_timer.h"
#include "event.h"

#include "mac_csma.h"
#include "phy.h"

#include "network.h"
#include "gossiping.h"

// UART callback function
static void char_rx(handler_arg_t arg, uint8_t c);
static void handle_cmd(handler_arg_t arg);

static uint8_t gossip_running = 0;

uint8_t reading_number = 0;
uint32_t num_cache;

/*
 * HELP
 */
static void print_usage()
{
    printf("\n\nTopology setup program 2\n");
    printf("Type command\n");
    printf("\th:\tprint this help\n");
    printf("\tt:\tinitialize neighbourhood HELLO\n");
}

static void hardware_init()
{
    // Openlab platform init
    platform_init();
    event_init();
    soft_timer_init();

    // Initialize serial communication interface (UART)
    uart_set_rx_handler(uart_print, char_rx, NULL);

}

static void switch_to_gossiping()
{
    if (!gossip_running) {
        start_gossiping();

        gossip_running = 1;

    }
}

static void begin_lookup()
{
    //printf("Beginning lookup!\n");
    lookup_neighbours();
}

static void handle_cmd(handler_arg_t arg)
{
    char read = (char) (uint32_t) arg;

    if (reading_number) {
        if (read >= '0' && read <= '9') {
            num_cache = num_cache*10 + (read - '0');
        } else {
            reading_number = 0;
            inject_value(num_cache);
            num_cache = 0;
        }
    } else {
        switch (read) {
            case 't':               // Start search for neighbours
                begin_lookup();
                break;
            case 'l':               // List all neighbours
                print_neighbours();
                break;
            case 'g':               // Start Gossiping
                switch_to_gossiping();
                break;
            case 'i':               // Read cache value from UART
                reading_number = 1;
                break;
            case 'h':               // Print node help
                print_usage();
                break;
        }
    }
}

int main()
{
    hardware_init();
    reset_neighbours(15, PHY_POWER_5dBm);
    platform_run();

    return 0;
}


/* Reception of a char on UART and store it in 'cmd' */
static void char_rx(handler_arg_t arg, uint8_t c) {
    // This is an Interupt service routine
    // => we should return as soon as possible!
    // Thus we push this as an event into the event queue
    event_post_from_isr(EVENT_QUEUE_APPLI, handle_cmd,
            (handler_arg_t)(uint32_t) c);
}

void mac_csma_data_received(uint16_t src_addr, const uint8_t *data,
                     uint8_t length, int8_t rssi, uint8_t lqi) {

    if (!gossip_running) {
        network_csma_data_received(src_addr, data, length, rssi, lqi);
    } else {
        gossip_csma_data_received(src_addr, data, length, rssi, lqi);
    }
}