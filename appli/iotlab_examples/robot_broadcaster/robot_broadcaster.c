/*
 * Robot Broadcaster : radio_broadcaster (to be worn by robot)
 *
 */

#include <platform.h>
#include <printf.h>
#include <string.h>
#include <math.h>

#include "event.h"
#include "radio.h"
#include <phy.h>
#include "soft_timer.h"
#include "lsm303dlhc.h"
#include "l3g4200d.h"
#include "lps331ap.h"
#include "isl29020.h"


// IMU
//
// timer alarm function
static void alarm(handler_arg_t arg);
/* Period of the sensor acquisition datas */
#define ACQ_PERIOD soft_timer_ms_to_ticks(5)
/* times of computation before transmit a result */
/* period in sec = (ACQ_PERIOD=5 x TX_COMPUTE) / 1000 */
#define TX_PERIOD 200

//event handler
static void handle_ev();

#define ACC_RES (1e-3)    // The resolution is 1 mg for the +/-2g scale
#define GYR_RES (8.75e-3) // The resolution is 8.75mdps for the +/-250dps scale
#define CALIB_PERIOD 100  // period in sec = CALIB_PERIOD=5 x TX_COMPUTE) / 1000

char send_buffer[125 - 9];  // minus size taken by radio_send aditional data

/** Global counters structure */
typedef struct TypCounters {
    /* index incremented at each criteria computation */
    uint32_t index;
    /* local index incremented at each computation between 2 packet sending*/
    uint16_t lindex;
} TypCounters;

TypCounters glob_counters = {0, 0};

// Functions
static void receive_callback(char *from, char* data, int rssi, int lqi)
{
    printf("broadcast;%s;%d;%d;%s\n", from, rssi, lqi, data);
}

static float temperature_sensor()
{
    int16_t raw;
    lps331ap_read_temp(&raw);
    //printf("Chip temperature raw=%x\n", raw);

    float temp = 42.5 + (- raw / 480.0);
    //printf("Chip temperature clean=%f\n", temp);
    return temp;
}

static float light_sensor()
{
    float value = isl29020_read_sample();
    //printf("Luminosity measure: %f lux\n", value);
    return value;
}

static void imu_init()
{
    // Switch off the LEDs
    leds_off(LED_0 | LED_1 | LED_2);
    // LSM303DLHC accelero sensor initialisation
    lsm303dlhc_powerdown();
    lsm303dlhc_acc_config(
            LSM303DLHC_ACC_RATE_400HZ,
            LSM303DLHC_ACC_SCALE_2G,
            LSM303DLHC_ACC_UPDATE_ON_READ);
    // LSM303DLHC magneto sensor initialisation
    lsm303dlhc_mag_config(LSM303DLHC_MAG_RATE_220HZ,
            LSM303DLHC_MAG_SCALE_2_5GAUSS,
            LSM303DLHC_MAG_MODE_CONTINUOUS,
            LSM303DLHC_TEMP_MODE_ON);
    // L3G4200D gyro sensor initialisation
    l3g4200d_powerdown();
    l3g4200d_gyr_config(L3G4200D_200HZ, L3G4200D_250DPS, true);
}

static void handle_ev()
{
    int16_t rawacc[3] = { 0 };
    int16_t rawgyr[3] = { 0 };
    //int16_t rawmag[3];
    int16_t i;
    float acc[3] = { 0 };
    float gyr[3] = { 0 };
    //float mag[3];
    static float pitch;
    int pitch_deg;
    static float biais;
    float temp, light;

    /* Read accelerometers */
    lsm303dlhc_read_acc(rawacc);
    /* Read gyrometers */
    l3g4200d_read_rot_speed(rawgyr);
    /* Read magnetometers */
    //lsm303dlhc_read_mag(rawmag);
    /* Gyrometer pitch biais estimation during CALIB_PERIOD*/
    if (glob_counters.index <= CALIB_PERIOD) {
        /* first index */
        if (glob_counters.index == 0) {
            biais = 0.0;
            pitch = 0.0;
        }
        /* estimation */
        biais += gyr[2];
        /* last index */
        if (glob_counters.index == CALIB_PERIOD) {
            biais = biais / CALIB_PERIOD;
            printf("Clb;%f\n",biais);
        }
    } else { /* After calibration */
        for (i = 0; i < 3; i++) {
            acc[i] = rawacc[i] * ACC_RES;
            gyr[i] = rawgyr[i] * GYR_RES;
            /* TBD mag. calibration */
            //mag[i] = rawmag[i] * 1.0;
        }

        /* Compute pitch value, see ACQ_PERIOD */
        pitch = pitch + (gyr[2] - biais) * 0.005 ;
        pitch_deg = (int) (pitch*180/M_PI);
        pitch_deg = pitch_deg % 360;

        if (glob_counters.lindex == TX_PERIOD) {
            /* Print sensors values to tx buffer. Sensors are:
		- IMU values (accelerometers, gyrometers),
		- temperature 
		- light 
            */
	    /* Acquire temperature */
	    temp = temperature_sensor();
	    /* Acquire light */
            light = light_sensor();

            int ret = snprintf(send_buffer, sizeof(send_buffer),
                    "Acc;%f;%f;%f;Gyr;%f;%f;%f;Temp;%f;Light;%f",
                    acc[0], acc[1], acc[2], gyr[0], gyr[1], gyr[2], temp, light);

	    printf("buffer: %s\n", send_buffer);

            // send only if data is correctly written
            if (ret <= sizeof(send_buffer))
                radio_send(send_buffer);

            glob_counters.lindex=0;
        } else {
            glob_counters.lindex++;
        }
    }
    glob_counters.index++;
}

static void alarm(handler_arg_t arg) {
    handle_ev();
    // Only restart timer after finishing handler
    // When reading light and temperature it takes more time than acq_period
    // around 8-9 ms
    soft_timer_start((soft_timer_t *)arg, ACQ_PERIOD, 0);
}

static void hardware_init()
{
    static soft_timer_t tx_timer;

    platform_init();
    event_init();
    soft_timer_init();

    isl29020_prepare(ISL29020_LIGHT__AMBIENT, ISL29020_RESOLUTION__16bit,
            ISL29020_RANGE__16000lux);
    isl29020_sample_continuous();

    lps331ap_powerdown();
    lps331ap_set_datarate(LPS331AP_P_12_5HZ_T_12_5HZ);


    phy_set_power(platform_phy, PHY_POWER_0dBm);
    radio_init(receive_callback);
    imu_init();

    soft_timer_set_handler(&tx_timer, alarm, &tx_timer);
    // repeat will be done by handler
    soft_timer_start(&tx_timer, ACQ_PERIOD, 0);
}

int main()
{
    hardware_init();
    platform_run();
    return 0;
}

