#include <msp430.h>
#include "main.h"
#include "calc.h"

volatile int adc_done;
volatile int samples_todo;
volatile int16_t temperature_raw;

#define CAL_ADC_15T30  *((uint16_t *)0x1A1A)   // Temperature Sensor Calibration-30 C for 1V5 (value around 675)
                                               // See device-specific datasheet for TLV table memory mapping
#define CAL_ADC_15T85  *((uint16_t *)0x1A1C)   // Temperature Sensor Calibration-85 C for 1V5 (value around 802)


/*
 * Port 2.7 can be used for timing analysis purposes
 */
#if 0
#define START_TIMING()  P2OUT |=  BIT7
#define STOP_TIMING()   P2OUT &= ~BIT7
#else
#define START_TIMING()
#define STOP_TIMING()
#endif

void init_adc(void)
{
	// Configure ADC - Pulse sample mode; ADCSC trigger
	ADCCTL0 = ADCSHT_2 | ADCON;                                         // S&H = 16 ADCCLKs; ADC ON
	ADCCTL1 = ADCSHS_0 | ADCSHP_1 | ADCCONSEQ_0 | ADCDIV_0 | ADCSSEL_1; // S/W trig, ACLK(REFCLK(32kHz)))/1, single ch/conv
	ADCCTL2 = ADCRES_1 | ADCDF_0 | ADCSR;                               // 10-bit conversion results, unsigned, 50ksps

	ADCIE |= ADCIE0;                                                    // Enable the interrupt request for a completed ADC conversion
}


void read_voltage_channels()
{

	START_TIMING();

	/* Wakeup the opamp and ADC if needed */
	if (sleep_mode)
		wakeup();

	/* Wait for existing conversion */
	unsigned int i = 100;
	while ((ADCCTL1 & ADCBUSY) && i-- > 0)
		__no_operation();

	// reset raw values
	raw.vx1 = raw.vx2 = raw.vy1 = raw.vy2 = 0;

	adc_done = 0;
	samples_todo = calibration.samples;
	// prevent wrong calibration values
	if (samples_todo == 0 || samples_todo % 2 != 0)
		samples_todo = 1;

	ADCCTL0 &= ~ADCENC;                 // Force disable ADC for configuring
	ADCMCTL0 = ADCSREF_2 + ADCINCH_5;   // Select first ADC input channel (VX1)
	ADCCTL0 |= ADCENC | ADCSC;          // Sampling and conversion start

	// interrupts for finished conversions will be triggered in sequence

	// Wait the ADC conversions to end
	i = 100;
	while (!adc_done && i-- > 0) {
		__bis_SR_register(LPM0_bits + GIE);
		__no_operation(); // Wait few ticks
		__no_operation();
		__no_operation();
	}

	adc_done = 0;

	if (i == 0) { // ADC is not able to conversion!
	    // set raw values to indicate wrong numbers
		raw.vx1 = raw.vx2 = raw.vy1 = raw.vy2 = 0xFFFF;
		return;
	}

	if (calibration.samples == 2) {
		raw.vx1 >>= 1; raw.vx2 >>= 1;
		raw.vy1 >>= 1; raw.vy2 >>= 1;
	}
	else if (calibration.samples == 4) {
		raw.vx1 >>= 2; raw.vx2 >>= 2;
		raw.vy1 >>= 2; raw.vy2 >>= 2;
	}
	else if (calibration.samples == 8) {
		raw.vx1 >>= 3; raw.vx2 >>= 3;
		raw.vy1 >>= 3; raw.vy2 >>= 3;
	}
	/*else if (calibration.samples == 16) {
		raw.vx1 >>= 4; raw.vx2 >>= 4;
		raw.vy1 >>= 4; raw.vy2 >>= 4;
	}
	else {
		raw.vx1 /= calibration.samples; raw.vx2 /= calibration.samples;
		raw.vy1 /= calibration.samples; raw.vy2 /= calibration.samples;
	}*/


	// raw = 1023-raw
	// XOR is more effective than subtraction
	raw.vx1 = 1023 ^ raw.vx1;
	raw.vx2 = 1023 ^ raw.vx2;
	raw.vy1 = 1023 ^ raw.vy1;
	raw.vy2 = 1023 ^ raw.vy2;

	STOP_TIMING();
}


