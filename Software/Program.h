#ifndef _PROGRAM_H_
#define _PROGRAM_H_
#include <Arduino.h>

class ProgramClass {
public:
	void Setup();
	void Loop();
	void Edit();
	void List();
	void Run();
	
	bool Running;

private:
	uint8_t NumberOfStmts();
	void SortStmts();

	uint8_t PgmCounter;
	bool    InWaitStmt;
};

extern ProgramClass Program;


#endif
