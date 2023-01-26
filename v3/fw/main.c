#include <msp430fr2311.h>
#include <msp430.h>

#include "main.h"
#include "i2c.h"
#include "adc.h"



raw_measurements_t raw;
position_measurement_t position;
vector_measurement_t vector;
angle_measurement_t angles;
int sleep_mode = 1;


#pragma SET_DATA_SECTION(".fram_vars")
calibration_t calibration = {
	.offset_x = 0,
	.offset_y = 0,
	.height = 670,
	.samples = 1,
	.temperature_calib = 662,
};

int calibration_enabled = 1;
uint8_t i2c_address = I2C_ADDRESS;
#pragma SET_DATA_SECTION()



/*
 * Watchdog time configuration
 *
 * Select master clock as the clock source WDT is cleared only in the heartbeat interrupt,
 * so the reset time must be greater than maximum heartbeat period.
 *
 * WDTIS_0 = 2^31 clock cycles = 1074 seconds aka 18 minutes @2MHz
 * WDTIS_1 = 2^27 clock cycles = 67 seconds @2MHz
 * WDTIS_2 = 2^23 clock cycles = 4.2 seconds @2MHz
 */
#ifdef USE_WDT
#define RESET_WDT() (WDTCTL = WDTPW + WDTCNTCL + WDTIS_1)
#else
#define RESET_WDT() (WDTCTL = WDTPW + WDTHOLD) // Disabled!
#endif


int main(void)
{
	RESET_WDT();

	/*
	 * 1) Initialization of clocks
	 */

	// Configure DCO
	CSCTL0 = 0;
	CSCTL1 = DCOFTRIMEN_1 | DCOFTRIM2 | DCOFTRIM0 | DCORSEL_3 | DISMOD_1; // Set DCO to 4MHz, No modulation
	//CSCTL2 = FLLD;                          // Set FFL = DCO = 4MHz
	//CSCTL3 = FLLREFDIV;                     // Set all dividers to 1
	CSCTL4 = SELMS_0;                       // ACLK = None, MCKL = DCO
	CSCTL5 = VLOAUTOOFF | DIVS_0 | DIVM_0;  // SMCLK = DCO
	//CSCTL8 = MCLKREQEN;

	CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK;

	/*
	 * 2) Initialize all control IO pins
	 */

	/** Configure PORT1 */
	// P1.0 = VEREF+            Analog Ref
	// P1.1 = VX1               Analog 1 IN
	// P1.2 = I2C data          IN/OUT
	// P1.3 = I2C clock         IN
	// P1.4 = VY1               Analog 4 IN
	// P1.5 = VY2               Analog 5 IN
	// P1.6 = VX2               Analog 6 IN
	// P1.7 = VREF              1.5V Reference out
	P1DIR = 0;
	P1OUT = 0;

	// Setup analog inputs and VREF output
	P1SEL0 = BIT1 | BIT4 | BIT5 | BIT6 | BIT7;
	P1SEL1 = BIT1 | BIT4 | BIT5 | BIT6 | BIT7;

	PMMCTL0_H = PMMPW_H; // Unlock PMM
	PMMCTL2 |= EXTREFEN | INTREFEN; // Enable internal reference and VREF output to pin P1.7
	PMMCTL2 |= TSENSOREN; // Enable internal temperature sensor
	PMMCTL0_H = 0; // Lock PMM

	/** Configure PORT2 */
	// P2.0 = Vopamp          OUT
	// P2.6 = LED             OUT
	// P2.7 = Unconnected pin used for timing analysis
	P2DIR = BIT0 | BIT6 | BIT7;
	P2OUT = 0; // Opamp and LED are off by default


	PM5CTL0 &= ~LOCKLPM5;


	/*
	 * 3) Initialize other peripherals
	 */
	init_i2c();
	init_adc();
	init_heartbeat_timer();

	sleepmode();

	/*
	 * 4) Start normal operation aka listen I2C and other interrupts.
	 */

	__bis_SR_register(GIE); // Enable interrupts
	__no_operation(); // First chance for the interrupt to fire!

	RESET_WDT();

	unsigned int idle_counter = ~0;

	while(1) {

		// Goto sleep except listen interrupts
		__bis_SR_register(LPM1_bits + GIE);
		__no_operation();

		// If we wake up check is there new messages to be handled...
		if (new_message) {
			handle_command();
			new_message = 0;
			idle_counter = 0;
		}


		RESET_WDT();
		TB0CTL |= TBCLR;

		if (idle_counter > 20 && !sleep_mode) {
			// Goto "deepsleep" if I2C is not actively used
			sleepmode();
		}
		else {
			idle_counter++;
			if (idle_counter >= 500) {
				// Trigger POR reset after ~20 seconds of idling
				PMMCTL0 |= PMMSWPOR;
				while(1);
			}
		}
	}

}


