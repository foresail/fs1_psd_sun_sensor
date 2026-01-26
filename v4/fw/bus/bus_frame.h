#ifndef BUS_FRAME_H
#define BUS_FRAME_H

#include <stdint.h>
#include <stdlib.h>


#define BUS_ADDRESS_OBC 0x01
#define BUS_ADDRESS_EPS 0x02
#define BUS_ADDRESS_UHF 0x03
#define BUS_ADDRESS_EGSE 0x0f
#define BUS_ADDRESS_EGSEBOX 0xff

#define ADCS_PSD_XP (0xA5)
#define ADCS_PSD_XN (0xA6)
#define ADCS_PSD_YP (0xA7)
#define ADCS_PSD_YN (0xA8)
#define ADCS_PSD_ZP (0xA9)
#define ADCS_PSD_ZN (0xAA)
#define ADCS_DSS_YN (0xAB)
#define ADCS_MTQ    (0xAC)

#ifndef BUS_MY_ADDRESS
#define BUS_MY_ADDRESS ADCS_PSD_XP
#endif

#define BUS_SYNC_HIGH 0x5A
#define BUS_SYNC_LOW  0xCE

#define BUS_DATA_MAX 0x100
#define BUS_HEADER_BYTES 7
#define BUS_CRC_BYTES 2
#define BUS_OVERHEAD (BUS_HEADER_BYTES + BUS_CRC_BYTES)

// BusFrame represents packets transferred on the bus.
typedef struct {
    uint16_t len;   // Length of the data field without CRC
    union {
        struct {
            uint8_t sync_high, sync_low;
            uint8_t len_high, len_low;
            uint8_t src, dst, cmd;
            uint8_t data[BUS_DATA_MAX + BUS_CRC_BYTES];
        };

        uint8_t buf[BUS_DATA_MAX + BUS_OVERHEAD];
    };
} BusFrame;


typedef enum {
	BUS_STATE_WAITING_FOR_SYNC,
	BUS_STATE_RX_IN_PROGRESS,
	BUS_STATE_RX_PACKET_RECEIVED,
} BusRxState;

typedef struct Bus BusHandle;

/*
 * Advance receiver state machine
 */
int bus_handle_rx_byte(BusHandle* self, uint8_t data);

/*
 * Bus CRC-16 MODBUS implementation
 */
uint16_t bus_crc16(const uint8_t* data, size_t len);

/*
 * Reset receiver state machine.
 */
void bus_reset_rx(BusHandle* self);

/*
 * Helper function for preparing tx BusFrame. Fills in sync word, len bytes based
 * on rsp->len and calculates crc. Rest is up to user to fill in correctly.
 */
BusFrame* bus_prepare_tx_frame(BusFrame* rsp);

#endif
