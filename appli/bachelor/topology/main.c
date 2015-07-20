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

#define ADDR_BROADCAST 0xFFFF

// UART callback function
static void char_rx(handler_arg_t arg, uint8_t c);
static void handle_cmd(handler_arg_t arg);


/* Reception of a radio message */
/*void mac_csma_data_received(uint16_t src_addr,
        const uint8_t *data, uint8_t length, int8_t rssi, uint8_t lqi)
{
    // disable help message after receiving one packet
    print_help = 0;

    printf("\nradio > ");
    printf("Got packet from %x. Len: %u Rssi: %d: '%s'\n",
            src_addr, length, rssi, (const char*)data);
    handle_cmd((handler_arg_t) '\n');
}*/

/*
 * HELP
 */
static void print_usage()
{
    printf("\n\nTopology setup program\n");
    printf("Type command\n");
    printf("\th:\tprint this help\n");
    printf("\tt:\tinitialize neighbourhood HELLO\n");
}

static void hardware_init()
{
    // Openlab platform init
    platform_init();
    event_init();

    // Initialize serial communication interface (UART)
    uart_set_rx_handler(uart_print, char_rx, NULL);

    // Init csma Radio mac layer
    // mac_csma_init(CHANNEL, RADIO_POWER);

}

static void begin_lookup()
{
    lookup_neighbours(15, PHY_POWER_m17dBm);
}

static void handle_cmd(handler_arg_t arg)
{
    switch ((char) (uint32_t) arg) {
        case 't':
            begin_lookup();
            break;
        case '\n':
            printf("\ncmd > ");
            break;
        case 'h':
        default:
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
    // disable help message after receiving char
    event_post_from_isr(EVENT_QUEUE_APPLI, handle_cmd,
            (handler_arg_t)(uint32_t) c);
}