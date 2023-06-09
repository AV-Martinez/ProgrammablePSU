#include "UI.h"
#include "ST7032.h"
#include "InputDevice.h"
#include "Globals.h"
#include "HW.h"
#include "Program.h"

#define MODE_RUNNING  	0
#define MODE_SETVOLTS 	1
#define MODE_SETAMPS  	2
#define MODE_MENU		3

UIClass UI;

void UIClass::Setup() {
	InputDevice.Setup(4,2,3);
	LCD.begin(16,2);
	LCD.setContrast(Globals.Data.LCDContrast);
	
	char cter[8] = {0, B11111, 0, B11111, 0, B11111, 0, 0};
	LCD.createChar(0, cter);

	MenuIx = 0;
	Mode   = MODE_RUNNING;
	EnterMode();
	
	pinMode(PIN_CL_LED, OUTPUT);
	digitalWrite(PIN_CL_LED, LOW);
}

void UIClass::Loop() {
	static uint32_t flashticks = 0;
	
	if (Mode == MODE_SETVOLTS || Mode == MODE_SETAMPS) {
		if (flashticks == 0 || millis()-flashticks > 333) {
			flashticks = millis();
			FlashValue = !FlashValue;
			if (Mode == MODE_SETVOLTS) DisplayVoltsTarget();
			else 				       DisplayAmpsLimit();
		}
	}
	
	if (Mode == MODE_MENU) {
		if (flashticks == 0 || millis()-flashticks > 333) {
			flashticks = millis();
			FlashValue = !FlashValue;
			if (MenuIx == 0) 
				DisplayContrastValue();
			else 
				DisplayPgmStatValue();
		}
	}
	
	bool encEvent, pbEvent;
	InputDevice.Loop(&encEvent, &pbEvent);
	if (encEvent) {
		if (Mode == MODE_SETVOLTS) {
			Globals.Data.VoltsTarget += VOLTS_STEP_VALUE * (InputDevice.EncoderDir == 'D' ? -1 : +1);
			if (Globals.Data.VoltsTarget >= VOLTS_MAX_VALUE) Globals.Data.VoltsTarget = VOLTS_MAX_VALUE;
			if (Globals.Data.VoltsTarget <= VOLTS_MIN_VALUE) Globals.Data.VoltsTarget = VOLTS_MIN_VALUE;
			HW.SetVoltsTarget(Globals.Data.VoltsTarget);
		}
		else if (Mode == MODE_SETAMPS) {
			Globals.Data.AmpsLimit += AMPS_STEP_VALUE * (InputDevice.EncoderDir == 'D' ? -1 : +1);
			if (Globals.Data.AmpsLimit >= AMPS_MAX_VALUE) Globals.Data.AmpsLimit = AMPS_MAX_VALUE;
			if (Globals.Data.AmpsLimit <= AMPS_MIN_VALUE) Globals.Data.AmpsLimit = AMPS_MIN_VALUE;
			HW.SetAmpsLimit(Globals.Data.AmpsLimit);
		}
		else if (Mode == MODE_MENU) {
			if (MenuIx == 0) {
				if (InputDevice.EncoderDir == 'D') {
					if (Globals.Data.LCDContrast > 15) Globals.Data.LCDContrast -= 1;
				}
				else {
					if (Globals.Data.LCDContrast < 50) Globals.Data.LCDContrast += 1;
				}
				LCD.setContrast(Globals.Data.LCDContrast);
				Globals.SaveToEEPROM();
			}
			else 
				ProgramRunning = !ProgramRunning;
		}
	}
	if (pbEvent) {
		if (InputDevice.PushButtonPress == 'S') {
			if (Mode == MODE_MENU) {
				FlashValue = true;
				if (MenuIx == 0) {
					DisplayContrastValue();
					MenuIx = 1;
				}
				else {
					DisplayPgmStatValue();
					MenuIx = 0;
				}
			}
			else {
				FlashValue = true;
				if (Mode == MODE_SETVOLTS) DisplayVoltsTarget();
				if (Mode == MODE_SETAMPS)  DisplayAmpsLimit();
				Mode = (Mode+1) % 3;
				if (Mode == MODE_RUNNING) Globals.SaveToEEPROM();
			}
		}
		else {
			if (Mode != MODE_MENU) { 
				ProgramRunning = Program.Running;
				Mode = MODE_MENU; EnterMode(); 
			}
			else if (Mode == MODE_MENU) { 
				Program.Running = ProgramRunning;
				Globals.SaveToEEPROM(); 
				Mode = MODE_RUNNING; EnterMode(); 
			}
		}
	}
}