void calculate_position() {

	int32_t sum = raw.vx1 + raw.vx2 + raw.vy1 + raw.vy2;
	int32_t a = (int32_t)(raw.vx2 + raw.vy1) - (int32_t)(raw.vx1 + raw.vy2);
	int32_t b = (int32_t)(raw.vx2 + raw.vy2) - (int32_t)(raw.vx1 + raw.vy1);

	position.x = (a << 11) / sum; // value from -1024 to 1024
	position.y = (b << 11) / sum;
	position.intensity = sum >> 2; // 0 - 1024

	// In some corner case when no sun is visible and ADCs are reporting near 0
	// it's possible to get negative intensity number.
	if (position.intensity > 1024)
		position.intensity = 0;

	if (calibration_enabled) {
		position.x += calibration.offset_x;
		position.y += calibration.offset_y;
	}

}



#define LUT_SIZE 256

const int16_t lt[LUT_SIZE] = {
	     0,    9,   18,   27,   36,   45,   54,   62,
	    71,   80,   89,   98,  106,  115,  123,  132,
	   140,  149,  157,  165,  174,  182,  190,  198,
	   206,  213,  221,  229,  236,  244,  251,  258,
	   266,  273,  280,  287,  294,  300,  307,  314,
	   320,  326,  333,  339,  345,  351,  357,  363,
	   369,  374,  380,  386,  391,  396,  402,  407,
	   412,  417,  422,  427,  432,  436,  441,  445,
	   450,  454,  459,  463,  467,  472,  476,  480,
	   484,  488,  491,  495,  499,  503,  506,  510,
	   513,  517,  520,  524,  527,  530,  533,  537,
	   540,  543,  546,  549,  552,  555,  558,  560,
	   563,  566,  569,  571,  574,  576,  579,  581,
	   584,  586,  589,  591,  593,  596,  598,  600,
	   603,  605,  607,  609,  611,  613,  615,  617,
	   619,  621,  623,  625,  627,  629,  631,  633,
};


int16_t atan(int16_t x) {
	x = (x >= 0) ? x : -x;
	unsigned int pos = x >> 2;
	if (pos >= LUT_SIZE - 1)
		return lt[LUT_SIZE - 1];

	// Linear interpolation from pos to pos+1 without using multiply operation
	unsigned int y = lt[pos];
	unsigned int d = (lt[pos + 1] + lt[pos]) >> 2;
	unsigned int i = x - (pos << 2);
	while (i-- > 0)
		y += d;

	return (x >= 0) ? (int16_t)y : -((int16_t)y);
}


void calculate_vectors() {
	vector.x = -position.x;
	vector.y = -position.y;
	vector.z = calibration.height;
	vector.intensity = position.intensity;
}


void calculate_angles() {

	angles.ax = atan(position.x);
	angles.ay = atan(position.y);

	angles.intensity = position.intensity;
}


void sleepmode() {

	OPAMP_OFF();

	PMMCTL0_H = PMMPW_H; // Unlock PMM
	PMMCTL2 &= ~INTREFEN; // Disable internal voltage reference
	PMMCTL2 &= ~TSENSOREN; // Disable internal temperature sensor
	ADCCTL0 &= ~ADCON; // Disable ADC
	PMMCTL0_H = 0; // Lock PMM

	sleep_mode = 1;
}

void wakeup() {

	OPAMP_ON();

	PMMCTL0_H = PMMPW_H; // Unlock PMM
	PMMCTL2 |= INTREFEN; // Enable internal voltage reference
	PMMCTL2 |= TSENSOREN; // Enable internal temperature sensor
	ADCCTL0 |= ADCON; // Enable ADC
	PMMCTL0_H = 0; // Lock PMM

	__delay_cycles(400);  // Delay for stuff to settle

	sleep_mode = 0;
}


void init_heartbeat_timer() {
	/*
	 * Configure TimerA0
	 *
	 * SMCLK = DCO / 1
	 * Timer frequency = SMCLK / (2000 * 8 * 8) = 1953 Hz
	 */

	TB0CTL = ID_3 + TBSSEL_2 + MC_1; // SMCLK, UP mode, divide by 8
	TB0CCR0 = 3300; // up to this count
	TB0EX0 = TBIDEX_7; // Divide by 8
	TB0CCTL0 = CCIE; // TACCR0 interrupt enabled
	TB0CTL |= TBCLR;
	TB0R = 0;
}


/*
 * Timer B0 interrupt (triggering every 50ms)
 */
#pragma vector=TIMER0_B0_VECTOR
__interrupt void Timer_B0(void)
{
	// Wake up the main loop
	__bic_SR_register_on_exit(LPM0_bits);
}
