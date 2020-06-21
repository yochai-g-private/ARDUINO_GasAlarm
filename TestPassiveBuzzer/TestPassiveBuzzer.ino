/*
 Name:		TestPassiveBuzzer.ino
 Created:	12/18/2019 4:03:42 PM
 Author:	MCP
*/

#include <NYG.h>

using namespace NYG;

enum PIN_NUMBERS
{
	Grama_Output_PIN		= D2,	// RELAY
	Grama_Input_PIN			= D3,	// SWITCH
	Buzzer_PIN				= D4,
	GreenLed_PIN			= D6,
	RedLed_PIN				= D7,
	Potentiometer_PIN		= A4,
	SmokeSensor_PIN			= A5,
};

float	sinVal;         // Define a variable to save sine value
int		toneVal;          // Define a variable to save sound frequency

void setup() 
{
	pinMode(Buzzer_PIN, OUTPUT); // Set Buzzer pin to output mode
}

void loop() {
	for (int x = 0; x < 360; x++) {       // X from 0 degree->360 degree
		sinVal = sin(x * (PI / 180));       // Calculate the sine of x
		toneVal = 2000 + sinVal * 500;      // Calculate sound frequency according to the sine of x
		tone(Buzzer_PIN, toneVal);           // Output sound frequency to Buzzer_PIN
		delay(1);
	}
}
