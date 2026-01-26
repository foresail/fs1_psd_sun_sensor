#include "bus.h"

BusFrame* bus_get_tx_frame(BusHandle* self) { // __attribute__((weak)) {
    return &self->frame_tx;
}

////////////////////////////////////////////////////////////////////////////////
// Pseudo random number generator
// Source: https://www.analog.com/en/design-notes/random-number-generation-using-lfsr.html
// NOTE: c stdlib rand() uses malloc and free...
////////////////////////////////////////////////////////////////////////////////

#define POLY_MASK_32 0xb4bcd35c
#define POLY_MASK_31 0x7a5bc2e3

static uint32_t lfsr32 = 0xabcde;
static uint32_t lfsr31 = 0x23456789;

static uint32_t shift_lfsr(uint32_t lfsr, uint32_t mask) {
    if (lfsr & 1) {
        lfsr ^= mask;
    } else {
        lfsr >>= 1;
    }
    return lfsr;
}

void bus_seed(uint32_t seed) {
	lfsr32 = seed ^ (seed << 8);
	lfsr31 = seed ^ (seed << 16) ^ (seed >> 16);
}

uint32_t bus_rand() {
    lfsr32 = shift_lfsr(lfsr32, POLY_MASK_32);
    lfsr32 = shift_lfsr(lfsr32, POLY_MASK_32);
    lfsr31 = shift_lfsr(lfsr31, POLY_MASK_31);

    return (lfsr32 ^ lfsr31) & 0xffff;
}
