#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>

#define USE_WDT

/* Turn on/off debug features */
#define DEBUG

/* Frequency of the external crystal */
#define CLK_HZ 2000000

typedef struct {
	int16_t offset_x, offset_y;
	int16_t height;
	int16_t samples;
	int16_t temperature_calib;
} calibration_t;


typedef struct {
	uint16_t vx1;
	uint16_t vx2;
	uint16_t vy1;
	uint16_t vy2;
} raw_measurements_t;


typedef struct {
	int16_t x;
	int16_t y;
	uint16_t intensity;
} position_measurement_t;


typedef struct {
	int16_t x, y, z;
	uint16_t intensity;
} vector_measurement_t;

typedef struct {
	int16_t ax;
	int16_t ay;
	uint16_t intensity;
} angle_measurement_t;


extern raw_measurements_t raw;
extern position_measurement_t position;
extern vector_measurement_t vector;
extern angle_measurement_t angles;
extern int sleep_mode;


#pragma SET_DATA_SECTION(".fram_vars")
extern calibration_t calibration;
extern int calibration_enabled;
extern uint8_t i2c_address;
#pragma SET_DATA_SECTION()


#define OPAMP_ON()  do { P2OUT |=  BIT0; } while(0)
#define OPAMP_OFF() do { P2OUT &= ~BIT0; } while(0)



#ifdef DEBUG

#define LED_ON()    do { P2OUT |=  BIT6; } while(0)
#define LED_OFF()   do { P2OUT &= ~BIT6; } while(0)
#define LED_TOGGLE()do { P2OUT ^=  BIT6; } while(0)

#else
#define LED_ON()
#define LED_OFF()
#define LED_TOGGLE()
#endif




void calculate_position();
void calculate_vectors();
void calculate_angles();

void sleepmode();
void wakeup();
void init_heartbeat_timer();



#endif /* __MAIN_H__ */
