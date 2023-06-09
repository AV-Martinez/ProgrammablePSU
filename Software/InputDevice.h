#ifndef _INPUT_DEVICE_H
#define _INPUT_DEVICE_H

#include <Arduino.h>
#include "Encoder.h"

class InputDeviceClass {
public:
	void    Setup(uint8_t pushButtonPin, uint8_t pinEnc1, uint8_t pinEnc2);
	void    Loop(bool *encoder, bool *pushbutton);
	char    EncoderDir;
	char 	PushButtonPress;

private:
	Encoder *Enc;
	uint8_t  PBpin;
	uint8_t  PBStatus;
	uint32_t PBTicks;
	
};

extern InputDeviceClass InputDevice;

#endif
