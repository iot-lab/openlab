#include <time.h>

#include <platform.h>
#include <stdint.h>
#include <stdlib.h>
#include <printf.h>
#include <string.h>

#include "phy.h"
#include "soft_timer.h"
#include "event.h"

#include "iotlab_i2c.h"


// UART callback function
static void char_rx(handler_arg_t arg, uint8_t c);
static void handle_cmd(handler_arg_t arg);

static void get_time()
{
    struct soft_timer_timeval time;
    if (iotlab_get_time(&time))
        printf("Error while getting Control node time\n");
    else
        printf("Control node time: %u.%06u\n", time.tv_sec, time.tv_usec);

    time_t timestamp = (time_t)time.tv_sec;
    struct tm *local_time = gmtime(&timestamp);
    char time_str[64];
    strftime(time_str, (sizeof time_str), "%Y-%m-%d %H:%M:%S", local_time);
    printf("Date: UTC %s.%06u\n", time_str, time.tv_usec);
}

static void get_id()
{
    uint16_t node_id;
    if (iotlab_get_node_id(&node_id))
        printf("Error while getting node_id\n");
    else
        printf("Node id: %04x\n", node_id);
}

static void send_event()
{
    static unsigned i = 0;
    i++;
    if (iotlab_send_event(i))
        printf("Error while sending event\n");
    else
        printf("Sending event value %u\n", i);
}

/*
 * HELP
 */
static void print_usage()
{
    printf("\n\nIoT-LAB Simple Demo program\n");
    printf("Type command\n");
    printf("\th:\tprint this help\n");
    printf("\tt:\tget control node time\n");
    printf("\tn:\tget node id\n");
    printf("\te:\tsend an event to control node\n");
}

static void hardware_init()
{
    // Openlab platform init
    platform_init();
    event_init();
    soft_timer_init();

    iotlab_i2c_init();

    // Switch off the LEDs
    leds_off(LED_0 | LED_1 | LED_2);

    // Uart initialisation
    uart_set_rx_handler(uart_print, char_rx, NULL);

}

static void handle_cmd(handler_arg_t arg)
{
    switch ((char) (uint32_t) arg) {
        case 't':
            get_time();
            break;
        case 'e':
            send_event();
            break;
        case 'n':
            get_id();
            break;
        case '\n':
            printf("\n");
            break;
        case '\r':
            return;
        case 'h':
        default:
            print_usage();
            break;
    }
    printf("cmd > ");
}

int main()
{
    hardware_init();
    platform_run();
    return 0;
}


/* Reception of a char on UART and store it in 'cmd' */
static void char_rx(handler_arg_t arg, uint8_t c) {
    event_post_from_isr(EVENT_QUEUE_APPLI, handle_cmd,
            (handler_arg_t)(uint32_t) c);
}
