#include "telecommands.h"
#include "platform/debug.h"
#include "main.h"
#include "calc.h"
#include "adc.h"
#include <msp430.h>
#include <string.h>

#ifdef DEBUG
#define SAMPLING_LED_ON()  LED_ON()
#define SAMPLING_LED_OFF() LED_OFF()
#else
#define SAMPLING_LED_ON()
#define SAMPLING_LED_OFF()
#endif

static void respond_with_status_code(BusFrame* rsp, uint8_t status_code) {
	rsp->cmd = RSP_STATUS;
	rsp->len = 1;
	rsp->data[0] = status_code;
}

void handle_command(const BusFrame* cmd, BusFrame* rsp) {
	rsp->dst = cmd->src;

	switch (cmd->cmd) {

	    case CMD_GET_STATUS: {
	        /*
	         * General status/test command
	         */

	        respond_with_status_code(rsp, sleep_mode ? RSP_STATUS_SLEEP : RSP_STATUS_OK);

	        break;
	    }

	    case CMD_GET_RAW: {
	        /*
	         * DOES NOT SAMPLE SENSOR
	         * Get raw current measurements
	         */

	        rsp->cmd = RSP_RAW;
	        memcpy(rsp->data, &raw, sizeof(raw));
	        rsp->len = sizeof(raw);

	        break;
	    }

	    case CMD_GET_POSITION: {
	        /*
	         * Get position of the light spot
	         */

	        SAMPLING_LED_ON();
	        read_voltage_channels();
	        calculate_position();
	        SAMPLING_LED_OFF();

	        rsp->cmd = RSP_POSITION;
	        memcpy(rsp->data, &position, sizeof(position));
	        rsp->len = sizeof(position);

	        break;
	    }

	    case CMD_GET_VECTOR: {
	        /*
	         * Get sun vector
	         */

	        SAMPLING_LED_ON();
	        read_voltage_channels();
	        calculate_position();
	        calculate_vectors();
	        SAMPLING_LED_OFF();

	        rsp->cmd = RSP_VECTOR;
	        memcpy(rsp->data, &vector, sizeof(vector));
	        rsp->len = sizeof(vector);

	        break;
	    }

#ifdef CALC_ANGLES
	    case CMD_GET_ANGLES: {
	        /*
	         * Get sun angle
	         */

	        SAMPLING_LED_ON();
	        read_voltage_channels();
	        calculate_position();
	        calculate_angles();
	        SAMPLING_LED_OFF();

	        rsp->cmd = RSP_ANGLES;
	        memcpy(rsp->data, &angles, sizeof(angles));
	        rsp->len = sizeof(angles);

	        break;
	    }

	    case CMD_GET_ALL: {
	        /*
	         * Get all the measurement data (mainly for testing purposes)
	         */

	        SAMPLING_LED_ON();
	        read_voltage_channels();
	        calculate_position();
	        calculate_angles();
	        SAMPLING_LED_OFF();

	        rsp->cmd = RSP_ALL;
	        memcpy(rsp->data, &raw, sizeof(raw));
	        memcpy(rsp->data + sizeof(raw), &position, sizeof(position));
	        memcpy(rsp->data + sizeof(raw) + sizeof(position), &angles, sizeof(angles));
	        rsp->len =  sizeof(raw) + sizeof(position) + sizeof(angles);
	        break;

	    }
#endif

	    case CMD_GET_TEMPERATURE: {
	        /*
	         * Return MCU temperature reading
	         */

            int16_t temp = read_temperature();

	        rsp->cmd = RSP_TEMPERATURE;
	        memcpy(rsp->data, &temp, sizeof(temp));
	        rsp->len = sizeof(temp);

	        break;
	    }
        case CMD_GET_CONFIG: {
            switch (cmd->data[0]) {
                case CMD_CONFIG_CALIBRATION: {
                    //
                    // Get calibration
                    //

                    rsp->cmd = RSP_CONFIG;
                    rsp->data[0] = CMD_CONFIG_CALIBRATION;
                    memcpy(rsp->data+1, &calibration, sizeof(calibration));
                    rsp->len = sizeof(calibration) +1;

                    break;
                }
                default:
                {
                    respond_with_status_code(rsp, RSP_STATUS_UNKNOWN_COMMAND);
                }
            }
            break;
        }
	    case CMD_SET_CONFIG: {
	        switch (cmd->data[0]) {
                case CMD_CONFIG_CALIBRATION: {
                    //
                    // Set calibration values
                    //

                    if (cmd->len == sizeof(calibration)+1) {

                        SYSCFG0 = FRWPPW; // Disable FRAM write protection
                        memcpy(&calibration, cmd->data+1, sizeof(calibration));
                        SYSCFG0 = FRWPPW | PFWP;  // Re-enable FRAM write protection

                        respond_with_status_code(rsp,RSP_STATUS_OK);
                    }
                    else
                        respond_with_status_code(rsp,RSP_STATUS_INVALID_PARAM);

                    break;
                }

#ifdef CALC_ANGLES
                case CMD_CONFIG_LUT: {
                    //
                    // Set a segment of the angle look-up-table
                    //

                    unsigned idx = cmd->data[0];
                    if (cmd->len != 18 && idx < 32) {
                        SYSCFG0 = FRWPPW; // Disable FRAM write protection
                        __no_operation();
                        //memcpy(&lut[idx], cmd->data[1], cmd->len - 1); // TODO
                        SYSCFG0 = FRWPPW | PFWP;  // Re-enable FRAM write protection
                        respond_with_status_code(rsp,RSP_STATUS_OK);
                    }
                    else
                        respond_with_status_code(rsp,RSP_STATUS_INVALID_PARAM);
                    break;
                }
#endif
                default: {
                    respond_with_status_code(rsp, RSP_STATUS_UNKNOWN_COMMAND);
                }
            }
	        break;
	    }
	    default:
	    {
	        respond_with_status_code(rsp, RSP_STATUS_UNKNOWN_COMMAND);
	        break;
	    }
	    }

	reset_idle_counter();

}
