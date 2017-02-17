// Copyright (c) Microsoft. All rights reserved.

#include "pch.h"
#include "StartupTask.h"

#include "Sensor.h"
#include "Rules.h"

using namespace BlindGuardian;

using namespace Platform;
using namespace Windows::ApplicationModel::Background;
using namespace Windows::Foundation;
using namespace Windows::Devices::Gpio;
using namespace Windows::System::Threading;
using namespace concurrency;

StartupTask::StartupTask()
{
}

void StartupTask::Run(IBackgroundTaskInstance^ taskInstance)
{
	Deferral = taskInstance->GetDeferral();
	InitGpio();
	TimerElapsedHandler ^handler = ref new TimerElapsedHandler(
		[this](ThreadPoolTimer ^timer)
	{
		pinValue = (pinValue == GpioPinValue::High) ? GpioPinValue::Low : GpioPinValue::High;
		pin->Write(pinValue);
	});

	TimeSpan interval;
	interval.Duration = 500 * 1000 * 10;
	Timer = ThreadPoolTimer::CreatePeriodicTimer(handler, interval);
}

void StartupTask::InitGpio()
{
	pin = GpioController::GetDefault()->OpenPin(LED_PIN);
	pinValue = GpioPinValue::High;
	pin->Write(pinValue);
	pin->SetDriveMode(GpioPinDriveMode::Output);

	Windows::Data::Json::JsonObject json;
	json.Parse(R"(
	{
		"rules": [
			{ "name": "r1", "condition": "temp > 30", "body": "mot1.open() " },
			{ "name": "r2", "condition": "temp > 30", "body": "mot2.set_pos() " }
	   ]
	}
	)");
	auto rules = json.GetNamedArray("rules");
}

