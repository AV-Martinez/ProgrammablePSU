#include <EEPROM.h>
#include "Program.h"
#include "Globals.h"
#include "HW.h"

ProgramClass Program;

void ProgramClass::Setup() {
	Running    = false;
	InWaitStmt = false;
}

void ProgramClass::Loop() {
	if (!Running) return;
	if (InWaitStmt) {
		uint16_t millivolts, milliamps;
		HW.GetReadings(&millivolts, &milliamps);
		bool cd;
		switch (Globals.Data.Stmt[PgmCounter].stmt) {
			case STMT_WAITVOLTS_LESS:	cd = millivolts <= Globals.Data.Stmt[PgmCounter].value; break;
			case STMT_WAITVOLTS_MORE:	cd = millivolts >= Globals.Data.Stmt[PgmCounter].value; break;
			case STMT_WAITAMPS_LESS:	cd = milliamps  <= Globals.Data.Stmt[PgmCounter].value; break;
			case STMT_WAITAMPS_MORE:	cd = milliamps  >= Globals.Data.Stmt[PgmCounter].value; break;
		}
		if (cd) {
			InWaitStmt = false;
			PgmCounter++;
		}
		return;
	}
	while (Globals.Data.Stmt[PgmCounter].line == 0xFF && PgmCounter < MAX_PGM_STMTS) PgmCounter++;
	if (PgmCounter == MAX_PGM_STMTS) {
		Running = false;
		return;
	}
	switch (Globals.Data.Stmt[PgmCounter].stmt) {
		case STMT_SETVOLTS: 
			HW.SetVoltsTarget(Globals.Data.Stmt[PgmCounter].value); PgmCounter++; break;
		case STMT_SETAMPS:
			HW.SetAmpsLimit(Globals.Data.Stmt[PgmCounter].value); PgmCounter++; break;
		case STMT_WAITVOLTS_LESS:
		case STMT_WAITVOLTS_MORE:
		case STMT_WAITAMPS_LESS:
		case STMT_WAITAMPS_MORE:
			InWaitStmt = true; break;
	}
}

void ProgramClass::Edit() {
	if (NumberOfStmts() != 0) List();
	Serial.println(F("*** in edit mode. Type [line number] [statement]. Type [Enter] to exit"));
		
	while (true) {
		/// Read line
		const uint8_t LineLength = 50;
		char    	  line[LineLength];
		uint8_t 	  lineIx = 0;
		while (true)
			if (Serial.available()) {
				char c = Serial.read();
				if (c == '\n' || c == '\r' || lineIx == LineLength-1) {
					line[lineIx] = '\0';
					lineIx = 0;
					break;
				}
				else line[lineIx++] = c;
			}
		
		/// Break if empty line
		char *linenum = strtok(line, " ");
		if (linenum == NULL) break;
		
		uint8_t linenum_ = atoi(linenum);
		if (linenum_ == 0) {
			Serial.println(F("No line number"));
			continue;
		}
		if (linenum_ == 0xFF) {
			Serial.println(F("Bad line number"));
			continue;
		}
		if (NumberOfStmts() == MAX_PGM_STMTS-1) {
			Serial.println(F("No more mem"));
			continue;
		}
		
		char *stmt = strtok(NULL, " ");
		if (stmt == NULL) { 
			/// Statement of same line number to be deleted
			for (uint8_t s = 0; s < MAX_PGM_STMTS; s++) 
				if (Globals.Data.Stmt[s].line == linenum_) {
					Globals.Data.Stmt[s].line = 0xFF;
					SortStmts();
					List();
					continue;
				}
			continue;
		}

		uint8_t stmt_; uint16_t value_;
		if (!strcmp_P(stmt, PSTR("setvolts"))) {
			stmt_ = STMT_SETVOLTS;
			char *value = strtok(NULL, " ");
			if (value == NULL) { Serial.println(F("Missing value")); continue; }
			value_ = VOLTS_STEP_VALUE*round((1.0*atoi(value))/(1.0*VOLTS_STEP_VALUE));
			if (value_ < VOLTS_MIN_VALUE || value_ > VOLTS_MAX_VALUE) { Serial.println(F("Value out of range")); continue; }
		}
		else if (!strcmp_P(stmt, PSTR("setamps"))) {
			stmt_ = STMT_SETAMPS;
			char *value = strtok(NULL, " ");
			if (value == NULL) { Serial.println(F("Missing value")); continue; }
			value_ = AMPS_STEP_VALUE*round((1.0*atoi(value))/(1.0*AMPS_STEP_VALUE));
			if (value_ < AMPS_MIN_VALUE || value_ > AMPS_MAX_VALUE) { Serial.println(F("Value out of range")); continue; }
		}
		else if (!strcmp_P(stmt, PSTR("waitvolts")) || !strcmp_P(stmt, PSTR("waitamps"))) {
			char *op = strtok(NULL, " ");
			if (op == NULL) { Serial.println(F("Missing operator")); continue; }
			if (strcmp_P(op, PSTR(">")) && strcmp_P(op, PSTR("<"))) { Serial.println(F("Bad operator")); continue; }
			char *value = strtok(NULL, " ");
			if (value == NULL) { Serial.println(F("Missing value")); continue; }
			value_ = atoi(value);
			if (!strcmp_P(stmt, PSTR("waitvolts"))) {
				if (value_ < VOLTS_MIN_VALUE || value_ > VOLTS_MAX_VALUE) { Serial.println(F("Value out of range")); continue; }
				if (!strcmp_P(op, PSTR(">"))) stmt_ = STMT_WAITVOLTS_MORE; else stmt_ = STMT_WAITVOLTS_LESS;
			}
			else {
				if (value_ < AMPS_MIN_VALUE || value_ > AMPS_MAX_VALUE) { Serial.println(F("Value out of range")); continue; }
				if (!strcmp_P(op, PSTR(">"))) stmt_ = STMT_WAITAMPS_MORE; else stmt_ = STMT_WAITAMPS_LESS;
			}
		}
		else {
			char l[50]; snprintf_P(l, sizeof(l), PSTR("%s: Bad stmt\n"), stmt);
			Serial.print(l);
		}
		
		/// If line exists, substitute
		bool lineExists = false;
		for (uint8_t s = 0; s < MAX_PGM_STMTS; s++) 
			if (Globals.Data.Stmt[s].line == linenum_) {
				Globals.Data.Stmt[s].stmt  = stmt_;
				Globals.Data.Stmt[s].value = value_;
				lineExists = true;
				break;
			}
		if (lineExists) { List(); continue; }
		
		/// Include in program buffer
		for (uint8_t s = 0; s < MAX_PGM_STMTS; s++) 
			if (Globals.Data.Stmt[s].line == 0xFF) {
				Globals.Data.Stmt[s].line  = linenum_;
				Globals.Data.Stmt[s].stmt  = stmt_;
				Globals.Data.Stmt[s].value = value_;
				SortStmts();
				List();
				break;
			}
	}
	Globals.SaveToEEPROM();
}

