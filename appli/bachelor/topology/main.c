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

#define ADDR_BROADCAST 0xFFFF

// UART callback function
static void char_rx(handler_arg_t arg, uint8_t c);
static void handle_cmd(handler_arg_t arg);

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

    // Init csma Radio mac layer
    // mac_csma_init(CHANNEL, RADIO_POWER);

}

static void begin_lookup()
{
    //printf("Beginning lookup!\n");
    lookup_neighbours(15, PHY_POWER_0dBm);
}

static void handle_cmd(handler_arg_t arg)
{
    switch ((char) (uint32_t) arg) {
        case 't':
            begin_lookup();
            break;
        case 'l':
            print_neighbours();
            break;
        case 'g':
            start_gossiping();
        case 'h':
            print_usage();
            break;
    }
}

int main()
{
    hardware_init();
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