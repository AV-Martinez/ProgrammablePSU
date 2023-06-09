#ifndef _GLOBALS_H_
#define _GLOBALS_H_
#include <Arduino.h>

/** Statements:
 * 		setvolts value 			-- set volts target in millivots
 * 		setamps value 			-- set amps limit in milliamps
 *		waitamps {<|>} value	-- wait for condition	
 * 		waitvolts {<|>} value	-- wait for condition
*/

#define MAX_PGM_STMTS 10

#define STMT_SETVOLTS 			0
#define STMT_SETAMPS 			1
#define STMT_WAITAMPS_MORE 		2
#define STMT_WAITAMPS_LESS 		3
#define STMT_WAITVOLTS_MORE 	4
#define STMT_WAITVOLTS_LESS 	5

typedef struct {
	uint8_t  line;
	uint8_t  stmt;
	uint16_t value;
} PgmStmt;

class GlobalsClass {
public:
	void Setup			(bool forceInit = false);
	void LoadFromEEPROM (bool forceInit);
	void SaveToEEPROM   ();
	int	 FreeMemory		();

	struct {
		uint16_t VoltsTarget;	/// Millivolts, target volts setting
		uint16_t AmpsLimit;		/// Milliamps, limiting current
		uint8_t  LCDContrast;	/// Contrast value for LCD
		PgmStmt  Stmt[MAX_PGM_STMTS];
		
		/// Calibration data
		bool     Calibrated;	
		double	 Cal_Ioff_A;
		double	 Cal_Ioff_B;
		double 	 Cal_Volt_A;
		double 	 Cal_Volt_B;
		double 	 Cal_Amps_A;
		double   Cal_Amps_B;
		
		char     ok[3];
	} Data;

	void Double2Str(double value, char *result, uint8_t size);
	
	
};

extern GlobalsClass Globals;

#endif
