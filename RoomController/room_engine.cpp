#include "pch.h"
#include "common.h"
#include "room_engine.h"
#include "debug_stream.h"

using namespace winrt;
using namespace winrt::Windows::Data::Json;
using namespace winrt::Windows::Networking::Connectivity;

const wchar_t module_name[] = L"ROOM";

namespace roomctrl {

room_server::room_server(const path& path) : _rules(path / "rules.json"), _config(path / "config.json")
{
	_http.on(L"/status",		L"html/room_status.html");
	_http.on(L"/edit",			L"html/edit_rule.html");
	_http.on(L"/log",			L"html/server_log.html");
	_http.on(L"/pair",			L"html/pair_remote.html");
	_http.on(L"/styles.css",	L"html/styles.css");
	_http.on(L"/scripts.js",	L"html/scripts.js");
	_http.on(L"/back.jpg",		L"html/img/background.jpg");
	_http.on(L"/favicon.ico",	L"html/img/favicon.ico");

	_http.on(L"/api/room",		[this](auto&, auto&) { return std::make_tuple(content_type::json, get_sensors()); });
	_http.on(L"/api/log",		[this](auto&, auto&) { return std::make_tuple(content_type::json, logger.to_string()); });
	_http.on(L"/api/rules",		rest_adapter<rules_db>::get(_rules));
	_http.on(L"/api/pairing",	[this](auto&, auto&) { return std::make_tuple(content_type::json, ::to_wstring(_pair_info.ToString())); });
	_http.on(L"/api/blinds",	[this](auto& req, auto&) {
		if(req.type != http_method::put)	throw http_status::method_not_allowed;
		auto pos = JsonObject::Parse(req.body).GetNamedNumber(L"position");
		if(pos == 100)	_tasks.push([this]() {_motor.open(); });
		if(pos == 0)	_tasks.push([this]() {_motor.close(); });
		return std::make_tuple(content_type::text, L"");
	});
	_http.on(L"/api/actions",	[this](auto& req, auto& value) { 
		auto action = JsonObject::Parse(req.body).GetNamedString(L"action");
		if(action == L"pair")	pair_remote();
		return std::make_tuple(content_type::text, L""); 
	});
	_http.on(L"/",				L"html/room_status.html");

	init(_sensors, _actuators);
	logger.info(module_name, L"Room server v%s started", version().c_str());
}

void room_server::init(const vec_sensors &sensors, const vec_actuators &actuators)
{
	using std::chrono::milliseconds;
	_sensors = sensors;
	_actuators = actuators;
	_parser.clear();
	_parser.set(L"wait", [](auto& params) { const auto ms = get_arg<milliseconds>(params, 0); std::this_thread::sleep_for(ms); return value_t{ 1 }; });
	for(auto& ps : _sensors) {
		_parser.set(ps->name(), ps);
		_parser.set(ps->name() + L".min", [ps](auto&) {return ps->min(); });
		_parser.set(ps->name() + L".max", [ps](auto&) {return ps->max(); });
		_parser.set(ps->name() + L".reset", [ps](auto&) {return ps->reset(), ps->value(); });
	}
	for(auto& pa : _actuators) {
		auto obj = pa->name();
		for(auto& paction : pa->actions()) {
			auto name = obj.empty() ? paction->name() : obj + L"." + paction->name();
			_parser.set(name, [paction](auto& p) {return paction->activate(p), value_t{ 1 }; });
		}
	}
}

wstring room_server::version()
{
	try	{
		const auto ver = winrt::Windows::ApplicationModel::Package::Current().Id().Version();
		wchar_t ver_str[40] = { 0 };
#ifdef _DEBUG
		const wchar_t format[] = L"%d.%d.%d.%d-D";
#else
		const wchar_t format[] = L"%d.%d.%d.%d";
#endif
		swprintf(ver_str, _countof(ver_str), format, ver.Major, ver.Minor, ver.Build, ver.Revision);
		return { ver_str };
	} catch(...)	{}
	return L"debug"s;
}

room_server::ip_info room_server::get_ip()
{
	ip_info ips = { L"-", L"-" };
	try	{
		for(const auto& profile : NetworkInformation::GetConnectionProfiles()) {
			for(const auto& host : NetworkInformation::GetHostNames()) {
				if(!host.IPInformation())	continue;
				if(host.IPInformation().NetworkAdapter().NetworkAdapterId() == profile.NetworkAdapter().NetworkAdapterId())
					if(profile.IsWlanConnectionProfile())	ips.wifi = host.CanonicalName();
					else									ips.lan  = host.CanonicalName();
			}
		}
	} catch(const winrt::hresult_error& hr)	{
		logger.error(module_name, hr);
	}
	return ips;
}

wstring room_server::get_sensors()
{
	JsonObject json;
	auto ip = get_ip();
	for (auto& s : _sensors) {
		json.SetNamedValue(s->name(), is_error(s->value()) ? JsonValue::CreateStringValue(L"--") :
			JsonValue::CreateNumberValue(std::get<value_type>(*s->value())));
	}
	//auto mot_ip = _udns.get_address(_esp8266.host_name());
	json.SetNamedValue(L"remote_id", JsonValue::CreateNumberValue(_dm35le.get_remote_id()));
	json.SetNamedValue(L"motctrl",   JsonValue::CreateStringValue(_ext.online() ? _ext.remote_ip().DisplayName().c_str() : L"") );
	JsonObject sensors;
	sensors.SetNamedValue(L"sensors", json);
	sensors.SetNamedValue(L"version", JsonValue::CreateStringValue(version()));
	sensors.SetNamedValue(L"lan", JsonValue::CreateStringValue(ip.lan));
	sensors.SetNamedValue(L"wifi", JsonValue::CreateStringValue(ip.wifi));
	return ::to_wstring(sensors.ToString());
}

task<void> room_server::pair_remote()
{
	co_await winrt::resume_background();
	_pair_info.SetNamedValue(L"done", JsonValue::CreateBooleanValue(false));
	_pair_info.SetNamedValue(L"commands", JsonValue::CreateNumberValue(0));
	co_await _beeper.beep();
	auto ok = _dm35le.pair_remote(10s, [this](int commands_received) {
		_pair_info.SetNamedValue(L"commands", JsonValue::CreateNumberValue(commands_received));
	});
	_pair_info.SetNamedValue(L"id", JsonValue::CreateNumberValue(ok ? _dm35le.get_remote_id() : 0));
	co_await (ok ? _beeper.beep() : _beeper.fail());
	_pair_info.SetNamedValue(L"done", JsonValue::CreateBooleanValue(true));
}

value_t room_server::eval(const wchar_t *expr)
{
	return _parser.eval(expr);
}

task<void> room_server::start()
{
	co_await _temp_in.start();
	co_await _ext.start();
	co_await _http.start();
	co_await 500ms;
	_motor.start();
	_beeper.beep();
}

void room_server::run()
{
	_led.invert();

	while(!_tasks.empty()) {
		std::function<void()> f;
		if(_tasks.try_pop(f))	f();
	}

	for(auto& sensor : _sensors) sensor->update();

	for(const auto& rule : _rules.get_all())
	{
		if(!rule.enabled)	continue;

		rule_status status = rule_status::error;

		auto result = _parser.eval(rule.condition, true);
		if(is_error(result)) {
			logger.error(module_name, L"error evaluating condition '%s': %s", rule.condition.c_str(), get_error_msg(result));
		} else {
			status = result == value_t{ 0 } ? rule_status::inactive : rule_status::active;
		}

		if(status != rule.status && status == rule_status::active) {
			logger.info(module_name, L"activated rule '%s' (%s)", rule.name.c_str(), rule.action.c_str());
			_beeper.beep();
			result = _parser.eval(rule.action);
			if(is_error(result)) {
				logger.error(module_name, L"error evaluating action '%s': %s", rule.action.c_str(), get_error_msg(result));
				status = rule_status::error;
			}
		}
		_rules.set_status(rule.id, status);
	}
}

}