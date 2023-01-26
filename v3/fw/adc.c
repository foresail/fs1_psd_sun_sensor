#include <msp430.h>
#include "main.h"
#include "i2c.h"



volatile int adc_done;
volatile int samples_todo;
volatile int16_t temperature;


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
	ADCCTL0 = ADCSHT_2 | ADCON;                // S&H = 16 ADCCLKs; ADC ON
	ADCCTL1 = ADCSHS_0 | ADCSHP | ADCCONSEQ_0 | ADCDIV_0 | ADCSSEL_1; // S/W trig, SMCLK/1, single ch/conv
	ADCCTL2 = ADCRES_1 | ADCDF_0 | ADCSR;     // 12-bit conversion results, unsigned, 50ksps
	ADCIE |= ADCIE0;                           // Enable the interrupt request for a completed ADC conversion
//
	//ADCIE = 0x3F;
	//__delay_cycles(400);                      // Delay for Ref to settle
	// Note: No delay is needed here because of the normal startup delay
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

	raw.vx1 = 0;
	raw.vx2 = 0;
	raw.vy1 = 0;
	raw.vy2 = 0;

	adc_done = 0;
	samples_todo = calibration.samples;
	if (samples_todo == 0 || samples_todo > 8)
		samples_todo = 1;

	ADCCTL0 &= ~ADCENC; // Force disable ADC for configuring
	ADCMCTL0 = ADCSREF_2 + ADCINCH_1; // Select ADC input channel
	ADCCTL0 |= ADCENC | ADCSC; // Sampling and conversion start

	// Wait the ADC conversions to end
	i = 100;
	while (!adc_done) { // && i-- > 0
		//__bis_SR_register(LPM0_bits + GIE);
		__no_operation(); // Wait few ticks
		__no_operation();
		__no_operation();
	}

	adc_done = 0;

	if (i == 0) { // ADC is not able to conversion!
		raw.vx1 = 0xFFFF;
		raw.vx2 = 0xFFFF;
		raw.vy1 = 0xFFFF;
		raw.vy2 = 0xFFFF;
		return;
	}

	if (calibration.samples == 1) {
		// none
	}
	else if (calibration.samples == 2) {
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
	else if (calibration.samples == 16) {
		raw.vx1 >>= 4; raw.vx2 >>= 4;
		raw.vy1 >>= 4; raw.vy2 >>= 4;
	}
	else {
		raw.vx1 /= calibration.samples; raw.vx2 /= calibration.samples;
		raw.vy1 /= calibration.samples; raw.vy2 /= calibration.samples;
	}

	raw.vx1 = 1023 - raw.vx1;
	raw.vx2 = 1023 - raw.vx2;
	raw.vy1 = 1023 - raw.vy1;
	raw.vy2 = 1023 - raw.vy2;

	STOP_TIMING();
}


int read_temperature()
{
	START_TIMING();

	/* Wakeup the opamp and ADC if needed */
	if (sleep_mode)
		wakeup();

	/* Wait for existing conversion */
	unsigned int i = 100;
	while((ADCCTL1 & ADCBUSY) && i-- > 0)
		__no_operation();

	adc_done = 0;
	ADCCTL0 &= ~ADCENC; // Disable ADC
	ADCMCTL0 = ADCSREF_1 + ADCINCH_12; // Compare ADC channel 12 against 1.5V reference
	ADCCTL0 |= ADCENC + ADCSC; // Sampling and conversion start

	i = 100;
	while (!adc_done) { //  && i-- > 0);
		//__bis_SR_register(LPM0_bits + GIE);
		__no_operation(); // Wait few ticks
		__no_operation();
		__no_operation();
	}

	STOP_TIMING();

	if (calibration.temperature_calib == 0) {
		// If calibration offset is 0 give out raw ADC count.
		// This is used for calibration
		return temperature;
	}
	else {
		// Calculate calibrated temperature in desiCelciuses
		int IntDegC = (temperature - calibration.temperature_calib) * 4 + 300;
		return IntDegC;
	}
}


// ADC10 interrupt service routine
#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
{
	switch(__even_in_range(ADCIV, 12))
	{
	case  0: break;                          // No interrupt
	case  2: break;                          // conversion result overflow
	case  4: break;                          // conversion time overflow
	case  6: break;                          // ADCHI
	case  8: break;                          // ADCLO
	case 10: break;                          // ADCIN
	case 12: {                               // ADCIFG0: End of conversion

		switch (ADCMCTL0 & 0x0F) {
		case ADCINCH_1:                      // A1: VX1
			raw.vx1 += ADCMEM0;
			ADCCTL0 &= ~ADCENC;
			ADCMCTL0 = ADCSREF_2 + ADCINCH_6; // Enable conversion for next channel
			ADCCTL0 |= ADCENC + ADCSC;
			break;

		case ADCINCH_6:                      // A6: VX2
			raw.vx2 += ADCMEM0;
			ADCCTL0 &= ~ADCENC;
			ADCMCTL0 = ADCSREF_2 + ADCINCH_4; // Enable conversion for next channel
			ADCCTL0 |= ADCENC + ADCSC;
			break;

		case ADCINCH_4:                      // A4: VY1
			raw.vy1 += ADCMEM0;
			ADCCTL0 &= ~ADCENC;
			ADCMCTL0 = ADCSREF_2 + ADCINCH_5; // Enable conversion for next channel
			ADCCTL0 |= ADCENC + ADCSC;
			break;

		case ADCINCH_5:                      // A5: VY2
			raw.vy2 += ADCMEM0;

			samples_todo--;
			if (samples_todo == 0) {
				ADCCTL0 &= ~ADCENC; // Stop sampling
				adc_done = 1;
				__bic_SR_register_on_exit(LPM0_bits); // Exit LPM
			}
			else {
				ADCCTL0 &= ~ADCENC;
				ADCMCTL0 = ADCSREF_1 + ADCINCH_1; // Enable conversion for next channel
				ADCCTL0 |= ADCENC + ADCSC;
			}
			break;

		case ADCINCH_12:                     // A12: Temperature sensor
			temperature = ADCMEM0;
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
