#include <platform.h>
#include <stdint.h>
#include <stdlib.h>
#include <printf.h>
#include <string.h>
#include <time.h>

#include "phy.h"
#include "soft_timer.h"
#include "event.h"

#ifdef IOTLAB_M3
#include "lps331ap.h"
#include "isl29020.h"
#endif
#include "iotlab_uid.h"
#include "mac_csma.h"
#include "phy.h"
#include "iotlab_i2c.h"

#include "iotlab_uid_num_hashtable.h"

// choose channel in [11-26]
#define CHANNEL 11
#define RADIO_POWER PHY_POWER_0dBm

#define ADDR_BROADCAST 0xFFFF

// UART callback function
static void char_rx(handler_arg_t arg, uint8_t c);
static void handle_cmd(handler_arg_t arg);

// timer alarm function
static void alarm(handler_arg_t arg);
static soft_timer_t tx_timer;
#define BLINK_PERIOD soft_timer_s_to_ticks(1)

/* Global variables */
// print help every second
volatile int8_t print_help  = 1;
volatile int8_t leds_active = 1;


#ifdef IOTLAB_M3
/**
 * Sensors
 */
static void temperature_sensor()
{
    int16_t value;
    lps331ap_read_temp(&value);
    printf("Chip temperature measure: %f\n", 42.5 + value / 480.0);
}


static void light_sensor()
{
    float value = isl29020_read_sample();
    printf("Luminosity measure: %f lux\n", value);
}


static void pressure_sensor()
{
    uint32_t value;
    lps331ap_read_pres(&value);
    printf("Pressure measure: %f mabar\n", value / 4096.0);
}
#endif


/**
 * Node UID
 */
static void print_node_uid()
{
    uint16_t node_uid = iotlab_uid();
    struct node node = node_from_uid(node_uid);
    printf("Current node uid: %04x (%s-%u)\n",
            node_uid, node.type_str, node.num);
}


/**
 * Control Node interaction
 */

static void print_cn_time()
{
    // Query control node time
    struct soft_timer_timeval time;
    if (iotlab_get_time(&time)) {
        printf("Error while getting Control node time\n");
        return;
    }

    time_t timestamp = (time_t)time.tv_sec;
    struct tm *local_time = gmtime(&timestamp);
    char date_str[64];
    strftime(date_str, (sizeof date_str), "%Y-%m-%d %H:%M:%S", local_time);

    printf("Control Node time: %u.%06u. Date is: UTC %s.%06u\n",
            time.tv_sec, time.tv_usec, date_str, time.tv_usec);
}


/*
 * Radio config
 */
static void send_packet()
{
    uint16_t ret;
    static uint8_t num = 0;

    static char packet[PHY_MAX_TX_LENGTH - 4];  // 4 for mac layer
    uint16_t length;
    // max pkt length <= max(cc2420, cc1101)
    snprintf(packet, sizeof(packet), "Hello World!: %u", num++);
    length = 1 + strlen(packet);

    ret = mac_csma_data_send(ADDR_BROADCAST, (uint8_t *)packet, length);

    printf("\nradio > ");
    if (ret != 0)
        printf("Packet sent\n");
    else
        printf("Packet sent failed\n");
}


static void send_big_packet()
{
    uint16_t ret;
    static uint8_t num = 0;

    static char packet[PHY_MAX_TX_LENGTH - 4];  // 4 for mac layer
    static char pluspack[40]="012345678901234567890123456789012345678\0";
    uint16_t length;

    snprintf(packet, sizeof(packet), "Big Hello World!: %u %s",num++, pluspack);
    length = 1 + strlen(packet);

    ret = mac_csma_data_send(ADDR_BROADCAST, (uint8_t *)packet, length);

    printf("\nradio > ");
    if (ret != 0)
        printf("Big packet sent\n");
    else
        printf("Big packet sent failed\n");
}


