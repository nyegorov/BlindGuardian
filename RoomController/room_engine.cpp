#include "pch.h"
#include "room_engine.h"

using namespace winrt::Windows::Data::Json;

const wchar_t module_name[] = L"ROOM";

namespace roomctrl {

room_server::room_server(const path_t& path) : _rules(path / "rules.json"), _config(path / "config.json"), _log(path / "log.txt")
{
	init({ &_temp_in, _motctrl.get_temp(), _motctrl.get_light(), &_motion, &_time }, { &_motctrl });
}

void room_server::init(const vec_sensors &sensors, const vec_actuators &actuators)
{
	_sensors = sensors;
	_actuators = actuators;
	for(auto& ps : _sensors) {
		_parser.set(ps->name(), ps);
		_parser.set(ps->name() + L".min", [ps](auto&) {return ps->min(); });
		_parser.set(ps->name() + L".max", [ps](auto&) {return ps->max(); });
		_parser.set(ps->name() + L".reset", [ps](auto&) {return ps->reset(), ps->value(); });
	}
	for(auto& pa : _actuators) {
		auto obj = pa->name();
		for(auto& paction : pa->actions()) {
			_parser.set(obj + L"." + paction->name(), [paction](auto& p) {return paction->activate(p), value_t{ 1 }; });
		}
	}
}

wstring room_server::get_log()
{
	return _log.to_string();
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
	auto mot_ip = _udns.get_address(_motctrl.host_name());
	json.SetNamedValue(_motctrl.host_name(), JsonValue::CreateStringValue(_motctrl.online() && mot_ip ? mot_ip.DisplayName() : L"") );
	JsonObject sensors;
	sensors.SetNamedValue(L"sensors", json);
	return sensors.ToString();
}

value_t room_server::eval(const wchar_t *expr)
{
	return _parser.eval(expr);
}

std::future<void> room_server::start()
{
	_log.enable_debug(_config.get(L"enable_debug", false));
	_motctrl.set_timeout(std::chrono::milliseconds(_config.get(L"socket_timeout", 2000)));
	co_await _udns.start();
	_server.add(L"/", L"html/room_status.html");
	_server.add(L"/status", L"html/room_status.html");
	_server.add(L"/edit", L"html/edit_rule.html");
	_server.add(L"/log", L"html/server_log.html");
	_server.add(L"/styles.css", L"html/styles.css");
	_server.add(L"/back.jpg", L"html/img/background.jpg");
	_server.add(L"/favicon.ico", L"html/img/favicon.ico");
	_server.add(L"/room.json", [this](auto&, auto&) { return std::make_tuple(content_type::json, get_sensors()); });
	_server.add(L"/rules.json", [this](auto&, auto&) { return std::make_tuple(content_type::json, get_rules()); });
	_server.add(L"/rule.json", [this](auto& r, auto&) { return std::make_tuple(content_type::json, _rules.get(std::stoul(r.params[L"id"s])).to_string()); });
	_server.add(L"/log.json", [this](auto&, auto&) { return std::make_tuple(content_type::json, get_log()); });
	_server.add_action(L"set_pos", [this](auto&, auto& value) {
		if(std::stoul(value) == 100)	_motctrl.open();
		if(std::stoul(value) == 0)		_motctrl.close();
	});
	_server.add_action(L"save_rule", [this](auto& req, auto& value) {
		_rules.save({ std::stoul(value), req.params[L"rule_name"s], req.params[L"condition"s], req.params[L"action"s] });
	});
	_server.add_action(L"delete_rule", [this](auto&, auto& value) { _rules.remove(std::stoul(value)); });
	co_await _server.start();
	co_return;
}

void room_server::run()
{
	if(_inprogress)	return;
	_inprogress = true;

	for(auto& sensor : _sensors) sensor->update();

	for(const auto& rule : _rules.get_all())
	{
		rule_status status;

		auto result = _parser.eval(rule.condition);
		if(is_error(result)) {
			_log.error(module_name, L"error evaluating condition '%s': %s", rule.condition.c_str(), get_error_msg(result));
			status = rule_status::error;
		} else {
			status = result == value_t{ 0 } ? rule_status::inactive : rule_status::active;
		}

		if(status != rule.status && status == rule_status::active) {
			result = _parser.eval(rule.action);
			_log.info(module_name, L"activated rule '%s' (%s)", rule.name.c_str(), rule.action.c_str());
			if(is_error(result)) {
				_log.error(module_name, L"error evaluating action '%s': %s", rule.action.c_str(), get_error_msg(result));
				status = rule_status::error;
			}
		}
		_rules.set_status(rule.id, status);
	}

	_inprogress = false;
}

}