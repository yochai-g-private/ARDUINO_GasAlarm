/*
 Name:		TestStableInput.ino
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
	Grama_Output_PIN		= D2,	// RELAY
	Grama_Input_PIN			= D3,	// SWITCH
	Buzzer_PIN				= D4,
	GreenLed_PIN			= D6,
	RedLed_PIN				= D7,
	Potentiometer_PIN		= A5,
	SmokeSensor_PIN			= A0,
};

//-----------------------------------------------------
//	Raw I/O elements
//-----------------------------------------------------

PullupPushButton			_GramaSwitch(Grama_Input_PIN);
BouncingSwitch				_GramaInput(_GramaSwitch);
DigitalOutputPin			_GramaRelay(Grama_Output_PIN);
//Grama1						_Grama(_GramaInput, _GramaRelay);
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
//GramaObserver				Switch(_Grama);


//-----------------------------------------------------
//	SETUP & LOOP
//-----------------------------------------------------

Toggler						toggler;
DigitalObserver				SwitchObserver(_GramaInput);

void setup() {
	setTimeFromBuildTime();

	toggler.Start(_GramaRelay, 2000, 1000);

	delay(2000);
}

uint32_t loop_count = 0;
unsigned long toggled_millis;

void loop() {

	if (!toggler.IsStarted())
		return;

	loop_count++;

	bool toggled = toggler.Toggle();

	if (toggled) 
	{ 
		ShowTime(false);  LOGGER << " - Loop #" << loop_count << " - TOGGLED" << NL; 
		toggled_millis = millis();
	}

	bool on;

	if (SwitchObserver.TestChanged(on))
	{
		unsigned long now_millis = millis();
		unsigned long delta = now_millis - toggled_millis;

		ShowTime(false); LOGGER << " - Loop #" << loop_count << " - " << (on ? "ON" : "OFF") << " ==> " << delta << NL;
	}
}
