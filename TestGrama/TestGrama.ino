/*
 Name:		TestGrama.ino
 Created:	12/18/2019 4:11:52 PM
 Author:	MCP
*/

#include <IInput.h>
#include <IOutput.h>
#include <Hysteresis.h>
#include <Toggler.h>
#include <Observer.h>
#include <Threshold.h>
//#include <Grama.h>
#include <PushButton.h>
#include <Logger.h>
#include <TimeEx.h>

using namespace NYG;

//-----------------------------------------------------
//	Pin numbers
//-----------------------------------------------------

enum PIN_NUMBERS
{
	Buzzer_PIN				=  4,
	Grama_Output_PIN		=  8,	// RELAY
	Grama_Input_PIN			=  9,	// SWITCH
	GreenLed_PIN			= 11,
	RedLed_PIN				= 12,
	Potentiometer_PIN		= A4,
	SmokeSensor_PIN			= A5,
};

//-----------------------------------------------------
//	Raw I/O elements
//-----------------------------------------------------

PullupPushButton			_GramaSwitch(Grama_Input_PIN);
BouncingSwitch				_GramaInput(_GramaSwitch);
DigitalOutputPin			_GramaRelay(Grama_Output_PIN);
Grama1						_Grama(_GramaInput, _GramaRelay);
DigitalOutputPin			_Buzzer(Buzzer_PIN);
DigitalOutputPin			_GreenLed(GreenLed_PIN);
DigitalOutputPin			_RedLed(RedLed_PIN);

AnalogInputPin				_Potentiometer(Potentiometer_PIN);
AnalogInputPin				_SmokeSensor(SmokeSensor_PIN);

SmokeHysteresis				_SmokeSensorH(_SmokeSensor);
PotentiometerHysteresis		_PotentiometerH(_Potentiometer);

//-----------------------------------------------------
//	I/O elements
//-----------------------------------------------------

Toggler						BuzzerToggler;
Toggler						LedToggler;

AnalogObserver				SmokeSensorObserver(_SmokeSensorH);
AnalogObserver				PotentiometerObserver(_PotentiometerH);
Threshold					SmokeThreshold(_SmokeSensorH, _Potentiometer.Get());
DigitalObserver				SmokeDanger(SmokeThreshold);
GramaObserver				Switch(_Grama);

// the setup function runs once when you press reset or power the board

int grama_seconds = 5;

void setup() {
	setTimeFromBuildTime();
	_Grama.Start(grama_seconds, grama_seconds);
}

// the loop function runs over and over again until power down or reset
void loop() {
	GramaState state;
	bool switch_changed = Switch.TestChanged(state);

	if (!switch_changed)
		return;

	ShowTime(false); LOGGER << F(" - ") << Grama::GetStateName(state) << NL;

	delay(1000);

	if (state != GS_ACTIVE)
	{
		//grama_seconds++;
		LOGGER << F("Grama seconds: ") << grama_seconds << NL;
		_Grama.Start(grama_seconds, grama_seconds);
		delay(1000);
	}

}
