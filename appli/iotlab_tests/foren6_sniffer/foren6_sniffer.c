#include <string.h>
#include "platform.h"



#define LOG_LEVEL LOG_LEVEL_INFO
#include "debug.h"

/*
 *
 * Copied and adapted from:
 *     https://github.com/cetic/contiki/blob/sniffer/examples/sniffer/sniffer.c
 *
 */

// Dropped all the serial configuration of the channel


#define MAGIC_LEN 4
/*
 * The following defines identify the fields included in the USB packet sent
 * to the Android device
 */
#define FIELD_CRC        1
#define FIELD_CRC_OK     2
#define FIELD_RSSI       4
#define FIELD_LQI        8
#define FIELD_TIMESTAMP 16

/*
 * Packet type: sniffed packet
 * Format:  magic | type | len | pkt | crc_ok | rssi | lqi
 */
#define MY_TYPE  (char) (FIELD_CRC_OK | FIELD_RSSI | FIELD_LQI)


/*
 * The magic sequence for synchronizing the communication from the sniffer to
 * the android device. The magic sequence is sent before sending the actual
 * data to the android device.
 */
/*                              0x53 0x4E 0x49 0x46                          */
//static const uint8_t magic[] = { 'S', 'N', 'I', 'F'};
static void sniff_rx(phy_status_t status);

void init_sniffer(uint8_t channel)
{
    log_info("Foren6 Sniffer with channel: %u", channel);
    phy_set_channel(platform_phy, channel);
    sniff_rx(~0);  // call without a valid status to start rx
}

static void sniff_rx(phy_status_t status)
{
    log_debug("status: %u", status);
    /* magic | type | len | pkt | crc | crc_ok | rssi | lqi | timestamp */
    static uint8_t data[256] = {
        [0] = 'S',
        [1] = 'N',
        [2] = 'I',
        [3] = 'F',
        [4] = MY_TYPE,
    };
    uint8_t index = 5;

    /* Two packets handling */
    static struct {
        phy_packet_t pkt_buf[2];
        int index;
    } sniff = {
        .index = 0,
        .pkt_buf = {
            // static version of phy_preprare_packet
            [0] = { .data = sniff.pkt_buf[0].raw_data },
            [1] = { .data = sniff.pkt_buf[1].raw_data },
        },
    };
    // get current packet and toggle next packet
    phy_packet_t *rx_pkt = &sniff.pkt_buf[sniff.index];
    sniff.index = (sniff.index + 1) % 2;

    log_debug("sniff.index: %u", sniff.index);

    // Enter RX again
    phy_status_t ret;
    ret = phy_rx_now(platform_phy, &sniff.pkt_buf[sniff.index], sniff_rx);
    if (ret != PHY_SUCCESS)
        log_error("PHY RX FAILED");


    /*
     * Handle received packet and dump it to serial line
     */

    if (status != PHY_SUCCESS)
        return;


    /* magic | type | len | pkt | crc | crc_ok | rssi | lqi | timestamp */


    // len
    const uint8_t len   = rx_pkt->length;
    data[index++] = len;
    log_debug("len %u", len);

    // payload
    memcpy(&data[index], rx_pkt->data, len);
    index += len;

    if (MY_TYPE & FIELD_CRC) {
        log_error("CRC not managed");
        index += 2;
    }

    if (MY_TYPE & FIELD_CRC_OK)
        data[index++] = 1;

    if (MY_TYPE & FIELD_RSSI)
        data[index++] = rx_pkt->rssi;
    if (MY_TYPE & FIELD_LQI)
        data[index++] = rx_pkt->lqi;

    if (MY_TYPE & FIELD_TIMESTAMP) {
        uint16_t timestamp = (rx_pkt->timestamp) % 0xFFFF;

        data[index++] = 0xFF & (timestamp >> 8);
        data[index++] = 0xFF & (timestamp);
    }
    log_debug("final len: %u", index);

    uart_transfer(uart_print, data, index);
    log_debug("uart transferted");
}
