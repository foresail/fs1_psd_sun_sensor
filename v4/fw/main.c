#include <msp430.h>

#include "main.h"
#include "adc.h"
#include "telecommands.h"

static volatile int interrupt_pending = 0;

////////////////////////////////////////////////////////////////////////////////
/// Platform bus code
////////////////////////////////////////////////////////////////////////////////

#define RS485_PRI_DIR_TX() P1OUT |= BIT2
#define RS485_PRI_DIR_RX() P1OUT &= ~BIT2

typedef struct {
	int active_bus;

	const uint8_t* tx_buf;
	size_t tx_idx, tx_len;

	int slave_rxed;
} BusDriver;

#define BUS_ID_PRIMARY 0

BusFrame* bus_slave_receive(BusHandle* self) {
	BusDriver* driver = (BusDriver*)self->driver;
	if (driver->slave_rxed) {
		driver->slave_rxed = 0;
		return &self->frame_rx;
	}
	return NULL;
}

void bus_slave_send(BusHandle* self, BusFrame* rsp) {
	BusDriver* driver = (BusDriver*)self->driver;

	// Make sure interrupts are disabled. Technically they should
	// be by this point, because bus_slave_send() is called _ONLY_
	// after the slave has received and handled a frame
	UCA0IE = 0;

	// Prepare for transmitting
	const BusFrame* tx_frame = bus_prepare_tx_frame(rsp);
	driver->tx_buf = tx_frame->buf;
	driver->tx_len = tx_frame->len + BUS_OVERHEAD;
	driver->tx_idx = 0;

	// Begin transfer
	// NOTE: TX empty buffer flag needs to be set manually
	if (driver->active_bus == BUS_ID_PRIMARY) {
		RS485_PRI_DIR_TX();
		UCA0IFG = UCTXIFG;
		UCA0IE = UCTXIE;
	}
}

static BusHandle bus_adcs;

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER1_B0_VECTOR
__interrupt void bus_rx_timeout_irq()
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER1_B0_VECTOR))) bus_rx_timeout_irq()
#else
#error Compiler not supported!
#endif
{
	TB1CTL &= ~MC__UPDOWN; // Put into stop mode

	bus_adcs.receive_timeouts++;

	bus_reset_rx(&bus_adcs);

	// Enable RX on bus
	RS485_PRI_DIR_RX();
	UCA0IE = UCRXIE;
}


