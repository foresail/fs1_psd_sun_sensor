#include "i2c.h"
#include "main.h"
#include "adc.h"
#include <msp430.h>


volatile int new_message = 0;

unsigned char received_message[BUFFER_LENGTH];
unsigned char transmit_message[BUFFER_LENGTH];
unsigned int receive_len, transmit_len, transmit_idx;

#if 0
#define SAMPLING_LED_ON()  LED_ON()
#define SAMPLING_LED_OFF() LED_OFF()
#else
#define SAMPLING_LED_ON()
#define SAMPLING_LED_OFF()
#endif

/*
 * Enhanced Universal Serial Communication Interface (eUSCI) I2C Mode
 * http://www.ti.com/lit/ug/slau425f/slau425f.pdf
 */

void init_i2c() {

	P1SEL0 |= BIT2 | BIT3; // Pins for I2C (Secondary module function)

#if 1
	// Internal pull-ups for testing
	P1REN |= BIT2 | BIT3;
	P1OUT |= BIT2 | BIT3;
#endif

	// eUSCI configuration for I2C
	UCB0CTLW0 |= UCSWRST;                   // Software reset enabled
	UCB0CTLW0 |= UCMODE_3 + UCSYNC;         // I2C slave mode, sync mode, SMCLK
	UCB0I2COA0 = i2c_address + UCOAEN;      // Own address + enable
	UCB0CTLW1 |= UCCLTO_3;                  // Clock low timeout ?ms
	UCB0CTLW0 &= ~UCSWRST;                  // Clear reset register

	// Enable transmit and stop interrupts for slave
	UCB0IE = UCRXIE0 | UCTXIE0 | UCSTTIE | UCSTPIE; // | UCCLTOIE;

	receive_len = 0;
	transmit_len = 0;
	transmit_idx = 0;
}




#pragma vector=USCI_B0_VECTOR
__interrupt void USCIB0_ISR(void)
{
	switch (__even_in_range(UCB0IV, 0x1E))
	{
	case 0x00: break;                   // Vector 0: No interrupts break;
	case 0x02:                          // Vector 2: ALIFG aka arbitration lost
		break;
	case 0x04: break;                   // Vector 4: NACKIFG aka not-acknowledgment
	case 0x06:                          // Vector 6: STTIFG aka Start

		if (UCB0CTLW0 & UCTR) { // Our transmission
			if (transmit_len) {
				UCB0TXBUF = transmit_message[0];
				transmit_idx = 1;
			}
			else {
				transmit_idx = 0;
				UCB0TXBUF = 0xFF; // Feed dummy data
			}

		}
		else { // Receiving starts
			new_message = 0;
			receive_len = 0;
		}


		break;

	case 0x08:                          // Vector 8: STPIFG aka stop

		if (UCB0CTLW0 & UCTR) { /* Transmitting stopped */
			// Mark that the write packet has been sent (maybe unnecessary)
			transmit_len = 0;
		}
		else { /* Receiving stopped */
			// Wake up the main program to process the message
			new_message = 1;
			__bic_SR_register_on_exit(LPM0_bits);
		}

		UCB0IFG &= ~UCSTPIFG; // Clear interrupt
		break;

	case 0x0a: break;                   // Vector 10: RXIFG3
	case 0x0c: break;                   // Vector 14: TXIFG3
	case 0x0e: break;                   // Vector 16: RXIFG2
	case 0x10: break;                   // Vector 18: TXIFG2
	case 0x12: break;                   // Vector 20: RXIFG1
	case 0x14: break;                   // Vector 22: TXIFG1
	case 0x16:                          // Vector 24: RXIFG0 aka Receive (slave 0)

		if (receive_len < BUFFER_LENGTH)
			received_message[receive_len++] = UCB0RXBUF;
		else {
			/* Master tries to write too much. Send NACK! */
			UCB0CTLW0 |= UCTXNACK;
			UCB0IFG &= ~UCRXIFG0;
		}
		break;

	case 0x18:                          // Vector 26: TXIFG0 aka Transmit (slave 0)

		if (transmit_idx < transmit_len) {
			UCB0TXBUF = transmit_message[transmit_idx++];
		}
		else {
			/* Master tries and we have no data! Send dummy bytes because NACKing is not possible... */
			UCB0TXBUF = 0xFF;
			UCB0IFG &= ~UCTXIFG0;
		}
		break;

	case 0x1a: break;                   // Vector 28: BCNTIFG aka byte counter interrupt
	case 0x1c:                          // Vector 30: Clock Low Timeout

		// We should never get here!
		// But if nothing is done here the whole bus will get stuck!

		UCB0IFG &= ~UCCLTOIFG;
		UCB0CTLW0 |= UCTXNACK;
		UCB0TXBUF = 0xFF;

		//init_i2c();
		break;

	case 0x1e: break;                    // Vector 32: 9th bit break;
	default: break;
	}
}


