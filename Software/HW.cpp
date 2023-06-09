#include "HW.h"
#include "Globals.h"
#include "UI.h"

HWClass HW;

void HWClass::Setup(uint8_t pinVADC, uint8_t pinIADC) {
	PinVADC = pinVADC;
	PinIADC = pinIADC;

	DDRB |= 0b00011111;	/// DAC pin connections as output
	DDRD |= 0b11100000; /// DAC pin connections as output

	Limiting = false;
}

void HWClass::Loop() {
	static uint32_t ticks = 0;
	uint32_t now = millis();
	
	if (ticks == 0 || now-ticks > 10) {
		ticks = now;

		uint16_t millivolts, milliamps;
		GetReadings(&millivolts, &milliamps);

		static uint32_t updateTicks = 0;
		if (updateTicks == 0 || now - updateTicks > 250) {
			updateTicks = now;
			UI.DisplayVoltsReading(millivolts);
			UI.DisplayAmpsReading(milliamps);
		}
		
		static uint8_t voltsAtLimiting = 0;
		if (milliamps < Globals.Data.AmpsLimit) { /// No current limiting
			if (Limiting) {
				if (voltsAtLimiting < 255) SetDACValue(++voltsAtLimiting); else return;
				if (voltsAtLimiting >= VoltsTarget_) {
					Limiting = false;
					UI.SetLimitingCurrentMode(false);
				}
				else return;
			}
		}
		else { /// Current limiting
			if (!Limiting) {
				Limiting = true; 
				voltsAtLimiting = VoltsTarget_;
				UI.SetLimitingCurrentMode(true);
			}
			if (voltsAtLimiting > 0) SetDACValue(--voltsAtLimiting); else return;
		}
	}
}

void HWClass::SetVoltsTarget(uint16_t milliVolts) {
	Globals.Data.VoltsTarget = milliVolts;
	VoltsTarget_ = (uint8_t)((milliVolts/1000.0) * DAC_2_Vout); 
	SetDACValue(VoltsTarget_);
	delay(100);
	UI.DisplayVoltsTarget();
	uint16_t millivolts, milliamps;
	GetReadings(&millivolts, &milliamps);
	UI.DisplayVoltsReading(millivolts);
	UI.DisplayAmpsReading(milliamps);
}

void HWClass::SetAmpsLimit(uint16_t milliAmps){
	Globals.Data.AmpsLimit = milliAmps;
	UI.DisplayAmpsLimit();
}

void HWClass::GetADCs(uint16_t *adcV, uint16_t *adcI) {
	*adcV = analogRead(PinVADC);
	*adcI = analogRead(PinIADC);
}

void HWClass::GetReadings(uint16_t *volts, uint16_t *amps, uint16_t *_adcV, uint16_t *_adcI) {
	uint16_t adcV = analogRead(PinVADC);
	uint16_t adcI = analogRead(PinIADC);

	*volts = Globals.Data.Cal_Volt_A + adcV*Globals.Data.Cal_Volt_B;
	
	uint16_t iOff = Globals.Data.Cal_Ioff_A + adcV*Globals.Data.Cal_Ioff_B;
	if (adcI <= iOff)
		*amps = 0;
	else {
		int16_t amps_ = Globals.Data.Cal_Amps_A + (adcI-iOff)*Globals.Data.Cal_Amps_B;
		if (amps_ >= 0) *amps = (uint16_t)amps_; else *amps = 0;
	}
		
	if (_adcV != NULL) *_adcV = adcV;
	if (_adcI != NULL) *_adcI = adcI;

}

void HWClass::SetDACValue(uint8_t value) {
	for (int8_t bit = 7; bit >= 0; bit--) /// Set MSB asap for fastest response
		if (value & (1<<bit)) 
			switch (bit) {
				case 7: PORTB |= (1<<4); break; 
				case 6: PORTB |= (1<<3); break; 
				case 5: PORTB |= (1<<2); break; 
				case 4: PORTB |= (1<<1); break; 
				case 3: PORTB |= (1<<0); break; 
				case 2: PORTD |= (1<<7); break; 
				case 1: PORTD |= (1<<6); break; 
				case 0: PORTD |= (1<<5); break; 
			}
		else 
			switch(bit) {
				case 7: PORTB &=~ (1<<4); break;
				case 6: PORTB &=~ (1<<3); break;
				case 5: PORTB &=~ (1<<2); break;
				case 4: PORTB &=~ (1<<1); break;
				case 3: PORTB &=~ (1<<0); break;
				case 2: PORTD &=~ (1<<7); break;
				case 1: PORTD &=~ (1<<6); break;
				case 0: PORTD &=~ (1<<5); break;
			}
}

