#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <stdint.h>

// Systick (1 systick = 16 ms)
extern volatile uint16_t sys_ticks;

typedef uint16_t timestamp_t;

#define TIMESTAMP_MS  (1)
//#define TIMESTAMP_SEC (TIMESTAMP_MS * 1000)

timestamp_t get_timestamp(void);

#endif
