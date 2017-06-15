#include "pch.h"
#include "room_engine.h"
#include "debug_stream.h"

using namespace winrt;
using namespace winrt::Windows::Data::Json;
using namespace winrt::Windows::Networking::Connectivity;

const wchar_t module_name[] = L"ROOM";

namespace roomctrl {

room_server::room_server(const path_t& path) : _rules(path / "rules.json"), _config(path / "config.json")
{
	_http.on(L"/", L"html/room_status.html");
	_http.on(L"/index.html", L"html/room_status.html");
	_http.on(L"/status", L"html/room_status.html");
	_http.on(L"/edit", L"html/edit_rule.html");
	_http.on(L"/log", L"html/server_log.html");
	_http.on(L"/pair", L"html/pair_remote.html");
	_http.on(L"/styles.css", L"html/styles.css");
	_http.on(L"/back.jpg", L"html/img/background.jpg");
	_http.on(L"/favicon.ico", L"html/img/favicon.ico");
	_http.on(L"/room.json",  [this](auto&, auto&) { return std::make_tuple(content_type::json, get_sensors()); });
	_http.on(L"/rules.json", [this](auto&, auto&) { return std::make_tuple(content_type::json, get_rules()); });
	_http.on(L"/rule.json",  [this](auto&r,auto&) { return std::make_tuple(content_type::json, _rules.get(std::stoul(r.params[L"id"s])).to_string()); });
	_http.on(L"/pair.json",  [this](auto&, auto&) { return std::make_tuple(content_type::json, _pair_info.ToString()); });
	_http.on(L"/log.json",   [this](auto&, auto&) { return std::make_tuple(content_type::json, logger.to_string()); });
	_http.on_action(L"pair_remote", [this](auto&, auto& value) { pair_remote(); });
	_http.on_action(L"set_pos", [this](auto&, auto& value) {
		if(std::stoul(value) == 100)	_tasks.push([this]() {_motor.open(); });
		if(std::stoul(value) == 0)		_tasks.push([this]() {_motor.close(); });
	});
	_http.on_action(L"save_rule", [this](auto& req, auto& value) {
		auto enabled = req.params.find(L"enabled") != req.params.end();
		auto id = _rules.save({ std::stoul(value), req.params[L"rule_name"s], req.params[L"condition"s], req.params[L"action"s], enabled });
		_rules.set_status(id, rule_status::inactive);
		logger.info(module_name, L"save rule %d", id);
	});
	_http.on_action(L"delete_rule", [this](auto&, auto& value) { 
		auto id = std::stoul(value);
		_rules.remove(id); 
		logger.info(module_name, L"delete rule %d", id);
	});
	_http.on_action(L"enable_rule", [this](auto& req, auto& value) { 
		auto rule = _rules.get(std::stoul(req.params[L"enable_rule"s]));
		if(rule.id) {
			rule.enabled = req.params.find(L"enabled") != req.params.end();
			_rules.save(rule);
			logger.info(module_name, L"%s rule %d", rule.enabled ? L"enable" : L"disable", rule.id);
		}
	});

	init(_sensors, _actuators);
	logger.info(module_name, L"Room server v%s started", version().c_str());
}

void room_server::init(const vec_sensors &sensors, const vec_actuators &actuators)
{
	using std::chrono::milliseconds;
	_sensors = sensors;
	_actuators = actuators;
	_parser.clear();
	_parser.set(L"wait", [](auto& params) { auto ms = get_arg<milliseconds>(params, 0); std::this_thread::sleep_for(ms); return value_t{ 1 }; });
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
		auto ver = winrt::Windows::ApplicationModel::Package::Current().Id().Version();
		wchar_t ver_str[20];
		swprintf(ver_str, _countof(ver_str), L"%d.%d.%d.%d", ver.Major, ver.Minor, ver.Build, ver.Revision);
		return { ver_str };
	} catch(...)	{}
	return L"debug"s;
}

room_server::ip_info room_server::get_ip()
{
	ip_info ips = { L"-", L"-" };
	try	{
		for(auto& profile : NetworkInformation::GetConnectionProfiles()) {
			for(auto& host : NetworkInformation::GetHostNames()) {
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

wstring room_server::get_rules()
{
	return _rules.to_string();
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
	return sensors.ToString();
}

IAsyncAction room_server::pair_remote()
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

IAsyncAction room_server::start()
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

		rule_status status;

		auto result = _parser.eval(rule.condition, true);
		if(is_error(result)) {
			logger.error(module_name, L"error evaluating condition '%s': %s", rule.condition.c_str(), get_error_msg(result));
			status = rule_status::error;
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