/** Calibration process
 * 
 * 	The purpose of the calibration process is to calculate adequate linear regression parameters
 *  to derive Volts from adcV readings and Amps from adcI readings. 
 * 
 * 	Calibration is performed in two steps, first with load and then without load. This allows to 
 * 	ensure that the no load test happens with a discharged output capacitor.
 * 
 *	1. Load test:
 * 		* A load of given ohms and watts is connected.
 * 		* Voltage varies from VOLTS_MIN_VALUE to the max acceptable voltage for the load in nPoints
 * 		* Data captured at each step:
 * 			Measured volts from user (load_readV)
 * 			Measured amps from user (load_readI)
 * 			adcV values (load_adcV)
 * 			adcI values (load_adcI)
 * 
 * 	2. No load test:
 * 		* Any load is disconnected.
 * 		* Voltage varies from VOLTS_MIN_VALUE to VOLTS_MAX_VALUE in nPoints
 * 		* Data captured at each step:
 * 			Measured volts from user (open_readV)
 * 			adcV values (open_adcV)
 * 			adcI values (open_adcI)
 * 
 * 	3. Calculate the adcV to Volts regression using open_* data
 * 
 * 	4. Calculate the adcV to adcI regression using open_* data. This is the adcI offset when no load
 * 	   is connected, and is the value that must be substracted from an adcI reading under load. 
 * 
 * 	5. Calculate the adcI to Amps regression using load* data. The adcI data used to calculate the
 *     regression parameters results from substracting from the adcI value the adcI offset calculated
 *     in the previous step.
*/

void HWClass::CalibrationProcess() {
	bool earlyQuit = false;
	const uint8_t nPoints = 5;

	/// Readings:
	uint16_t load_adcV[nPoints], load_adcI[nPoints], load_readV[nPoints], load_readI[nPoints];
	uint16_t open_adcV[nPoints], open_adcI[nPoints], open_readV[nPoints]; 
	
	char *rfs;
	char line[100];
	Serial.println();
	Serial.print(F("* Blank input exits.\n"));
	Serial.print(F("* Step 1/4. Input load's ohms: ")); rfs = UI.ReadFromSerial();
	if (atof(rfs) > 0) {
		Serial.println(rfs);
		double loadR = atof(rfs);
		Serial.print(F("* Step 2/4. Input load's max watts: ")); rfs = UI.ReadFromSerial();
		if (atof(rfs) > 0) {
			Serial.println(rfs);
			double loadW  = atof(rfs); 					
			uint16_t vMax = (uint16_t)(1000.0*(sqrt(loadW/loadR) * loadR)); 
			uint16_t iMax = (uint16_t)((vMax*1.1)/loadR); /// vMax*1.1 so amps limit is not reached during test
			snprintf_P(line, sizeof(line), PSTR("  Max millivolts=%d\n"), vMax); Serial.print(line);
			snprintf_P(line, sizeof(line), PSTR("  Max milliamps =%d\n"), iMax); Serial.print(line);
			HW.SetAmpsLimit(iMax); 
			Serial.print(F("* Step 3/4. Connect load. Then press Enter\n")); UI.ReadFromSerial();
			uint8_t ix = 0; 
			for (uint16_t v = VOLTS_MIN_VALUE; v < vMax; v += (vMax-VOLTS_MIN_VALUE)/(nPoints-1)) {
				HW.SetVoltsTarget(v); delay(100);
				HW.GetADCs(&load_adcV[ix], &load_adcI[ix]);
				snprintf_P(line, sizeof(line), PSTR("  M%d/%d @%d adcv=%d adci=%d \n"), ix+1, nPoints, v, load_adcV[ix], load_adcI[ix]);
				Serial.print(line);
				Serial.print(F("* Millivolts reading: ")); rfs = UI.ReadFromSerial();
				if (atoi(rfs) == 0) { earlyQuit = true; break; } Serial.println(rfs); load_readV[ix] = atoi(rfs);
				Serial.print(F("* Milliamps reading: ")); rfs = UI.ReadFromSerial();
				if (atoi(rfs) == 0) { earlyQuit = true; break; } Serial.println(rfs); load_readI[ix] = atoi(rfs);
				ix++;
			}
			HW.SetVoltsTarget(VOLTS_MIN_VALUE);
			HW.SetAmpsLimit(500);
			if (!earlyQuit) { 
				Serial.print(F("* Step 4/4. Disconnect load. Then press Enter\n")); UI.ReadFromSerial();
				ix = 0; earlyQuit = false;
				Serial.print(F("  "));
				for (uint16_t v = VOLTS_MIN_VALUE; v <= VOLTS_MAX_VALUE; v += (VOLTS_MAX_VALUE-VOLTS_MIN_VALUE)/(nPoints-1)) {
					HW.SetVoltsTarget(v); delay(100); 
					HW.GetADCs(&open_adcV[ix], &open_adcI[ix]);
					snprintf_P(line, sizeof(line), PSTR("  M%d/%d @%d adcv=%d adci=%d\n"), ix+1, nPoints, v, open_adcV[ix], open_adcI[ix]);
					Serial.print(line);
					Serial.print(F("* Millivolts reading: ")); rfs = UI.ReadFromSerial();
					if (atoi(rfs) == 0) { earlyQuit = true; break; } Serial.println(rfs); open_readV[ix] = atoi(rfs);
					ix++;
				} 
				HW.SetVoltsTarget(VOLTS_MIN_VALUE);
				if (!earlyQuit) {
					Serial.print(F("adcV(x) to V(y) regression: "));
					LinearRegression(open_adcV, open_readV, nPoints, &Globals.Data.Cal_Volt_A, &Globals.Data.Cal_Volt_B);
					for (uint8_t i = 0; i < nPoints; i++) {
						uint16_t cv = Globals.Data.Cal_Volt_A + open_adcV[i]*Globals.Data.Cal_Volt_B;
						double   dpct = 100.0*((double)cv-(double)open_readV[i])/(double)open_readV[i];
						char dpct_[10]; Globals.Double2Str(dpct, dpct_, 10);
						snprintf_P(line, sizeof(line), PSTR("  %d/%d adcV=%4d Vread=%5d V(est)=%5d (%s%%)\n"), i+1, nPoints, open_adcV[i], open_readV[i], cv, dpct_); 
						Serial.print(line);
					}
					Serial.print(F("adcV(x) to adcI_open(y) regression: "));
					LinearRegression(open_adcV, open_adcI, nPoints, &Globals.Data.Cal_Ioff_A, &Globals.Data.Cal_Ioff_B);
					for (uint8_t i = 0; i < nPoints; i++) {
						uint16_t cv = Globals.Data.Cal_Ioff_A + open_adcV[i]*Globals.Data.Cal_Ioff_B;
						double   dpct = 100.0*((double)cv-(double)open_adcI[i])/(double)open_adcI[i];
						char dpct_[10]; Globals.Double2Str(dpct, dpct_, 10);
						snprintf_P(line, sizeof(line), PSTR("  %d/%d adcV=%4d adcI=%4d adcI(est)=%d (%s%%)\n"), i+1, nPoints, open_adcV[i], open_adcI[i], cv, dpct_); 
						Serial.print(line);
					}  
					Serial.print(F("adcI_load(x) to I(y) regression: "));
					for (uint8_t i = 0; i < nPoints; i++) {
						uint16_t Ioff = Globals.Data.Cal_Ioff_A + load_adcV[i]*Globals.Data.Cal_Ioff_B;
						load_adcI[i] -= Ioff;
					}
					LinearRegression(load_adcI, load_readI, nPoints, &Globals.Data.Cal_Amps_A, &Globals.Data.Cal_Amps_B);
					for (uint8_t i = 0; i < nPoints; i++) {
						uint16_t cv = Globals.Data.Cal_Amps_A + load_adcI[i]*Globals.Data.Cal_Amps_B;
						double   dpct = 100.0*((double)cv-(double)load_readI[i])/(double)load_readI[i];
						char dpct_[10]; Globals.Double2Str(dpct, dpct_, 10);
						snprintf_P(line, sizeof(line), PSTR("  %d/%d adcI=%4d Iread=%5d I(est)=%5d (%s%%)\n"), i+1, nPoints, load_adcI[i], load_readI[i], cv, dpct_); 
						Serial.print(line);
					}
					Globals.Data.Calibrated = true;
					Globals.SaveToEEPROM();
					UI.EnterMode(); /// refresh v/i showing to V/I
				}
			}
			Serial.print(F("*** Done"));
		}
		else Serial.print(F("Bad value. Exiting.\n")); 
	}
	else Serial.print(F("Bad value. Exiting\n")); 
}

