/*
 Name:		TestRelay.ino
 Created:	12/18/2019 4:05:40 PM
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

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	pinMode(Grama_Output_PIN, OUTPUT);
}

bool on;

// the loop function runs over and over again until power down or reset
void loop() {
	on = !on;
	digitalWrite(Grama_Output_PIN, on ? HIGH : LOW);
	Serial.println(on ? "HIGH" : "LOW");
	delay(1000);
}
