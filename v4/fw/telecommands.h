#ifndef __TELECOMMANDS_H
#define __TELECOMMANDS_H

#include "bus.h"

/* Command codes: */
#define CMD_GET_STATUS          0x01
#define CMD_GET_RAW             0x02
#define CMD_GET_POSITION        0x03
#define CMD_GET_VECTOR          0x04
#define CMD_GET_ANGLES          0x05
#define CMD_GET_ALL             0x06
#define CMD_GET_TEMPERATURE     0x07
// GET/SET Config commands
#define CMD_GET_CONFIG      0xA1
#define CMD_SET_CONFIG      0xA2

/* Response codes: */
#define RSP_STATUS              0xD1
#define RSP_RAW                 0xD2
#define RSP_POSITION            0xD3
#define RSP_VECTOR              0xD4
#define RSP_ANGLES              0xD5
#define RSP_ALL                 0xD6
#define RSP_TEMPERATURE         0xD7
#define RSP_CONFIG              0xE1

// Config sub commands
#define CMD_CONFIG_CALIBRATION  0xB1
#define CMD_CONFIG_LUT          0xB2

/* Status codes: */
#define RSP_STATUS_OK                 0xF0
#define RSP_STATUS_SLEEP              0xF1
#define RSP_STATUS_UNKNOWN_COMMAND    0xF2
#define RSP_STATUS_INVALID_PARAM      0xF3
#define RSP_STATUS_ERROR              0xF4

#define RSP_STATUS_SAMPLING_ERROR     0xF5
#define RSP_STATUS_DIVISION_ZERO      0xF6
#define RSP_STATUS_NOT_ODD            0xF7
#define RSP_STATUS_CALC_ERROR         0xF8

/* Subsystem-specific command handler.
 * Return 1 if there is a response, 0 if not. */
void handle_command(const BusFrame* cmd, BusFrame* rsp);

#endif
