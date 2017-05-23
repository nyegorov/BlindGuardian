#include "pch.h"
#include "StartupTask.h"
#include "log_manager.h"
#include "debug_stream.h"

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Devices::Gpio;
using namespace winrt::Windows::System::Threading;

wdebugstream wdebug;
debugstream debug;
const path data_path{ wstring(ApplicationData::Current().LocalFolder().Path()) };
log_manager logger{ data_path / "log.txt" };

namespace roomctrl {

StartupTask::StartupTask()
{
	debug << "*** STARTUP ***" << std::endl;
	_server = std::make_unique<room_server>(data_path);
	_server->config().set(L"poll_interval", 1000);
	_server->config().set(L"socket_timeout", 1000);
	_server->config().set(L"socket_timeout_action", 60000);
	_server->config().set(L"enable_debug", true);
	logger.enable_debug(_server->config().get(L"enable_debug", false));
}

void StartupTask::Run(IBackgroundTaskInstance taskInstance)
{
	debug << "*** RUN SERVER ***" << std::endl;
	_deferral = taskInstance.GetDeferral();

	//std::async([this]() { 
	std::thread th([this]()	{
		_server->start().get();
		while(true) {
			std::this_thread::sleep_for(1s);
			_server->run();
		} 
	});
	th.detach();
}

}