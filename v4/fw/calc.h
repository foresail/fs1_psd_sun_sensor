#ifndef CALC_H
#define CALC_H

#include <stdint.h>

typedef struct {
    int16_t offset_x, offset_y;
    int16_t height;
    int16_t samples; // Should be 1 <= 8
    int16_t temperature_bias; // in deciDegC
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

#ifdef CALC_ANGLES

typedef struct {
    int16_t ax;
    int16_t ay;
    uint16_t intensity;
} angle_measurement_t;

#endif

extern raw_measurements_t raw;
extern position_measurement_t position;
extern vector_measurement_t vector;
#ifdef CALC_ANGLES

extern angle_measurement_t angles;

#endif

#ifdef NO_CCS
__attribute__ ((section(".fram_vars")))
extern calibration_t calibration;
#else
#pragma SET_DATA_SECTION(".fram_vars")
extern calibration_t calibration;
#pragma SET_DATA_SECTION()
#endif

// calcculate sun spot postion on sensor
void calculate_position(void);

// calculate sun vector
void calculate_vectors(void);

#ifdef CALC_ANGLES
// This requires a lookup table. This will need a lot more memory.
// calculate the sun angle
void calculate_angles(void);

#endif

#endif /* CALC_H */