void HWClass::LinearRegression(uint16_t *x, uint16_t *y, uint8_t n, double *a, double *b) {
/**
 * Y = a + b*X
 * 
 * b = (n*(Sxy) - (Sx)*(Sy)) / (n*(Sx²)-(Sx)²)
 * a = ((Sy)*(Sx²) - (Sx)*(Sxy)) / (n*(Sx²) - (Sx)²)   or
 * a = ((Sy) - b*(Sx)) / n
*/
	uint32_t Sxy = 0;
	uint32_t Sx  = 0;
	uint32_t Sy  = 0;
	uint32_t Sx2 = 0;
	for (uint8_t i = 0; i < n; i++) {
		uint32_t x_ = (uint32_t)x[i];
		uint32_t y_ = (uint32_t)y[i];
		Sx  += x_;
		Sy  += y_;
		Sxy += x_*y_;
		Sx2 += x_*x_;
	}
	if (false) {
		Serial.print("n   "); Serial.println(n);
		Serial.print("Sx  "); Serial.println(Sx);
		Serial.print("Sy  "); Serial.println(Sy);
		Serial.print("Sxy "); Serial.println(Sxy);
		Serial.print("Sx2 "); Serial.println(Sx2);
	}
	*b = (double)(n*Sxy-Sx*Sy) / (double)(n*Sx2- Sx*Sx);
	*a = (Sy - (*b)*Sx) / (double)n;
	Serial.print("  Y = "); Serial.print(*a); Serial.print(" + "); Serial.print(*b); Serial.println(" * X");
}

