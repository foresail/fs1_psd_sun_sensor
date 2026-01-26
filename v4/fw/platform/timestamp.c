#include "timestamp.h"

timestamp_t get_timestamp(void) {
    return sys_ticks<<4; // shift 4 to multiply by 2^4 to get 1 ms
}