inline void set_response(unsigned char status) {
	transmit_message[0] = status;
	transmit_len = 1;
}


void handle_command() {

	if (receive_len == 0)
		return;


	switch (received_message[0]) {

	case CMD_STATUS: {
		/*
		 * General status/test command
		 */

		transmit_message[0] = sleep_mode ? RSP_SLEEP : RSP_OK;
		transmit_len = 1;

		break;
	}

	case CMD_GET_RAW: {
		/*
		 * Get raw current measurements
		 */

		 SAMPLING_LED_ON();
		read_voltage_channels();
		SAMPLING_LED_OFF();

		transmit_message[0] = RSP_RAW;
		memcpy(transmit_message + 1, &raw, sizeof(raw));
		transmit_len = sizeof(raw) + 1;

		break;
	}

	case CMD_GET_POINT: {
		/*
		 * Get position of the light spot
		 */

		SAMPLING_LED_ON();
		read_voltage_channels();
		calculate_position();
		SAMPLING_LED_OFF();

		transmit_message[0] = RSP_POINT;
		memcpy(transmit_message + 1, &position, sizeof(position));
		transmit_len = 1 + sizeof(position);

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

		transmit_message[0] = RSP_VECTOR;
		memcpy(transmit_message + 1, &vector, sizeof(vector));
		transmit_len = 1 + sizeof(vector);

		break;
	}

	case CMD_GET_ANGLES: {
		/*
		 * Get sun angle
		 */

		SAMPLING_LED_ON();
		read_voltage_channels();
		calculate_position();
		calculate_angles();
		SAMPLING_LED_OFF();

		transmit_message[0] = RSP_ANGLES;
		memcpy(transmit_message + 1, &angles, sizeof(angles));
		transmit_len = 1 + sizeof(angles);

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

		transmit_message[0] = RSP_ALL;
		memcpy(transmit_message + 1, &raw, sizeof(raw));
		memcpy(transmit_message + sizeof(raw) + 1, &position, sizeof(position));
		memcpy(transmit_message + sizeof(raw) + sizeof(position) + 1, &angles, sizeof(angles));
		transmit_len = sizeof(raw) + sizeof(position) + sizeof(angles) + 1;

		break;
	}


	case CMD_GET_TEMPERATURE: {
		/*
		 * Return MCU temperature reading
		 */

		int temperature = read_temperature();

		transmit_message[0] = RSP_TEMPERATURE;
		transmit_message[1] = temperature & 0xFF;
		transmit_message[2] = (temperature >> 8) & 0xFF;
		transmit_len = 3;

		break;
	}

	case CMD_SET_CALIBRATION: {
		/*
		 * Set calibration values
		 */

		if (receive_len == sizeof(calibration) + 1) {

			SYSCFG0 = FRWPPW; // Disable FRAM write protection
			memcpy(&calibration, received_message + 1, sizeof(calibration));
			SYSCFG0 = FRWPPW | PFWP;  // Re-enable FRAM write protection

			set_response(RSP_OK);
		}
		else
			set_response(RSP_INVALID_PARAM);

		break;
	}

	case CMD_GET_CALIBRATION: {
		/*
		 * Get calibration
		 */

		transmit_message[0] = RSP_CALIBRATION;
		memcpy(transmit_message + 1, &calibration, sizeof(calibration));
		transmit_len = sizeof(calibration) + 1;

		break;
	}

	case CMD_SET_LUT: {
		/*
		 * Set a segment of the angle look-up-table
		 */

		unsigned idx = received_message[1];
		if (receive_len != 18 && idx < 32) {
			SYSCFG0 = FRWPPW; // Disable FRAM write protection
			__no_operation();
			//memcpy(&lut[idx], received_message[2], receive_len - 2); // TODO
			SYSCFG0 = FRWPPW | PFWP;  // Re-enable FRAM write protection
			set_response(RSP_OK);
		}
		else
			set_response(RSP_INVALID_PARAM);
		break;
	}

	case CMD_SET_I2C_ADDRESS: {
		/*
		 * Set device I2C address.
		 * Change is permanent but requires a reboot
		 */
		if (receive_len == 2 && (received_message[1] & 0x80) == 0) {
			SYSCFG0 = FRWPPW; // Disable FRAM write protection
			__no_operation();
			i2c_address = received_message[1];
			SYSCFG0 = FRWPPW | PFWP;  // Re-enable FRAM write protection
			set_response(RSP_OK);
		}
		else
			set_response(RSP_INVALID_PARAM);
		break;
	}

	default:
		/* Unknown command */
		set_response(RSP_UNKNOWN_COMMAND);
	}
	receive_len = 0;
}
