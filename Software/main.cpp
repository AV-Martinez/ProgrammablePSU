#include <Wire.h>
#include "main.h"
#include "Globals.h"
#include "HW.h"
#include "UI.h"
#include "Program.h"

void ShowCommands() {
	char line[100]; 
	Serial.print(F("\nCommands:\n"));
	snprintf_P(line, sizeof(line), PSTR("  setvolts [millivolts]         (%d)\n"), Globals.Data.VoltsTarget);	 Serial.print(line);
	snprintf_P(line, sizeof(line), PSTR("  setamps  [milliamps]          (%d)\n"), Globals.Data.AmpsLimit);		 Serial.print(line);
	snprintf_P(line, sizeof(line), PSTR("  pgm [list|edit|run|stop|help] (%s)\n"), Program.Running ? "running" : "stopped"); Serial.print(line);
	snprintf_P(line, sizeof(line), PSTR("  read\n")); Serial.print(line);
	snprintf_P(line, sizeof(line), PSTR("  limits\n")); Serial.print(line);
	snprintf_P(line, sizeof(line), PSTR("  calibration\n")); Serial.print(line);
}

void Setup() {
	Serial.begin(115200);
	Globals.Setup(false);

	UI.Setup();

	HW.Setup(A1,A0);
	HW.SetVoltsTarget(Globals.Data.VoltsTarget);
	HW.SetAmpsLimit(Globals.Data.AmpsLimit);
	
	Program.Setup();

	ShowCommands();

	/**
	for(uint8_t address = 1; address < 127; address++) {
		Wire.beginTransmission(address);
		uint8_t error = Wire.endTransmission();
		if (error == 0) {
			Serial.print(F("I2C at addr 0x"));
			if (address<16) Serial.print(F("0"));
			Serial.println(address,HEX);
		}	
	}
	Serial.println(F("I2C scan done."));
	*/
	
}

void Loop() {
	
	HW.Loop();
	UI.Loop();
	Program.Loop();

	while (Serial.available()) {
		char line[100];
		const uint8_t  CmdLength = 20;
		static char    Cmd[CmdLength];
		static uint8_t CmdIx = 0;
		char c = Serial.read();
		if (c == '\n' || c == '\r' || CmdIx == CmdLength-1) {
			Cmd[CmdIx] = '\0';
			Serial.print(F(">>> ")); Serial.println(Cmd);
			char *command = strtok(Cmd, " ");
			if (!strcmp_P(command, PSTR("read"))) {
				uint16_t millivolts, milliamps;
				uint16_t adcV, adcI;
				HW.GetReadings(&millivolts, &milliamps, &adcV, &adcI);
				snprintf_P(line, sizeof(line), PSTR("V=%dmV I=%dmA adcV=%d adcI=%d\n"), millivolts, milliamps, adcV, adcI);
				Serial.print(line);
			}
			else if (!strcmp_P(command, PSTR("setvolts"))) {
				uint16_t volts = atoi(strtok(NULL, " "));
				if (volts) {
					uint16_t roundValue = VOLTS_STEP_VALUE*round((1.0*volts)/(1.0*VOLTS_STEP_VALUE));
					if (roundValue >= VOLTS_MIN_VALUE && roundValue <= VOLTS_MAX_VALUE) {
						Globals.Data.VoltsTarget = roundValue;
						HW.SetVoltsTarget(Globals.Data.VoltsTarget);
						Globals.SaveToEEPROM();
					}
					else
						Serial.println(F("Value out of range"));
				}
				else
					Serial.println(F("Missing value"));
			}
			else if (!strcmp_P(command, PSTR("setamps"))) {
				uint16_t amps = atoi(strtok(NULL, " "));
				if (amps) {
					uint16_t roundValue = AMPS_STEP_VALUE*round((1.0*amps)/(1.0*AMPS_STEP_VALUE));
					if (roundValue >= AMPS_MIN_VALUE && roundValue <= AMPS_MAX_VALUE) {
						Globals.Data.AmpsLimit = roundValue;
						HW.SetAmpsLimit(Globals.Data.AmpsLimit);
						Globals.SaveToEEPROM();
					}
					else
						Serial.println(F("Value out of range"));
				}
				else
					Serial.println(F("Missing value"));
			}
			else if (!strcmp_P(command, PSTR("limits"))) {
				snprintf_P(line, sizeof(line), PSTR("millivolts: %d...%d (step %d)\nmilliamps:  %d...%d  (step %d)\n"), VOLTS_MIN_VALUE, VOLTS_MAX_VALUE, VOLTS_STEP_VALUE, AMPS_MIN_VALUE, AMPS_MAX_VALUE, AMPS_STEP_VALUE);
				Serial.print(line);
			}
			else if (!strcmp_P(command, PSTR("pgm"))) {
				char *action = strtok(NULL, " ");
				if  	(!strcmp_P(action, PSTR("edit"))) Program.Edit();
				else if (!strcmp_P(action, PSTR("list"))) Program.List();
				else if (!strcmp_P(action, PSTR("run")))  Program.Running = true;
				else if (!strcmp_P(action, PSTR("stop"))) Program.Running = false;
				else if (!strcmp_P(action, PSTR("help"))) {
					Serial.print(F("\nSyntax:\n"));
					Serial.print(F("  [line number] [statement]\n"));
					Serial.print(F("Valid statements:\n"));
					Serial.print(F("  setvolts  [millivolts]\n"));
					Serial.print(F("  setamps   [milliamps]\n"));
					Serial.print(F("  waitvolts [<|>] [millivolts]\n"));
					Serial.print(F("  waitamps  [<|>] [milliamps]\n"));
				}
			}
			else if (!strcmp_P(command, PSTR("calibration"))) {
				char dstr[10]; 
				snprintf_P(line, sizeof(line), PSTR("Calibrated: %c\n"), Globals.Data.Calibrated ? 'T' : 'F'); Serial.print(line);
				Globals.Double2Str(Globals.Data.Cal_Ioff_A, dstr, 10); snprintf_P(line, sizeof(line), PSTR("Cal_Ioff_A= %s\n"), dstr); Serial.print(line);
				Globals.Double2Str(Globals.Data.Cal_Ioff_B, dstr, 10); snprintf_P(line, sizeof(line), PSTR("Cal_Ioff_B= %s\n"), dstr); Serial.print(line);
				Globals.Double2Str(Globals.Data.Cal_Volt_A, dstr, 10); snprintf_P(line, sizeof(line), PSTR("Cal_Volt_A= %s\n"), dstr); Serial.print(line);
				Globals.Double2Str(Globals.Data.Cal_Volt_B, dstr, 10); snprintf_P(line, sizeof(line), PSTR("Cal_Volt_B= %s\n"), dstr); Serial.print(line);
				Globals.Double2Str(Globals.Data.Cal_Amps_A, dstr, 10); snprintf_P(line, sizeof(line), PSTR("Cal_Amps_A= %s\n"), dstr); Serial.print(line);
				Globals.Double2Str(Globals.Data.Cal_Amps_B, dstr, 10); snprintf_P(line, sizeof(line), PSTR("Cal_Amps_B= %s\n"), dstr); Serial.print(line);
				HW.CalibrationProcess();
			}
			ShowCommands();
			CmdIx = 0;
		}
		else 
			Cmd[CmdIx++] = c;
	}
}

