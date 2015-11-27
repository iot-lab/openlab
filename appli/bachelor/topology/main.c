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

// Timer structs
static soft_timer_t alarm[1];

#define ACTIVE_INTERVAL 5

// UART callback function
static void char_rx(handler_arg_t arg, uint8_t c);
static void handle_cmd(handler_arg_t arg);

static uint8_t gossipRunning = 0;

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

static void handle_timer(handler_arg_t arg)
{
    // The thread is now active thus execute the active_thread method
    active_thread();
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
    
    // The gossip algorithm states, that a node issues a gossip at a periodic interval
    // that is shared across all nodes thus we are initializing a timer here
    soft_timer_set_handler(alarm, handle_timer, (handler_arg_t) 0);

}

static void switch_to_gossiping()
{
    if (!gossipRunning) {
        start_gossiping();

        // Initialize the active thread timer
        soft_timer_start(alarm, soft_timer_s_to_ticks(ACTIVE_INTERVAL), 1);

        gossipRunning = 1;

    }
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
            switch_to_gossiping();
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