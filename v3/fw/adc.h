#ifndef __ADC_H__
#define __ADC_H__


/*
 * Enable and initialize internal ADC
 */
void init_adc(void);


/*
 *
 */
void read_voltage_channels();

/*
 * Sample internal temperature sensor.
 * This command will wait for the measurement to happen and it will take few ticks
 */
int read_temperature();


#endif /* __ADC_H__ */
