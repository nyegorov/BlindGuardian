#include "pch.h"
#include "StartupTask.h"

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::ApplicationModel;

using namespace roomctrl;
using namespace std::chrono;
using namespace std::chrono_literals;

wchar_t msg[] = L"Http server started.\r\n";

namespace roomctrl {

StartupTask::StartupTask() : _server(path(wstring(ApplicationData::Current().LocalFolder().Path())) / "rules.json", {}, {})
{
}

void StartupTask::Run(IBackgroundTaskInstance taskInstance)
{
	Deferral = taskInstance.GetDeferral();
	OutputDebugString(msg);
	InitGpio();

	TimerElapsedHandler handler([this](ThreadPoolTimer timer) {
		pinValue = (pinValue == GpioPinValue::High) ? GpioPinValue::Low : GpioPinValue::High;
		pin.Write(pinValue);
	});

	TimeSpan interval = 500ms;
	Timer = ThreadPoolTimer::CreatePeriodicTimer(handler, interval);

	_server.start();
/*	std::wifstream ifs(L"status.json", std::ios::binary);
	ifs.imbue(std::locale{ std::locale(), new std::codecvt_utf8<wchar_t>() });
	wstringstream str;
	str << ifs.rdbuf();
	wstring content = str.str();*/
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

}