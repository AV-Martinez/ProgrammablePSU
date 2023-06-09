#include "InputDevice.h"

InputDeviceClass InputDevice;

void InputDeviceClass::Setup(uint8_t pushButtonPin, uint8_t pinEnc1, uint8_t pinEnc2) {
	Enc = new Encoder (pinEnc1, pinEnc2);

	PBpin = pushButtonPin;
	pinMode(PBpin, INPUT_PULLUP);
	PBStatus = 1;
}

void InputDeviceClass::Loop(bool *encoder, bool *pushbutton) {

	*encoder = false;
	static uint32_t ticksEnc = 0;
	if (millis()-ticksEnc > 300) {
		ticksEnc = millis();
		static long position = 0;
		long newPosition =  Enc->read();
		if (newPosition != position) {
			if (newPosition > position) EncoderDir = 'U'; else EncoderDir = 'D';
			position = newPosition;
			*encoder = true;
		}
	}

	*pushbutton = false;
	if (digitalRead(PBpin) == LOW) {
		if (PBStatus == 1) {
			PBTicks = millis();
			PBStatus = 0;
		}
	}
	else {
		if (PBStatus == 0) {
			uint32_t now = millis();
			if (now - PBTicks > 100) {
				if 		(now - PBTicks < 1000) { *pushbutton = true; PushButtonPress = 'S'; }
				else if (now - PBTicks < 5000) { *pushbutton = true; PushButtonPress = 'L'; }
				else    	                   { *pushbutton = true; PushButtonPress = 'X'; }
				PBStatus = 1;
			}
			else
				PBTicks = now;
		}
	}
	
}

