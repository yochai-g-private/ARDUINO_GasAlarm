/*
 Name:		GasAlarm.ino
 Created:	12/12/2019 11:17:45 AM
 Author:	MCP
*/

/*******

 All the resources for this project:
 https://www.hackster.io/Aritro

*******/

// Use MQ-2 gas sensor, not MQ-135 air quality sensor
//https://randomnerdtutorials.com/guide-for-mq-2-gas-smoke-sensor-with-arduino/
//https://create.arduino.cc/projecthub/Aritro/smoke-detection-using-mq-2-gas-sensor-79c54a

#include <NYG.h>
#include <IInput.h>
#include <IOutput.h>
#include <Hysteresis.h>
#include <Toggler.h>
#include <Observer.h>
#include <Threshold.h>
#include <PushButton.h>
#include <RedGreenLed.h>
#include <DigitalInputInvertor.h>
#include <DigitalOutputInvertor.h>

using namespace NYG;

#define _USE_BUZZER						1
#define _USE_THRESHOLD_POTENTIOMETER	0

//-----------------------------------------------------
//	Pin numbers
//-----------------------------------------------------

enum PIN_NUMBERS
{
	Grama_Output_PIN		= D2,	// RELAY
	Grama_Input_PIN			= D3,	// SWITCH
#if _USE_BUZZER
	Buzzer_PIN				= D4,
#endif
	GreenLed_PIN			= D8,
	RedLed_PIN				= D10,
#if _USE_THRESHOLD_POTENTIOMETER
	Potentiometer_PIN		= A5,
#endif //_USE_THRESHOLD_POTENTIOMETER
	SmokeSensor_PIN			= A0,
};


//-----------------------------------------------------
//	Raw I/O elements
//-----------------------------------------------------

PullupPushButton			_GramaSwitch(Grama_Input_PIN);
BouncingSwitch				_GramaInput(_GramaSwitch);
DigitalInputInvertor		_Disabled(_GramaInput);

#if _USE_THRESHOLD_POTENTIOMETER
AnalogInputPin				_Potentiometer(Potentiometer_PIN);
RoundDownHysteresis			_PotentiometerH(_Potentiometer, 20);
#define Potentiometer		_PotentiometerH
#else
#define DANGER_THRESHOLD	150
#endif //_USE_THRESHOLD_POTENTIOMETER
AnalogInputPin				_SmokeSensor(SmokeSensor_PIN);
RoundDownHysteresis			_SmokeSensorH(_SmokeSensor, 10);

#define SMOKE_STABILITY_TIMEOUT_MILLIS	1000

typedef StableAnalogInput<SMOKE_STABILITY_TIMEOUT_MILLIS, millis>	StableSmokeSensor;
StableSmokeSensor			SmokeSensor(_SmokeSensorH);

//-----------------------------------------------------
//	I/O elements
//-----------------------------------------------------

Toggler						AlarmToggler(Toggler::AFTER_STOP_STATE_ON);
Toggler						LedToggler;
Toggler						BuzzerToggler;
RedGreenLed					Led(RedLed_PIN, GreenLed_PIN);
DigitalOutputPin			BuiltinLed(LED_BUILTIN);
Toggler						BuiltinLedToggler;

AnalogObserver				SmokeSensorObserver(SmokeSensor);
#if _USE_THRESHOLD_POTENTIOMETER
AnalogObserver				PotentiometerObserver(Potentiometer);
Threshold					SmokeThreshold(SmokeSensor, _Potentiometer.Get());
#else
Threshold					SmokeThreshold(SmokeSensor, DANGER_THRESHOLD);
#endif //_USE_THRESHOLD_POTENTIOMETER
DigitalObserver				SmokeDanger(SmokeThreshold);
DigitalObserver				Switch(_Disabled);

#if _USE_BUZZER
ToneOutputPin				Buzzer(Buzzer_PIN);
#endif