int16_t read_temperature()
{
	START_TIMING();

	// Wakeup the opamp and ADC if needed
	if (sleep_mode)
		wakeup();

    // Wait for existing conversion
    uint8_t i = 100;
    while((ADCCTL1 & ADCBUSY) && i-- > 0)
        __no_operation();

    adc_done = 0;
    ADCCTL0 &= ~ADCENC; // Disable ADC
    ADCMCTL0 = ADCSREF_1 + ADCINCH_12; // Compare ADC channel 12 against 1.5V reference
    ADCCTL0 |= ADCENC + ADCSC; // Sampling and conversion start

    i = 100;
    while (!adc_done && i-- > 0) {
        __bis_SR_register(LPM0_bits + GIE);
        __no_operation(); // Wait few ticks
        __no_operation();
        __no_operation();
    }

    if (i == 0) { // ADC is not able to conversion!
        return 0xFFFF;
    }

    // int32_t needed because otherwise pre1 and 2 will overflow
	int32_t pre1 =  (temperature_raw - (int32_t)CAL_ADC_15T30)* (850 - 300);
	int32_t pre2 = (int32_t)CAL_ADC_15T85 - (int32_t)CAL_ADC_15T30;
	temperature_raw = (pre1 / pre2) + 300;

	STOP_TIMING();
    // Calculate calibrated temperature in deciCelciuses
    return (temperature_raw + calibration.temperature_bias);
}


// ADC10 interrupt service routine
#ifdef NO_CCS
void __attribute__ ((interrupt("adc"))) ADC_ISR(void)
#else
#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
#endif
{
	switch(__even_in_range(ADCIV, 12))
	{
	case ADCIV__NONE: break;                // No interrupt
	case ADCIV__ADCOVIFG: break;            // conversion result overflow
	case ADCIV__ADCTOVIFG: break;           // conversion time overflow
	case ADCIV__ADCHIIFG: break;            // ADCHI
	case ADCIV__ADCLOIFG: break;            // ADCLO
	case ADCIV__ADCINIFG: break;            // ADCIN
	case ADCIV__ADCIFG0: {                  // ADCIFG0: End of conversion

		switch (ADCMCTL0 & 0x0F) {
#ifdef V4X
        // P1.1 = VX1               Analog 1 IN
        // P1.3 = VX2               Analog 3 IN
        // P1.4 = VY1               Analog 4 IN
        // P1.5 = VY2               Analog 5 IN
        case ADCINCH_5:                      // A5: VY2
			raw.vy2 += ADCMEM0;
			ADCCTL0 &= ~ADCENC;
			ADCMCTL0 = ADCSREF_2 + ADCINCH_4; // Enable conversion for next channel
			ADCCTL0 |= ADCENC + ADCSC;
			break;

		case ADCINCH_4:                      // A4: VY1
			raw.vy1 += ADCMEM0;
			ADCCTL0 &= ~ADCENC;
			ADCMCTL0 = ADCSREF_2 + ADCINCH_3; // Enable conversion for next channel
			ADCCTL0 |= ADCENC + ADCSC;
			break;

		case ADCINCH_3:                      // A3: VX2
			raw.vx2 += ADCMEM0;
			ADCCTL0 &= ~ADCENC;
			ADCMCTL0 = ADCSREF_2 + ADCINCH_1; // Enable conversion for next channel
			ADCCTL0 |= ADCENC + ADCSC;
			break;

		case ADCINCH_1:                      // A1: VX1
			raw.vx1 += ADCMEM0;

#else
		case ADCINCH_5:                      // A5: VX1
			raw.vx1 += ADCMEM0;
			ADCCTL0 &= ~ADCENC;
			ADCMCTL0 = ADCSREF_2 + ADCINCH_4; // Enable conversion for next channel
			ADCCTL0 |= ADCENC + ADCSC;
			break;

		case ADCINCH_4:                      // A4: VX2
			raw.vx2 += ADCMEM0;
			ADCCTL0 &= ~ADCENC;
			ADCMCTL0 = ADCSREF_2 + ADCINCH_3; // Enable conversion for next channel
			ADCCTL0 |= ADCENC + ADCSC;
			break;

		case ADCINCH_3:                      // A3: VY1
			raw.vy1 += ADCMEM0;
			ADCCTL0 &= ~ADCENC;
			ADCMCTL0 = ADCSREF_2 + ADCINCH_1; // Enable conversion for next channel
			ADCCTL0 |= ADCENC + ADCSC;
			break;

		case ADCINCH_1:                      // A1: VY2
			raw.vy2 += ADCMEM0;
#endif
			samples_todo--;
			if (samples_todo == 0) {
				ADCCTL0 &= ~ADCENC; // Stop sampling
				adc_done = 1;
				__bic_SR_register_on_exit(LPM0_bits); // Exit LPM
			}
			else {
				ADCCTL0 &= ~ADCENC;
				ADCMCTL0 = ADCSREF_1 + ADCINCH_4; // Enable conversion for next channel
				ADCCTL0 |= ADCENC + ADCSC;
			}
			break;

		case ADCINCH_12:                     // A12: Temperature sensor
		    temperature_raw = ADCMEM0;
			ADCCTL0 &= ~ADCENC;
			adc_done = 1;
			__bic_SR_register_on_exit(LPM0_bits); // Exit LPM
			break;
		}

		break;                               // Clear CPUOFF bit from 0(SR)
	}
	default: break;
	}
}
