/*
 * Simple robot
 *
 */

#include <platform.h>
#include <printf.h>
#include <string.h>
#include <math.h>

#include "soft_timer.h"
#include "lsm303dlhc.h"
#include "l3g4200d.h"
#include "event.h"


// timer alarm function
static void alarm(handler_arg_t arg);
static soft_timer_t tx_timer;
/* Period of the sensor acquisition datas */
#define ACQ_PERIOD soft_timer_ms_to_ticks(5)
/* times of computation before transmit a result */
/* period in sec = (ACQ_PERIOD=5 x TX_COMPUTE) / 1000 */
#define TX_PERIOD 200

//event handler
static void handle_ev(handler_arg_t arg);

#define ACC_RES (1e-3)    // The resolution is 1 mg for the +/-2g scale
#define GYR_RES (8.75e-3) // The resolution is 8.75mdps for the +/-250dps scale
#define CALIB_PERIOD 100  // period in sec = CALIB_PERIOD=5 x TX_COMPUTE) / 1000 

/** Global counters structure */
typedef struct TypCounters {
 /* index incremented at each criteria computation */
  uint32_t index;
  /* local index incremented at each computation between 2 packet sending*/
  uint16_t lindex;
} TypCounters;

TypCounters glob_counters = {0, 0};

static void hardware_init()
{
    // Openlab platform init
    platform_init();
    event_init();
    soft_timer_init(); 
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
    // Initialize a openlab timer
    soft_timer_set_handler(&tx_timer, alarm, NULL);
    soft_timer_start(&tx_timer, ACQ_PERIOD, 1);
}

int main()
{
	hardware_init();
	platform_run();
	return 0;
}

static void handle_ev(handler_arg_t arg)
{
  int16_t rawacc[3] = {0};
  int16_t rawmag[3] = {0};;
  int16_t rawgyr[3] = {0};;
  int16_t i;
  float acc[3] = {0};;
  float gyr[3] = {0};;
  float mag[3] = {0};;
  static float pitch;
  int pitch_deg;
  static float biais;


  /* Read accelerometers */ 
  lsm303dlhc_read_acc(rawacc);
  /* Read gyrometers */
  l3g4200d_read_rot_speed(rawgyr);
  /* Read magnetometers */ 
  lsm303dlhc_read_mag(rawmag);
  /* Gyrometer pitch biais estimation during CALIB_PERIOD*/
  if (glob_counters.index <= CALIB_PERIOD) {
    /* first index */
    if (glob_counters.index == 0)
      {
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
  } /* After calibration */
  else {
    for (i=0; i < 3; i++) {
      acc[i] = rawacc[i] * ACC_RES;
      gyr[i] = rawgyr[i] * GYR_RES;
      /* TBD mag. calibration */
      mag[i] = rawmag[i] * 1.0;
    }

    /* Compute pitch value, see ACQ_PERIOD */
    pitch = pitch + (gyr[2] - biais) * 0.005 ;
    pitch_deg = (int) (pitch*180/M_PI);
    pitch_deg = pitch_deg % 360;

    if (glob_counters.lindex == TX_PERIOD) {
      /* Print IMU values : accelerometers, gyrometers and magnetometers */
      printf("Acc;%f;%f;%f\n", acc[0], acc[1], acc[2]);
      printf("Gyr;%f;%f;%f\n", gyr[0], gyr[1], gyr[2]);
      printf("Mag;%f;%f;%f\n", mag[0], mag[1], mag[2]);
      /* Print pitch angle */
      printf("Ang;%d\n", pitch_deg);
      //printf("DBG = %f %f %f\n",biais, pitch,(gyr[2] - biais) * 0.005 );
      glob_counters.lindex=0;
    }
    else {
      glob_counters.lindex++;
    }
  }
  glob_counters.index++;
}

static void alarm(handler_arg_t arg) {
  event_post_from_isr(EVENT_QUEUE_APPLI, handle_ev, NULL);
}
