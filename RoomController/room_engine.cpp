#include "pch.h"
#include "room_engine.h"

using namespace winrt::Windows::Data::Json;

namespace roomctrl {

/*wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;
	return converterX.from_bytes(str);
}

string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;
	return converterX.to_bytes(wstr);
}*/

RoomEngine::RoomEngine(const path& db_path, const vec_sensors &sensors, const vec_actuators &actuators) :
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

void RoomEngine::update_rules(const vec_rules& rules)
{
	// TODO: _rules = rules;
}

void RoomEngine::update_rules(const wstring& rules)
{
/*	_rules.
	auto json = JsonObject::Parse(rules);
	auto jrules = json.GetNamedArray(L"rules");
	vec_rules vr;
	for(auto jri : jrules)
	{
		auto jr = jri.GetObject();
		vr.emplace_back(
			ws2s(jr.GetNamedString(L"name")),
			ws2s(jr.GetNamedString(L"condition")),
			ws2s(jr.GetNamedString(L"body"))
		);
	}
	update_rules(vr);*/
}

wstring RoomEngine::get_rules()
{
	return _rules.to_string();
}

value_t RoomEngine::eval(const wchar_t *expr)
{
	return _parser.eval(expr);
}

void RoomEngine::run()
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