#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A0_VECTOR
__interrupt void bus_primary_irq()
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A0_VECTOR))) bus_primary_irq()
#else
#error Compiler not supported!
#endif
{
	BusDriver* driver = (BusDriver*)bus_adcs.driver;

    switch(__even_in_range(UCA0IV, USCI_UART_UCTXCPTIFG)) {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG: { // Receive buffer full
        	driver->active_bus = BUS_ID_PRIMARY;

        	// Enable receiver timeout timer
        	TB1CTL |= MC__UP | TBCLR;
			TB1CCTL0 = CCIE;

        	if (bus_handle_rx_byte(&bus_adcs, UCA0RXBUF)) {
        		// Disable timer
				TB1CTL &= ~MC__UPDOWN;
				TB1CCTL0 = 0;

        		// After receiving successfully a frame appointed to our device,
				// disable receiver to make sure rx buffer won't get corrupted.
				// Next function to be called is bus_slave_send().
				UCA0IE = 0;

				// Wake up the main thread.
				driver->slave_rxed = 1;
				interrupt_pending = 1;
				__bic_SR_register_on_exit(LPM0_bits);
        	}
        } break;
        case USCI_UART_UCTXIFG: { // Transmit buffer empty
        	if (driver->tx_idx < driver->tx_len) {
        		UCA0TXBUF = driver->tx_buf[driver->tx_idx++];
        	} else {
        		UCA0IFG &= ~UCTXCPTIE; // NOTE: must be cleared or ISR will trigger on the previous byte
        		UCA0IE = UCTXCPTIE;
        	}
        } break;
        case USCI_UART_UCSTTIFG: { // Start bit received
        	UCA0IFG &= ~UCSTTIFG;
        } break;
        case USCI_UART_UCTXCPTIFG: { // Transmit complete
        	UCA0IE = 0;
        	UCA0IFG &= ~UCTXCPTIE;

        	// Go to receiver mode on bus
        	RS485_PRI_DIR_RX();
			UCA0IE = UCRXIE;
        } break;
        default: break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// Platform initialization and main loop
////////////////////////////////////////////////////////////////////////////////


volatile uint16_t sys_ticks = 0;

unsigned int idle_counter = ~0;
void reset_idle_counter(){
    idle_counter = 0;
}

// Sleep mode indicator flag. Sleep Mode - 0, Enabled - 1
uint8_t sleep_mode = 1;
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

/*
 * Watchdog time configuration
 *
 * Select master clock as the clock source WDT is cleared only in the heartbeat interrupt,
 * so the reset time must be greater than maximum heartbeat period.
 * VLOCLK_frq = 10kHz
 * WDTSSEL__ACLK = 32kHz
 * Reset Time [sec] = (2^X) / CLK_SOURCE
 * See WDTIS for values of X
 * WDTIS__512K = 16s
 */
#ifdef USE_WDT
#define RESET_WDT() (WDTCTL = WDTPW + WDTSSEL__ACLK + WDTIS__32K + WDTCNTCL)
#else
#define RESET_WDT() (WDTCTL = WDTPW + WDTHOLD) // Disabled!
#endif


void configure_clocks() {
    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW;
    FRCTL0 = FRCTLPW | NWAITS_1; // 0xffff; PAOUT=0x0000; // Ports 1 and 2

    // To configure the DCO frequency or FLL reference clock with DCO factory trim, first clear the CSCTL0
    // register. This makes sure that the DCO starts from the lowest frequency to avoid a frequency above
    // specification due to temperature or supply voltage drift over time. Wait at least two MCLK cycles before
    // the FLL is enabled. After the wait cycles, poll the FLLUNLOCK bits to determine if the FLL is locked in the
    // target frequency range.
    // The recommended process to configure the FLL is:
    // 1. Disable the FLL.
    // 2. Select the reference clock.
    // 3. Clear the CSCTL0 register.
    // 4. Set the DCO range and set FLLN and FLLD for target frequency.
    // 5. Execute NOP three times to allow time for the settings to be applied.
    // 6. Enable the FLL.
    // 7. Poll the FLLUNLOCK bits until the FLL is locked.

    // Configure DCO & FLL
    __bis_SR_register(SCG0);    // Disable FLL
    CSCTL3 |= SELREF__REFOCLK;  // Select reference clock source for FLL
    CSCTL0 = 0;                // Clear the CSCTL0 register.
    CSCTL1 |= DCORSEL_5;        // Set DCO range to 16MHz
    CSCTL2 |= FLLD__1 | 243;    // Set target frequency (16MHz) for FLL
                                // DCOCLK = 2^FLLD * (FLLN +1) * (FLLRefclk/n)
                                // DCOCLKDIV = (FLLN +1) * (FLLRefclk/n)
                                // n = FLLREFDIV (fixed to 1 for 32kHz)
                                // 2^1 *(243 + 1) * 32768 Hz = 16MHz
                                // (243 + 1) * 32768 Hz = 8MHz
                                // Set FLL Div = fDCOCLK/1
    // allow time for settings to be applied
    __delay_cycles(3);
    __bic_SR_register(SCG0);    // Enable the FLL control loop


    // Set clock distribution
    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK; // SELA: set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz
                                               // SELMS: MCLK and SMCLK are sourced from DCOCLKDIV, providing a clock frequency DCOCLK÷2

    // Step 7: Poll the FLLUNLOCK bits until the FLL is locked
    while (CSCTL7 & FLLUNLOCK) {
        // FLL is not locked yet
    }
    // Reset Unlock history
    CSCTL7 &= ~FLLUNLOCKHIS;

    // Set reset on drifting clock: If the FLLULPUC bit is set,
    // a reset (PUC) is triggered if FLLULIFG is set.
    CSCTL7 |= FLLULPUC;
}

void init_gpio(void){
    // turn off everything to save power
    PADIR=0xffff; PAOUT=0x0000; // Ports 1 and 2
    // do individual config

    /** Configure PORT1 */
    // P1.0 = VREF              Analog Ref IN
    // P1.1 = VY2               Analog 1 IN
    // P1.2 = DE                RS485 ENable
    // P1.3 = VY1               Analog 3 IN
    // P1.4 = VX2               Analog 4 IN
    // P1.5 = VX1               Analog 5 IN
    // P1.6 = RX                UART RX
    // P1.7 = TX                UART TX
    // if version == (v4 or v4.5):
    // P1.1 = VX1               Analog 1 IN
    // P1.3 = VX2               Analog 3 IN
    // P1.4 = VY1               Analog 4 IN
    // P1.5 = VY2               Analog 5 IN

    // Setup analog inputs
    P1SEL0 |= BIT0 | BIT1 | BIT3 | BIT4 | BIT5;
    P1SEL1 |= BIT0 | BIT1 | BIT3 | BIT4 | BIT5;

    // PMM
    PMMCTL0_H = PMMPW_H; // Unlock PMM
    PMMCTL2 |= INTREFEN; // Enable internal reference
    PMMCTL2 |= TSENSOREN; // Enable internal temperature sensor
    PMMCTL0_H = 0; // Lock PMM

    /** Configure PORT2 */
    // P2.0 = Vopamp          OUT
    // P2.6 = LED             OUT
    // P2.7 = Unconnected pin used for timing analysis
    P2DIR |= BIT0 | BIT6 | BIT7;
    P2OUT = 0; // Opamp and LED are off by default
}


void init_heartbeat_timer() {
	/*
	 * Configure TimerB0
	 *
	 * SMCLK = DCOCLKDIV
	 * Timer frequency(Hz) = SMCLK / (TB0CCR0 * 8 * 8)
	 * TB0CCR0 = SMCLK / (timer_frequency(Hz)*8*8)
	 * add 50 for every 1000, was measured to be more spot on at 16ms at room temp
	 */

	TB0CTL = MC__UP + TBSSEL__SMCLK + ID__8;    // UP mode, SMCLK, divide by 8
	TB0CCR0 = 2100;                             // up to this count
	TB0EX0 = TBIDEX__8;                         // Divide by 8
	TB0CCTL0 = CCIE;                            // TACCR0 interrupt enabled
	TB0CTL |= TBCLR;
	TB0R = 0;
}


/*
 * Timer B0 interrupt (triggering every 16ms)
 */
 #if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer_B0()
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_B0_VECTOR))) Timer_B0()
#else
#error Compiler not supported!
#endif
{
    sys_ticks++; // sys_tick every 16 ms
	// Wake up the main loop
    LED_TOGGLE();
    RESET_WDT();
	__bic_SR_register_on_exit(LPM0_bits);
}



