#include "pch.h"
#include "room_engine.h"
#include "debug_stream.h"

using namespace winrt;
using namespace winrt::Windows::Data::Json;

const wchar_t module_name[] = L"ROOM";

namespace roomctrl {

room_server::room_server(const path_t& path) : _rules(path / "rules.json"), _config(path / "config.json")
{
	/*debug << "waiting for command..." << std::endl;
	while(true) {
		auto cmd = _dm35le.read_cmd(RX_PIN);
		if(cmd) debug << "got value: " << std::hex << cmd << std::dec << std::endl;
	}*/
	
	_http.on(L"/", L"html/room_status.html");
	_http.on(L"/status", L"html/room_status.html");
	_http.on(L"/edit", L"html/edit_rule.html");
	_http.on(L"/log", L"html/server_log.html");
	_http.on(L"/pair", L"html/pair_remote.html");
	_http.on(L"/styles.css", L"html/styles.css");
	_http.on(L"/back.jpg", L"html/img/background.jpg");
	_http.on(L"/favicon.ico", L"html/img/favicon.ico");
	_http.on(L"/room.json", [this](auto&, auto&) { return std::make_tuple(content_type::json, get_sensors()); });
	_http.on(L"/rules.json", [this](auto&, auto&) { return std::make_tuple(content_type::json, get_rules()); });
	_http.on(L"/rule.json", [this](auto& r, auto&) { return std::make_tuple(content_type::json, _rules.get(std::stoul(r.params[L"id"s])).to_string()); });
	//_http.on(L"/pair.json", [this](auto&, auto&) { return std::make_tuple(content_type::json, get_pair_info()); });
	_http.on(L"/log.json", [this](auto&, auto&) { return std::make_tuple(content_type::json, logger.to_string()); });
	_http.on_action(L"pair_remote", [this](auto&, auto& value) { pair_remote(); });
	_http.on_action(L"set_pos", [this](auto&, auto& value) {
		if(std::stoul(value) == 100)	_tasks.push([this]() {_motor.open(); });
		if(std::stoul(value) == 0)		_tasks.push([this]() {_motor.close(); });
	});
	_http.on_action(L"save_rule", [this](auto& req, auto& value) {
		auto id = _rules.save({ std::stoul(value), req.params[L"rule_name"s], req.params[L"condition"s], req.params[L"action"s] });
		_rules.set_status(id, rule_status::inactive);
	});
	_http.on_action(L"delete_rule", [this](auto&, auto& value) { _rules.remove(std::stoul(value)); });

	init(_sensors, _actuators);
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

wstring room_server::get_rules()
{
	return _rules.to_string();
}

wstring room_server::get_sensors()
{
	JsonObject json;
	for (auto& s : _sensors) {
		json.SetNamedValue(s->name(), is_error(s->value()) ? JsonValue::CreateStringValue(L"--") :
			JsonValue::CreateNumberValue(std::get<value_type>(*s->value())));
	}
	//auto mot_ip = _udns.get_address(_esp8266.host_name());
	json.SetNamedValue(L"remote_id", JsonValue::CreateNumberValue(_dm35le.get_remote_id()));
	json.SetNamedValue(L"motctrl",   JsonValue::CreateStringValue(_ext.online() ? _ext.remote_ip().DisplayName().c_str() : L"") );
	JsonObject sensors;
	sensors.SetNamedValue(L"sensors", json);
	return sensors.ToString();
}

struct pair_info {
	bool done;
	int remote_id;
	int commands;
	wstring to_string() {

	}
};

void room_server::pair_remote()
{
	pair_info info;
	_beeper.beep();
	if(_dm35le.pair_remote())	_beeper.beep();
	else						_beeper.beep(100ms), std::this_thread::sleep_for(200ms), _beeper.beep(100ms), std::this_thread::sleep_for(200ms), _beeper.beep(100ms);
}

value_t room_server::eval(const wchar_t *expr)
{
	return _parser.eval(expr);
}

std::future<void> room_server::start()
{
	co_await _tmp75.start();
	co_await _ext.start();
	co_await _http.start();
	co_await 500ms;
	_motor.start();
	_beeper.beep();
	co_return;
}

void room_server::run()
{
	if(_inprogress)	return;
	_inprogress = true;

	_led.invert();

	while(!_tasks.empty()) {
		std::function<void()> f;
		_tasks.try_pop(f);
		f();
	}

	for(auto& sensor : _sensors) sensor->update();

	for(const auto& rule : _rules.get_all())
	{
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

	_inprogress = false;
}

}