void ProgramClass::Run() {
	PgmCounter = 0;
	Running = true;
	InWaitStmt = false;
	Loop();
}

uint8_t ProgramClass::NumberOfStmts() {
	uint8_t n = 0;
	for (uint8_t s = 0; s < MAX_PGM_STMTS; s++) if (Globals.Data.Stmt[s].line != 0xFF) n++;
	return n;
}

void ProgramClass::SortStmts() {
	uint8_t ix = 0;
	while (ix < MAX_PGM_STMTS-1) 
		if (Globals.Data.Stmt[ix].line > Globals.Data.Stmt[ix+1].line) {
			uint8_t  xline   = Globals.Data.Stmt[ix].line;
			uint8_t  xstmt   = Globals.Data.Stmt[ix].stmt;
			uint16_t xvalue  = Globals.Data.Stmt[ix].value;
			Globals.Data.Stmt[ix].line    = Globals.Data.Stmt[ix+1].line;
			Globals.Data.Stmt[ix].stmt    = Globals.Data.Stmt[ix+1].stmt;
			Globals.Data.Stmt[ix].value   = Globals.Data.Stmt[ix+1].value;
			Globals.Data.Stmt[ix+1].line  = xline;
			Globals.Data.Stmt[ix+1].stmt  = xstmt;
			Globals.Data.Stmt[ix+1].value = xvalue;
			ix = 0;
		}
		else 
			ix++;
}

void ProgramClass::List() {
	Serial.println(F("*** start"));
	char *stmts[] = {"setvolts", "setamps", "waitamps >", "waitamps <", "waitvolts >", "waitvolts <"};
	for (uint8_t s = 0; s < MAX_PGM_STMTS; s++)
		if (Globals.Data.Stmt[s].line != 0xFF) {
			char l[50]; snprintf_P(l, sizeof(l), PSTR("%03d %s %d\n"), Globals.Data.Stmt[s].line, stmts[Globals.Data.Stmt[s].stmt], Globals.Data.Stmt[s].value);
			Serial.print(l);
		}
	Serial.println(F("*** end"));
}

