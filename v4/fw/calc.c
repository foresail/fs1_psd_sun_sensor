#include "calc.h"

raw_measurements_t raw;
position_measurement_t position;
vector_measurement_t vector;
#ifdef CALC_ANGLES

angle_measurement_t angles;

#endif

#ifdef NO_CCS
__attribute__ ((section(".fram_vars")))
#else
//#pragma SET_DATA_SECTION(".fram_vars")
#endif
calibration_t calibration = {
    .offset_x = 0,
    .offset_y = 0,
    .height = 670,
    .samples = 1,
    .temperature_bias = 0 // Temperature BIAS in deciCelciuses

};

#ifndef NO_CCS
#pragma SET_DATA_SECTION()
#endif

void calculate_position() {

    int32_t sum = raw.vx1 + raw.vx2 + raw.vy1 + raw.vy2;
    int32_t a = (int32_t)(raw.vx2 + raw.vy1) - (int32_t)(raw.vx1 + raw.vy2);
    int32_t b = (int32_t)(raw.vx2 + raw.vy2) - (int32_t)(raw.vx1 + raw.vy1);

    position.x = (int16_t)((a << 11) / sum); // value from -1024 to 1024
    position.y = (int16_t)((b << 11) / sum);
    position.intensity = (uint16_t)(sum >> 2); // 0 - 1024

    // In some corner case when no sun is visible and ADCs are reporting near 0
    // it's possible to get negative intensity number.
    if (position.intensity > 1024)
        position.intensity = 0;

    position.x += calibration.offset_x;
    position.y += calibration.offset_y;

}

void calculate_vectors() {
    vector.x = -position.x;
    vector.y = -position.y;
    vector.z = calibration.height;
    vector.intensity = position.intensity;
}


#ifdef CALC_ANGLES

#define LUT_SIZE 256

const int16_t lt[LUT_SIZE] = {
         0,    9,   18,   27,   36,   45,   54,   62,
        71,   80,   89,   98,  106,  115,  123,  132,
       140,  149,  157,  165,  174,  182,  190,  198,
       206,  213,  221,  229,  236,  244,  251,  258,
       266,  273,  280,  287,  294,  300,  307,  314,
       320,  326,  333,  339,  345,  351,  357,  363,
       369,  374,  380,  386,  391,  396,  402,  407,
       412,  417,  422,  427,  432,  436,  441,  445,
       450,  454,  459,  463,  467,  472,  476,  480,
       484,  488,  491,  495,  499,  503,  506,  510,
       513,  517,  520,  524,  527,  530,  533,  537,
       540,  543,  546,  549,  552,  555,  558,  560,
       563,  566,  569,  571,  574,  576,  579,  581,
       584,  586,  589,  591,  593,  596,  598,  600,
       603,  605,  607,  609,  611,  613,  615,  617,
       619,  621,  623,  625,  627,  629,  631,  633,
};

int16_t atan(int16_t x) {
    x = (x >= 0) ? x : -x;
    unsigned int pos = x >> 2;
    if (pos >= LUT_SIZE - 1)
        return lt[LUT_SIZE - 1];

    // Linear interpolation from pos to pos+1 without using multiply operation
    unsigned int y = lt[pos];
    unsigned int d = (lt[pos + 1] + lt[pos]) >> 2;
    unsigned int i = x - (pos << 2);
    while (i-- > 0)
        y += d;

    return (x >= 0) ? (int16_t)y : -((int16_t)y);
}

void calculate_angles() {

    angles.ax = atan(position.x);
    angles.ay = atan(position.y);

    angles.intensity = position.intensity;
}

#endif
