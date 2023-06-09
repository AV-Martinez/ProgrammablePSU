#include <EEPROM.h>
#include "Globals.h"

GlobalsClass Globals;

void GlobalsClass::Setup(bool forceInit) {
	LoadFromEEPROM(forceInit);
}

void GlobalsClass::LoadFromEEPROM(bool forceInit) {
	EEPROM.get(0, Data);
	if (strcmp(Data.ok, "ok") || forceInit) {
		Serial.print(F("EEPROM init.\n"));
		Data.VoltsTarget  = 3300;
		Data.AmpsLimit    = 500;
		Data.LCDContrast  = 30;
		for (uint8_t s = 0; s < MAX_PGM_STMTS; s++) Data.Stmt[s].line = 0xFF;
		Data.Calibrated   = false;
		Data.Cal_Ioff_A   = 0.0;
		Data.Cal_Ioff_B	  = 0.01;
		Data.Cal_Volt_A   = 0.0;
		Data.Cal_Volt_B   = 17.350928641;
		Data.Cal_Amps_A   = 0.0;
		Data.Cal_Amps_B   = 4.887585533; 
		strcpy_P(Data.ok, PSTR("ok"));
		SaveToEEPROM();
	}
}

void GlobalsClass::SaveToEEPROM() {
	EEPROM.put(0, Data);
}

void GlobalsClass::Double2Str(double value, char *result, uint8_t size) {
	bool pos;
	if (value >= 0) pos = true; else { value = -value; pos = false; }
	uint16_t value_ = (uint16_t)(value);
	uint16_t frac   = (uint16_t)(value*1000.0) - value_*1000;
	snprintf_P(result, size, PSTR("%c%d.%03d"), pos ? '+' : '-', value_, frac);
}

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__
int GlobalsClass::FreeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}