////////////////////////////////////////////////////////////////////////////////
/// Platform initialization and main loop
////////////////////////////////////////////////////////////////////////////////

static void platform_init() {
	__disable_interrupt();

	WDTCTL = WDTPW | WDTHOLD;

	// Pin configuration
	{
		/*
		* 2) Initialize all control IO pins
		*/
		init_gpio();

		// P1.7 = TXD (SEL1.6=0, SEL0.7=1)
		// P1.6 = RXD (SEL1.6=0, SEL0.6=1)
		P1SEL0 |= BIT6 | BIT7;
		P1SEL1 &= ~(BIT6 | BIT7);
		P1DIR = BIT2;
		P1OUT = 0;
	}

	// UART reception timeout timer configuration
	{
		TB1CTL = TBSSEL__SMCLK | ID__8 | MC__UP;
		TB1CCTL0 = 0;
		TB1R = 0;
		TB1CCR0 = 399; // 0.2 msec
	}

	// UART configuration
	{
		// UCA1 (primary)
		UCA0CTLW0 = UCSWRST;                    // Set the state machine to reset
		UCA0CTLW0 |= UCSSEL__SMCLK;             // SMCLK

		// 115200 (see MSP430FR2311 User's Guide page 586)
        // https://e2e.ti.com/support/microcontrollers/msp-low-power-microcontrollers-group/msp430/f/msp-low-power-microcontroller-forum/478726/msp430fr4133-eusci_a-uart-setting-of-ucaxmctlw-register

        UCA0MCTLW |= UCOS16;                    // Oversampling enabled

        // Proper UART 115200 baud rate
        //UCA0BRW = 4;                            // Set the baud rate to 115200 (8MHz)
        //UCA0MCTLW |= UCBRF_5 | 0x5500;          // Modulation UCBRSx=0x55, UCBRFx=5

        // Baud rate of OBC is 3% (111607) slower AND MESSES UP EVERYTHING
		UCA0BRW = 4;                            // Set the baud rate to 115200 (8MHz)
		UCA0MCTLW |= UCBRF_10 | 0xB700;          // Modulation UCBRSx=0xB7, UCBRFx=10

		UCA0CTLW0 &= ~UCSWRST;                  // Release the reset
		UCA0IE |= UCRXIE;                       // Enable USCI_A0 RX interrupt

		// Enable RX
		RS485_PRI_DIR_RX();
		UCA0IE = UCRXIE;
	}



	/*
	 * 1) Initialization of clocks
	 */
	configure_clocks();


	// Disable the GPIO power-on default high-impedance mode to activate
	// previously configured port settings
	PM5CTL0 &= ~LOCKLPM5;

	/*
	 * 3) Initialize other peripherals
	 */

	init_adc();
	init_heartbeat_timer();

	sleepmode();

	/*
	 * 4) Start normal operation aka listen UART and other interrupts.
	 */

	__bis_SR_register(GIE); // Enable interrupts
	__no_operation(); // First chance for the interrupt to fire!

	RESET_WDT();

	LED_ON();
}

static void platform_loop() {
	BusDriver bus_driver = { 0 };
	bus_adcs.driver = &bus_driver;

	for (;;) {

		RESET_WDT();

		// Make sure that all interrupts are serviced before going to sleep
		__disable_interrupt();
		if (!interrupt_pending) {
			__bis_SR_register(LPM0_bits | GIE);
		}
		interrupt_pending = 0;
		__enable_interrupt();

		// Update slave bus
		{
			BusFrame* cmd = bus_slave_receive(&bus_adcs);
			if (cmd != NULL) {
				BusFrame* rsp = bus_get_tx_frame(&bus_adcs);
				handle_command(cmd, rsp);
				bus_slave_send(&bus_adcs, rsp);
			}
		}

		TB0CTL |= TBCLR;

		// Processor wakes up every 16ms --> Goes to sleep after 16ms*250 = 4s
		if (idle_counter > 250 && !sleep_mode) {
			// Goto "deepsleep" if rs485 is not actively used
			sleepmode();
		}
		else {
			idle_counter++;
			// Resets itself after 16ms*1250 = 20s
			if (idle_counter >= 1250) {
				// Trigger POR reset after ~20 seconds of idling
				PMMCTL0 |= PMMSWPOR;
			}
		}
	}
}

int main() {
    platform_init();
    platform_loop();

    return 0;
}
