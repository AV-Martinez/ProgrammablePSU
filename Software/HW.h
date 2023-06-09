#ifndef _HW_H_
#define _HW_H_
#include <Arduino.h>

#define VOLTS_MAX_VALUE 15000	/** Max target volts setting value, millivolts	*/
#define VOLTS_MIN_VALUE   500	/** Min target volts setting value, millivolts 	*/
#define VOLTS_STEP_VALUE  100 	/** Volts stepping value						*/
#define AMPS_MAX_VALUE   4500	/** Max limit amps setting value, milliaps		*/
#define AMPS_MIN_VALUE    100	/** Min limit amps setting value, milliamps		*/
#define AMPS_STEP_VALUE	   50	/** Amps stepping value							*/

/** R-2R ladder DAC pin connections.
 * 	MSB (bit 7) is the one closest to ladder output.
 * 
 *     		Rst 1    28 PC5
 *     		 Rx 2    27 PC4
 *     		 Tx 3    26 PC3
 *     		PD2 4    25 PC2
 *     		PD3 5    24 PC1
 *     		PD4 6    23 PC0
 *     		Vcc 7    22 Gnd
 *     		Gnd 8    21 Arf
 *     		XTL 9    20 Vcc
 *     		XTL 10   19 PB5
 * 		(1) PD5 11   18 PB4 (6)
 * 		(0) PD6 12   17 PB3 (4)
 * 		(2) PD7 13   16 PB2 (5)
 * 		(3) PB0 14   15 PB1 (7)
*/

/**	Output voltage divider and DAC feedback
 * 
 * 	R1 = 51200 (top resistor, 51K nominal)
 *  R2 = 19900 (bottom resistor, 20K nominal)
 * 
 *  At the opamp controling MOSFET gate: V+ == V-
 * 
 * 		V+ = Vout * R2 / (R1+R2)
 * 		V- = DAC * 5 / 255
 * 		DAC = Vout * (255/5) * R2/(R1+R2)
 * 		DAC = Vout * 14.27
*/ 
#define DAC_2_Vout 14.27

/** Voltage and Current measurement
 *
 * 	Voltage reading (V) is derived from the ADC reading (ADCv) at the output
 * 	voltage divider. Theoretically, it should be equal to the voltage
 * 	set through the DAC, but the output capacitor discharge rate will
 *  make it differ until discharge is complete (which, in turns, depends
 *  on the connected load). Analyzing,
 * 
 * 			V = ADCv*(5000/1023) * (R1+R2)/R2
 * 			V = ADCv * 17.46268
 * 
 * 	In practice, and for better precision, V is calculated in the
 *  calibration process as a linear regression to ADCv:
 * 
 * 			V = A + B * ADCv
 * 
 * 	Coefficients A and B are calculated in the calibration process and
 * 	stored as:
 * 
 * 			Globals.Data.Cal_Volt_A
 * 			Globals.Data.Cal_Volt_B
 * 
 * 	Current reading (I) is derived from the ADC reading (ADCi) provided at
 * 	the output of the gain differential amplifier, which is actually measuring
 *  the voltage drop across the 0.1R shunt resistor. The differential
 * 	amplifier is operating at 10x gain. Theoretically,
 * 
 * 			I = ((ADCi * (5000/1023)) / 10) / 0.1
 * 
 * 	Due to tolerances, there is some ADCi reading even when no load
 * 	is connected, and this reading is linearly dependant on the ADCv
 * 	value. So, to measure true I, the ADCi reading is corrected by:
 * 
 * 			ADCi_ = A + B * ADCv
 * 
 * 	And thus:
 * 
 * 			I = 4.887586 * (ADCi - ADCi_)
 * 
 * 	Coefficients A and B are calculated in the calibration process and
 * 	stored as:
 * 
 * 			Globals.Data.Cal_Ioff_A
 * 			Globals.Data.Cal_Ioff_B
 * 	
 * 	The dependency of the I measure to make an ADCv read first, which can
 * 	then be used to measure V itself, suggests to always measure I and V
 *  at the same time, incurring in two ADC readings.
*/

class HWClass {
public:
	void 	 Setup			 (uint8_t pinVADC, uint8_t pinIADC);
	void	 Loop			 ();

	void 	 SetVoltsTarget	 (uint16_t milliVolts);
	void 	 SetAmpsLimit	 (uint16_t milliAmps);

	void	 GetADCs		 (uint16_t *adcV,  uint16_t *adcI);
	void	 GetReadings	 (uint16_t *volts, uint16_t *amps, uint16_t *adcV = NULL, uint16_t *adcI = NULL);

	void	 CalibrationProcess();
	void	 LinearRegression(uint16_t *x, uint16_t *y, uint8_t n, double *a, double *b); /// Y = a + b*X
	
	bool     Limiting;
	
private:

	void SetDACValue(uint8_t value);
	
	uint8_t  PinVADC, PinIADC;
	
	uint8_t  VoltsTarget_;	/// Calculated DAC value (0..255) corresponding to Data.VoltsTarget
	
};

extern HWClass HW;

#endif
