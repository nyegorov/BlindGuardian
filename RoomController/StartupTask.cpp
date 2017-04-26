#include "pch.h"
#include "StartupTask.h"
#include "log_manager.h"
#include "debug_stream.h"

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::ApplicationModel;

using namespace roomctrl;
using namespace std::chrono;
using namespace std::chrono_literals;

wchar_t msg[] = L"Http server started.\r\n";

wdebugstream wdebug;
debugstream debug;
log_manager logger{ path(wstring(ApplicationData::Current().LocalFolder().Path())) / "log.txt" };

namespace roomctrl {

StartupTask::StartupTask() : _server(path(wstring(ApplicationData::Current().LocalFolder().Path())))
{
	_server.config().set(L"poll_interval", 1000);
	_server.config().set(L"socket_timeout", 1000);
	_server.config().set(L"socket_timeout_action", 60000);
	_server.config().set(L"enable_debug", true);
	logger.enable_debug(_server.config().get(L"enable_debug", false));
}

void StartupTask::Run(IBackgroundTaskInstance taskInstance)
{
	Deferral = taskInstance.GetDeferral();
	InitGpio();

/*	TimerElapsedHandler handler([this](ThreadPoolTimer timer) {
		//pinValue = (pinValue == GpioPinValue::High) ? GpioPinValue::Low : GpioPinValue::High;
		//pin.Write(pinValue);
		_server.run();
	});

	TimeSpan interval = milliseconds(_server.config().get(L"poll_interval", 1000));
	Timer = ThreadPoolTimer::CreatePeriodicTimer(handler, interval);

	_server.start();*/

	std::thread th([this]() { 
		_server.start().get();
		while(true) {
			std::this_thread::sleep_for(1s);
			_server.run();
		} 
	});
	th.detach();
	OutputDebugString(msg);
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