#if _USE_BUZZER
class Twitter : public IDigitalOutput
{
public:

	void Set(bool on)
	{
		if (on)
		{
			//LOGGER << "Beep!!!" << NL;
			Buzzer.Set(440);
		}
		else
		{
			//LOGGER << "Silent!!!" << NL;
			Buzzer.Quiet();
		}
	}

	bool Get()	const
	{
		return Buzzer.Get() != 0;
	}
}	Beeper;
#endif

#if _USE_THRESHOLD_POTENTIOMETER
int smoke_threshold;
#else
#define smoke_threshold DANGER_THRESHOLD
#endif //_USE_THRESHOLD_POTENTIOMETER

int smoke;

class Normal : public IDigitalOutput
{
	void Set(bool on)
	{
		//LOGGER << "Green=" << on << NL;
		if (on)
		{
			Led.SetGreen();
			if((smoke_threshold - smoke) <= _SmokeSensorH.GetDeviation())
				Buzzer.Set(880);
		}
		else
		{
			Led.SetOff();
			Buzzer.Quiet();
		}
	}

	bool Get()	const
	{
		return Led.IsGreenCurrent();
	}
} Normal;

static void BlinkGreenLed()
{
	unsigned long total = ((smoke_threshold - smoke) / _SmokeSensorH.GetDeviation());
	total *= 10 * 1000;
	//LOGGER << "total_seconds=" << total_seconds << NL;
	LedToggler.Start(Normal, Toggler::OnTotal(100, total), MILLIS);
}

#define BlinkRedLed(freq)		LedToggler.StartOnOff(Led.GetRed(),freq)

class Alarm : public IDigitalOutput
{
public:

	Alarm() : GreenRedBlinker(Led), _Relay(Grama_Output_PIN), Relay(_Relay)	{	}

	void Set(bool on)
	{
		enum { FREQUENCY = 250 };

		if (on)
		{
			Relay.On();
			BlinkRedLed(FREQUENCY);
		}
		else
		{
			Relay.Off();
			LedToggler.StartOnOff(GreenRedBlinker, FREQUENCY);
		}
	}

	bool Get()	const
	{
		return Relay.IsOn();
	}

protected:

	RedGreenLed::GR				GreenRedBlinker;

	DigitalOutputPin			_Relay;
	DigitalOutputInvertor		Relay;

} Alarm;

union
{
	struct
	{
		bool danger				: 1;
		bool disabled			: 1;
		bool disabled_is_fake	: 1;
	} s;

	uint8_t u;
}	state;

void update_state();

enum Status
{
	STATUS_WORKING,
	STATUS_DANGER,
};


Status status;

Timer AlarmStabilityTimer;

struct 
{
	int			min_smoke;
	int			max_smoke;
	uint32_t	total_tests;
	uint32_t	n_tests;
} Statistics;

Timer StatisticsTimer;
//========================================================
// Simulation modes
#define NO_SIMULATION						0
#define SIMULATE_DANGER_AT_BEGINNING		1
#define SIMULATE_DANGER_AFTER_SOME_TIME		2

#define SIMULATION_MODE		NO_SIMULATION

