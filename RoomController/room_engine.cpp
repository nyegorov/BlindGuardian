#include "pch.h"
#include "room_engine.h"

using namespace winrt::Windows::Data::Json;

namespace roomctrl {

room_server::room_server(const path& db_path, const vec_sensors &sensors, const vec_actuators &actuators) :
	_sensors(sensors), _actuators(actuators), _rules(db_path)
{
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
	return json.ToString();
}

value_t room_server::eval(const wchar_t *expr)
{
	return _parser.eval(expr);
}

std::future<void> room_server::start()
{
	_server.add(L"/", L"html/room_status.html");
	_server.add(L"/status", L"html/room_status.html");
	_server.add(L"/edit", L"html/edit_rule.html");
	_server.add(L"/back.jpg", L"html/img/background.jpg");
	_server.add(L"/favicon.ico", L"html/img/favicon.ico");
	_server.add(L"/room.json", [this](auto&, auto&) { return std::make_tuple(content_type::json, get_sensors()); });
	_server.add(L"/rules.json", [this](auto&, auto&) { return std::make_tuple(content_type::json, get_rules()); });
	_server.add(L"/rule.json", [this](auto& r, auto&) { return std::make_tuple(content_type::json, _rules.get(std::stoul(r.params[L"id"s])).to_string()); });
	_server.add_action(L"set_pos", [this](auto&, auto& value) { });
	_server.add_action(L"save_rule", [this](auto& req, auto& value) {
		_rules.save({ std::stoul(value), req.params[L"rule_name"s], req.params[L"condition"s], req.params[L"action"s] });
	});
	_server.add_action(L"delete_rule", [this](auto&, auto& value) { _rules.remove(std::stoul(value)); });
	co_await _server.start();
	co_return;
}

void room_server::run()
{
	for(auto& sensor : _sensors) sensor->update();

	for(const auto& rule : _rules.get_all())
	{
		auto result = _parser.eval(rule.condition);
		auto status = is_error(result) ? rule_status::error :
			result == value_t{ 0 } ? rule_status::inactive : rule_status::active;

		if(status != rule.status && status == rule_status::active) {
			result = _parser.eval(rule.action);
			if(is_error(result))	status = rule_status::error;
		}
		_rules.set_status(rule.id, status);
	}
}

}