void UIClass::SetLimitingCurrentMode(bool lc) {
	digitalWrite(PIN_CL_LED, lc);
	if (Mode == MODE_MENU) return;
	LimitingCurrent = lc;
	if (LimitingCurrent) {
		LCD.setCursor(7,0); LCD.print(F("<"));
		LCD.setCursor(7,1); LCD.write(0); // LCD.print(F("="));
	}
	else {
		LCD.setCursor(7,0); LCD.write(0); // LCD.print(F("="));
		LCD.setCursor(7,1); LCD.print(F("<"));
	}
}

void UIClass::DisplayAmpsReading(uint16_t milliamps, bool force) {
	if (Mode == MODE_MENU) return;
	uint16_t roundValue = AMPS_STEP_VALUE*round((1.0*milliamps)/(1.0*AMPS_STEP_VALUE));
	static int16_t lastValue = -1;
	if (roundValue == lastValue && !force) return;
	lastValue = roundValue;
	char line[10]; snprintf_P(line, sizeof(line), PSTR("%4d"), lastValue);
	LCD.setCursor(2,1); LCD.print(line);
}

void UIClass::DisplayVoltsReading(uint16_t millivolts, bool force) {
	if (Mode == MODE_MENU) return;
	uint16_t roundValue = VOLTS_STEP_VALUE*round((1.0*millivolts)/(1.0*VOLTS_STEP_VALUE));
	static int16_t lastValue = -1;
	if (roundValue == lastValue && !force) return;
	lastValue = roundValue;
	char line[10]; snprintf_P(line, sizeof(line), PSTR("%s"), MilliVolts2Volts(roundValue));
	LCD.setCursor(2,0); LCD.print(line);
}

void UIClass::DisplayVoltsTarget() {
	if (Mode == MODE_MENU) return;
	char line[20]; 
	if (FlashValue)
		snprintf_P(line, sizeof(line), PSTR("%s V"), MilliVolts2Volts(Globals.Data.VoltsTarget));
	else
		snprintf_P(line, sizeof(line), PSTR("      "));
	LCD.setCursor(9,0); LCD.print(line);
}

void UIClass::DisplayAmpsLimit() {
	if (Mode == MODE_MENU) return;
	char line[20]; 
	if (FlashValue) 
		snprintf_P(line, sizeof(line), PSTR("%4d mA"), Globals.Data.AmpsLimit);
	else 
		snprintf_P(line, sizeof(line), PSTR("       "));
	LCD.setCursor(9,1); LCD.print(line);
}

char *UIClass::ReadFromSerial() {
	const uint8_t maxlen = 10;
	static char line[maxlen]; 
	uint8_t ix = 0;
	while (true) {
		if (Serial.available()) {
			char c = Serial.read();
			if (c == '\n' || c == '\r') break;
			line[ix++] = c;
			if (ix == maxlen-1) break;
		}
	}
	line[ix] = '\0';
	while (Serial.available()) Serial.read();
	return line;
}

char *UIClass::MilliVolts2Volts(uint16_t milliVolts) {
	static char r[10];
	uint8_t  vint  = milliVolts / 1000;
	uint16_t vfrac = milliVolts - vint*1000;
	if (vint < 10)
		snprintf_P(r, sizeof(r), PSTR("%d.%02d"), vint, vfrac/10);
	else
		snprintf_P(r, sizeof(r), PSTR("%2d.%d"), vint, vfrac/100);
	return r;
}

void UIClass::EnterMode() {
	LCD.clear();
	FlashValue = true; 
	if (Mode == MODE_RUNNING) {
		LCD.setCursor(0,0); LCD.print(Globals.Data.Calibrated ? F("V") : F("v"));
		LCD.setCursor(0,1); LCD.print(Globals.Data.Calibrated ? F("I") : F("i"));
		SetLimitingCurrentMode(HW.Limiting);
		uint16_t millivolts, milliamps;
		HW.GetReadings(&millivolts, &milliamps);
		DisplayVoltsTarget();
		DisplayVoltsReading(millivolts, true);
		DisplayAmpsLimit();
		DisplayAmpsReading(milliamps, true);
	}
	else if (Mode == MODE_MENU) {
		MenuIx = 0;
		LCD.setCursor(0,0); LCD.print(F("Contrast:")); 	DisplayContrastValue();
		LCD.setCursor(0,1); LCD.print(F("Pgm:"));		DisplayPgmStatValue();
	}
}

void UIClass::DisplayContrastValue() {
	char line[10]; 
	if (FlashValue) 
		snprintf_P(line, sizeof(line), PSTR("%3d"), Globals.Data.LCDContrast);
	else 
		snprintf_P(line, sizeof(line), PSTR("   "));
	LCD.setCursor(9,0); LCD.print(line);
}

void UIClass::DisplayPgmStatValue() {
	char line[10]; 
	if (FlashValue) 
		snprintf_P(line, sizeof(line), PSTR("%s"), ProgramRunning ? "Running" : "Stopped");
	else 
		snprintf_P(line, sizeof(line), PSTR("       "));
	LCD.setCursor(5,1); LCD.print(line);
}