#if SIMULATION_MODE != NO_SIMULATION
Timer SimulationTimer;
#endif // SIMULATION_MODE != NO_SIMULATION
//========================================================
static void initialize_jurnaling()
{
	EepromJurnalWriter::Begin();
	//EepromJurnalWriter::Clean();
	EepromJurnalReader::PrintOut();

	LOGGER << NL;

	EepromJurnalWriter::SetLoggerAux();
	EepromJurnalWriter::Write("==============");
}
//========================================================
void setup()
{
#if _USE_BUZZER
	Buzzer.Quiet();
#endif
	
	Statistics.min_smoke = 1024;

	state.u = 0;

#if _USE_TIME_EX
	setTimeFromBuildTime();
#endif //_USE_TIME_EX

	Initialize();

	SwitchToWorkingStatus();

#if SIMULATION_MODE == SIMULATE_DANGER_AT_BEGINNING
	SimulationTimer.StartOnce(10000);
#endif // SIMULATION_MODE == SIMULATE_DANGER_AT_BEGINNING

	StatisticsTimer.StartForever(1, SECS);
	BuiltinLedToggler.StartOnOff(BuiltinLed, 1000);
}
//========================================================
void SwitchToWorkingStatus()
{
	Led.SetOff();
#if _USE_BUZZER
	Buzzer.Quiet();
#endif
	LedToggler.Stop();
	AlarmToggler.Stop();

	state.s.disabled_is_fake = false;

	BlinkGreenLed();

	LOGGER << "Working..." << NL;

	status = STATUS_WORKING;

	AlarmStabilityTimer.StartOnce(5, SECS);
}
//========================================================
void SwitchToDangerStatus()
{
	LedToggler.Stop();
	Led.SetOff();

#if SIMULATION_MODE == NO_SIMULATION
	AlarmToggler.Start(Alarm, Toggler::OnTotal(50, 60), SECS);
#endif //SIMULATION_MODE == NO_SIMULATION

	status = STATUS_DANGER;

	AlarmStabilityTimer.StartOnce(5, SECS);
}
//========================================================
void Initialize()
{
	initialize_jurnaling();

	LOGGER << "Initializing..." << NL;

#if SIMULATION_MODE == NO_SIMULATION
#if _USE_BUZZER
	Buzzer.Tone(440);	delay(50);		Buzzer.Quiet();
#endif
	Led.SetRed();		delay(1000);	Led.SetOff();
	Led.SetGreen();		delay(1000);	Led.SetOff();
#endif // SIMULATION_MODE == NO_SIMULATION

	Led.SetRed();

	SmokeSensor.Get();

	delay(SMOKE_STABILITY_TIMEOUT_MILLIS);
#if 1
	for(;;)
	{
		state.s.disabled = Switch.Get();
		if (!state.s.disabled)
			break;

		LOGGER << "ALARM DISABLED!!!" << NL;

#if _USE_BUZZER
		Buzzer.Tone(1000);
		delay(500);
		Buzzer.Quiet();
#endif
		delay(4500);
	}
#endif

	LOGGER << "Initialized OK" << NL;
	
	LOGGER << "Warming up..." << NL;

	for (;;)
	{
		Led.SetGreen();		delay(500);	
		Led.SetRed();		delay(500);

		update_state();

		if (smoke && !state.s.danger)
			break;
	}
}
//========================================================
void update_state()
{
#if _USE_THRESHOLD_POTENTIOMETER
	bool smoke_threshold_changed = PotentiometerObserver.TestChanged(smoke_threshold);

	if (smoke_threshold_changed)
	{
		SmokeThreshold.SetThreshold(smoke_threshold);

		if (!AlarmStabilityTimer.IsStarted())
			LOGGER << F("Thr: ") << smoke_threshold << NL;
	}
#endif //_USE_THRESHOLD_POTENTIOMETER

	bool smoke_changed = SmokeSensorObserver.TestChanged(smoke);

	if (smoke_changed)
	{
		if (!AlarmStabilityTimer.IsStarted())
			LOGGER <<  F("Gas: ") << smoke << NL;

		if (Statistics.min_smoke > smoke)
			Statistics.min_smoke = smoke;
		if (Statistics.max_smoke < smoke)
			Statistics.max_smoke = smoke;
	}

	bool smoke_danger;
	bool smoke_danger_changed = SmokeDanger.TestChanged(smoke_danger);

#if SIMULATION_MODE == SIMULATE_DANGER_AT_BEGINNING
	if (SimulationTimer.IsStarted() && !SimulationTimer.Test())
		smoke_danger = true;
#endif // SIMULATION_MODE == SIMULATE_DANGER_AT_BEGINNING

	if (smoke_danger_changed)
	{
		LOGGER << ((smoke_danger) ? F("") : F("NO")) << " DANGER: " << smoke << NL;
	}

	bool disabled = false;
	bool switch_changed = Switch.TestChanged(disabled);

	state.s.danger   = smoke_danger;
	state.s.disabled = disabled;

	if (smoke_changed && !smoke_danger && !disabled)
		BlinkGreenLed();
}
//========================================================
void Working()
{
#if SIMULATION_MODE != NO_SIMULATION
	LOGGER << "into  Working()" << NL; delay(1000);
#endif // SIMULATION_MODE != NO_SIMULATION

	LedToggler.Toggle();

	AlarmStabilityTimer.Test();

	if (!AlarmStabilityTimer.IsStarted())
	{
		update_state();

#if _USE_BUZZER
		BuzzerToggler.Toggle();
#endif
		LedToggler.Toggle();

		if (state.s.disabled)
		{
			if (!state.s.disabled_is_fake)
			{
				LOGGER << "Disabled is FAKE" << NL;

				state.s.disabled_is_fake = true;
				LedToggler.Stop();
				Led.SetOff();
				BlinkRedLed(1000);

#if _USE_BUZZER
				BuzzerToggler.Start(Beeper, 50, 60000);
#endif
			}
		}
		else
		{
			if (state.s.disabled_is_fake)
			{
				BuzzerToggler.Stop();

				LOGGER << "Disabled switch corrected" << NL;

				state.s.disabled_is_fake = false;
				LedToggler.Stop();
				Led.SetOff();
				BlinkGreenLed();
			}
		}

		if (state.s.danger)
		{
			SwitchToDangerStatus();
			return;
		}
	}

	delay(10);
}
//========================================================
void Danger()
{
#if SIMULATION_MODE != NO_SIMULATION
	LOGGER << "into  Danger()" << NL; delay(1000);
#endif // SIMULATION_MODE != NO_SIMULATION

	LedToggler.Toggle();
	AlarmToggler.Toggle();

#if SIMULATION_MODE == NO_SIMULATION
	//if (0 == (x % 180))
#endif // SIMULATION_MODE == NO_SIMULATION

	AlarmStabilityTimer.Test();

	if (!AlarmStabilityTimer.IsStarted())
	{
		update_state();

		if (!state.s.danger)
		{
			LOGGER << "No more danger." << NL;

			SwitchToWorkingStatus();

			return;
		}

		if (state.s.disabled)
		{
#if _USE_BUZZER
			Buzzer.Quiet();
#endif
			return;
		}
	}

	static int x = 0;

	x = (x + 1) % 360;

	float	sinVal  = sin(x * (PI / 180));
	int		toneVal = 2000 + sinVal * 500;

#if SIMULATION_MODE == NO_SIMULATION
#if _USE_BUZZER
	Buzzer.Tone(toneVal);
#endif
#else
	LOGGER << "Buzzer.Tone(toneVal);" << NL;
#endif //SIMULATION_MODE == NO_SIMULATION
}
//========================================================
static log_status()
{
	LOGGER << "Status: " << Statistics.min_smoke << ".." << Statistics.max_smoke;
	LOGGER << " (avg="    << (Statistics.total_tests / Statistics.n_tests) << ")" << NL;
}
//========================================================
void loop()
{
	BuiltinLedToggler.Toggle();

	if (StatisticsTimer.Test())
	{
		Statistics.total_tests += smoke;
		Statistics.n_tests++;
	}

	if (0 == (EepromJurnalWriter::GetLineNumber() % 10))
		log_status();

	switch (status)
	{
		case STATUS_WORKING:
		{
			Working();
			break;
		}

		case STATUS_DANGER:
		{
			Danger();
			break;
		}
	}
}
//========================================================
extern const char* gbl_build_date = __DATE__;
extern const char* gbl_build_time = __TIME__;
