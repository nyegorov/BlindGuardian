#include "pch.h"
#include "StartupTask.h"

using namespace roomctrl;
using namespace std::chrono;
using namespace std::chrono_literals;

wchar_t msg[] = L"Hello from C++/WinRT dll.\r\n";

StartupTask::StartupTask()
{
}

void StartupTask::Run(IBackgroundTaskInstance taskInstance)
{
	Deferral = taskInstance.GetDeferral();
	OutputDebugString(msg);
	InitGpio();

	TimerElapsedHandler handler(
	[this](ThreadPoolTimer timer)
	{
	pinValue = (pinValue == GpioPinValue::High) ? GpioPinValue::Low : GpioPinValue::High;
	pin.Write(pinValue);
	});

	TimeSpan interval = 500ms;
	Timer = ThreadPoolTimer::CreatePeriodicTimer(handler, interval);
}

void StartupTask::InitGpio()
{
	auto gpio = GpioController::GetDefault();
	if (gpio) {
		pin = gpio.OpenPin(LED_PIN);
		pinValue = GpioPinValue::High;
		pin.Write(pinValue);
		pin.SetDriveMode(GpioPinDriveMode::Output);
	}
}

