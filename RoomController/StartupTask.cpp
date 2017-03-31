#include "pch.h"
#include "StartupTask.h"

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::ApplicationModel;

using namespace roomctrl;
using namespace std::chrono;
using namespace std::chrono_literals;

wchar_t msg[] = L"Http server started.\r\n";

namespace roomctrl {

StartupTask::StartupTask() : _rules(path(wstring(ApplicationData::Current().LocalFolder().Path())) / "rules.json")
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

	std::wifstream ifs(L"status.json", std::ios::binary);
	ifs.imbue(std::locale{ std::locale(), new std::codecvt_utf8<wchar_t>() });
	wstringstream str;
	str << ifs.rdbuf();
	wstring content = str.str();

	_json = JsonObject::Parse(str.str());
	if (!_rules.get_ids().empty())	_rules.set_status(_rules.get_ids()[0], rule_status::active);
	_server.add(L"/",			L"html/room_status.html");
	_server.add(L"/status",		L"html/room_status.html");
	_server.add(L"/edit",		L"html/edit_rule.html");
	_server.add(L"/back.jpg",	L"html/img/background.jpg");
	_server.add(L"/favicon.ico",L"html/img/favicon.ico");
	_server.add(L"/room.json", [this](auto&, auto&) { return std::make_tuple(content_type::json, _json.ToString()); });
	_server.add(L"/rules.json", [this](auto&, auto&) { return std::make_tuple(content_type::json, _rules.to_string()); });
	_server.add(L"/rule.json", [this](auto& r, auto&) { return std::make_tuple(content_type::json, _rules.get(std::stoul(r.params[L"id"s])).to_string()); });
	_server.add_action(L"set_pos", [this](auto&, auto& value) { _json.SetNamedValue(L"position", JsonValue::CreateStringValue(value)); });
	_server.add_action(L"save_rule", [this](auto& req, auto& value) {
		_rules.save({ std::stoul(value), req.params[L"rule_name"s], req.params[L"condition"s], req.params[L"action"s] });
	});
	_server.add_action(L"delete_rule", [this](auto&, auto& value) { _rules.remove(std::stoul(value)); });
	_server.start();
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