/* Reception of a radio message */
void mac_csma_data_received(uint16_t src_addr,
        const uint8_t *data, uint8_t length, int8_t rssi, uint8_t lqi)
{
    // disable help message after receiving one packet
    print_help = 0;
    struct node src_node = node_from_uid(src_addr);

    printf("\nradio > ");
    printf("Got packet from %x (%s-%u). Len: %u Rssi: %d: '%s'\n",
            src_addr, src_node.type_str, src_node.num,
            length, rssi, (const char*)data);

    handle_cmd((handler_arg_t) '\n');
}


/**
 * Leds action
 */
static void leds_action()
{
    printf("\nleds > ");
    if (leds_active) {
        // The alarm timer looses the hand
        leds_active = 0;
        // Switch off the LEDs
        leds_off(LED_0 | LED_1 | LED_2);
        printf("off\n");
    } else {
        // The alarm timer takes the hand
        leds_active = 1;
        printf("blinking\n");
    }
}


/*
 * HELP
 */
static void print_usage()
{
    printf("\n\nIoT-LAB Simple Demo program\n");
    printf("Type command\n");
    printf("\th:\tprint this help\n");
#ifdef IOTLAB_M3
    printf("\tt:\ttemperature measure\n");
    printf("\tl:\tluminosity measure\n");
    printf("\tp:\tpressure measure\n");
#endif
    printf("\tu:\tprint node uid\n");
    printf("\td:\tread current date using control_node\n");
    printf("\ts:\tsend a radio packet\n");
    printf("\tb:\tsend a big radio packet\n");
    printf("\te:\ttoggle leds blinking\n");
    if (print_help)
        printf("\n Type Enter to stop printing this help\n");
}


static void hardware_init()
{
    // Openlab platform init
    platform_init();
    event_init();
    soft_timer_init();

    // Switch off the LEDs
    leds_off(LED_0 | LED_1 | LED_2);

    // Uart initialisation
    uart_set_rx_handler(uart_print, char_rx, NULL);

#ifdef IOTLAB_M3
    // ISL29020 light sensor initialisation
    isl29020_prepare(ISL29020_LIGHT__AMBIENT, ISL29020_RESOLUTION__16bit,
            ISL29020_RANGE__16000lux);
    isl29020_sample_continuous();

    // LPS331AP pressure sensor initialisation
    lps331ap_powerdown();
    lps331ap_set_datarate(LPS331AP_P_12_5HZ_T_12_5HZ);
#endif

    // Init csma Radio mac layer
    mac_csma_init(CHANNEL, RADIO_POWER);

    // Init control_node i2c
    iotlab_i2c_init();

    // Initialize a openlab timer
    soft_timer_set_handler(&tx_timer, alarm, NULL);
    soft_timer_start(&tx_timer, BLINK_PERIOD, 1);
}


static void handle_cmd(handler_arg_t arg)
{
    switch ((char) (uint32_t) arg) {
#ifdef IOTLAB_M3
        case 't':
            temperature_sensor();
            break;
        case 'l':
            light_sensor();
            break;
        case 'p':
            pressure_sensor();
            break;
#endif
        case 'u':
            print_node_uid();
            break;
        case 'd':
            print_cn_time();
            break;
        case 's':
            send_packet();
            break;
        case 'b':
            send_big_packet();
            break;
        case 'e':
            leds_action();
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
static void char_rx(handler_arg_t arg, uint8_t c)
{
    // disable help message after receiving char
    print_help = 0;
    event_post_from_isr(EVENT_QUEUE_APPLI, handle_cmd,
            (handler_arg_t)(uint32_t) c);
}


static void alarm(handler_arg_t arg)
{
    if (leds_active)
        leds_toggle(LED_0 | LED_1 | LED_2);

    /* Print help before getting first real \n */
    if (print_help) {
        event_post(EVENT_QUEUE_APPLI, handle_cmd, (handler_arg_t) 'h');
        event_post(EVENT_QUEUE_APPLI, handle_cmd, (handler_arg_t) '\n');
    }
}
