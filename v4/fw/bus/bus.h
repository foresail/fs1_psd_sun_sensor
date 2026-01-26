#ifndef BUS_H
#define BUS_H

#include "bus_frame.h"

// NOTE:
// It expected that user zero initializes the Bus struct. Usually
// it is defined as a global variable and it is placed in .bss
// section that is zero-initialized by default at init.
struct Bus {
    BusFrame frame_rx;
    BusFrame frame_tx;

	BusRxState rx_state;
    size_t rx_index, rx_length;

    // NOTE: after packet has been received, there should be no
    // traffic on the bus, so using larger sized error counters
    // is also possible without needing locks.
	uint8_t sync_errors, len_errors, crc_errors;
    uint8_t receive_timeouts;

    void* driver;
};

/*
 * Get memory allocation for preparing a frame for transmitting.
 */
BusFrame *bus_get_tx_frame(BusHandle *self);

////////////////////////////////////////////////////////////////////////////////
// Slave API -- nonblocking
////////////////////////////////////////////////////////////////////////////////

/*
 * Receive a new command appointed to our device.
 * Function is non-blocking and returns NULL if no new command has been received.
 */
BusFrame* bus_slave_receive(BusHandle* self);

/*
 * Transmit response frame.
 * Non blocking function. The transmiting continues on the background.
 */
void bus_slave_send(BusHandle* self, BusFrame* rsp);


////////////////////////////////////////////////////////////////////////////////
// Master API -- blocking
////////////////////////////////////////////////////////////////////////////////
#ifdef BUS_MASTERING_API

#define BUS_MAX_MASTER_TRIES 10
#define BUS_MIN_DELAY 5 // ticks
#define BUS_MASTER_RECEIVE_TIMEOUT 20 // [ms]/[ticks]
#define MAX_FRAME_DURATION   (1000 * (8 *  256 / 115200)) // [ms]/[ticks]

/*
 * Reserve the bus for mastering.
 * The function will block until a bus has become free and reserved for us.
 */
int bus_master_take(BusHandle* self);

/*
 * Release mastering mutex and stop access line pulling.
 */
void bus_master_give(BusHandle* self);

/*
 * Make a transfer on the bus after reserving it.
 * The function transmits the given command frame to bus and waits for response.
 * Received response frame is returned.
 */
BusFrame* bus_master_transfer(BusHandle* self, BusFrame* cmd);

/*
 * Initialize pseudorandom generator used by the bus mastering implementation.
 */
void bus_seed(uint32_t seed);

/*
 * Get a pseudorandom number.
 */
uint32_t bus_rand();

#endif /* BUS_MASTERING_API */

#endif /* BUS_H */
