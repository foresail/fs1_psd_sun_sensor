#ifndef __I2C_H__
#define __I2C_H__

#include <stdint.h>


/* Default slave I2C Address: */
#define I2C_ADDRESS             0x4A
/*
 * PSD sensor I2C addresses

#define PSD_ADDR_XP             0x4A
#define PSD_ADDR_XN             0x4B
#define PSD_ADDR_YP             0x4C
#define PSD_ADDR_YN             0x4D
#define PSD_ADDR_ZP             0x4E
#define PSD_ADDR_ZN             0x4F
 */

/* Command codes: */
#define CMD_STATUS              0x01
#define CMD_GET_RAW             0x03
#define CMD_GET_POINT           0x04
#define CMD_GET_VECTOR          0x05
#define CMD_GET_ANGLES          0x06
#define CMD_GET_ALL             0x07
#define CMD_GET_TEMPERATURE     0x08
#define CMD_SET_CALIBRATION     0x10
#define CMD_GET_CALIBRATION     0x11
#define CMD_SET_LUT             0x12
#define CMD_SET_I2C_ADDRESS     0xE8

/* Response codes: */
#define RSP_OK                  0xF0
#define RSP_SLEEP               0xF1
#define RSP_RAW                 0xF3
#define RSP_POINT               0xF4
#define RSP_VECTOR              0xF5
#define RSP_ANGLES              0xF6
#define RSP_ALL                 0xF7
#define RSP_TEMPERATURE         0xF8
#define RSP_CALIBRATION         0xFA
#define RSP_UNKNOWN_COMMAND     0xFD
#define RSP_INVALID_PARAM       0xFE
#define RSP_ERROR               0xFF


// Max I2C packet length (Must me a bit longer than telemetry frame!)
#define BUFFER_LENGTH   24


// I2C bus
extern volatile int new_message; // Flag to indicate a new received command
extern unsigned char received_message[BUFFER_LENGTH];
extern unsigned char transmit_message[BUFFER_LENGTH];
extern unsigned int receive_len, transmit_len, transmit_idx;



/*
 * Initialize I2C slave driver
 */
void init_i2c();

/*
 * Handle received command
 */
void handle_command();


#endif /* __I2C_H__*/
