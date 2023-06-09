#ifndef _UI_H_
#define _UI_H_
#include <Arduino.h>

#define PIN_CL_LED 13

class UIClass {
public:
	void Setup();
	void Loop();
	void SetLimitingCurrentMode(bool lc);

	void DisplayVoltsReading(uint16_t millivolts, bool force = false);
	void DisplayAmpsReading(uint16_t milliamps, bool force = false);

	void DisplayVoltsTarget();
	void DisplayAmpsLimit();
	
	char *ReadFromSerial();
	void EnterMode();
	
private:
	uint8_t Mode;
	uint8_t MenuIx;
	bool	ProgramRunning;
	
	char *MilliVolts2Volts(uint16_t milliVolts);
	
	bool FlashValue;
	bool LimitingCurrent;
	
	void DisplayContrastValue();
	void DisplayPgmStatValue();
};

extern UIClass UI;

#endif
