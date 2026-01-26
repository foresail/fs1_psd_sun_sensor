#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>

extern uint8_t sleep_mode;

#define USE_WDT

// Heartbeat Timer
#define HB_TIMER_ENABLE() TB0CTL |= MC_1
#define HB_TIMER_DISABLE() TB0CTL &= ~MC_1

// OPAMP Enable
#define OPAMP_ON()  do { P2OUT |=  BIT0; } while(0)
#define OPAMP_OFF() do { P2OUT &= ~BIT0; } while(0)

#ifdef DEBUG
// LED
#define LED_ON()    do { P2OUT |=  BIT6; } while(0)
#define LED_OFF()   do { P2OUT &= ~BIT6; } while(0)
#define LED_TOGGLE()do { P2OUT ^=  BIT6; } while(0)

#else
#define LED_ON()
#define LED_OFF()
#define LED_TOGGLE()
#endif

void configure_clocks(void);
void init_gpio(void);
void reset_idle_counter(void);
void sleepmode(void);
void wakeup(void);
void init_heartbeat_timer(void);

#endif /* __MAIN_H__ */
