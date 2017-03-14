#include "pch.h"
#include "Rules.h"

using namespace Windows::Data::Json;

namespace BlindGuardian {

wstring s2ws(const std::string& str)
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
}

RoomEngine::RoomEngine(const vec_sensors &sensors, const vec_actuators &actuators) : _sensors(sensors), _actuators(actuators)
{
	for(auto& ps : _sensors) {
		_parser.set(ps->name(), *ps);
		_parser.set(ps->name() + ".min", [ps](auto&) {return ps->min(); });
		_parser.set(ps->name() + ".max", [ps](auto&) {return ps->max(); });
		_parser.set(ps->name() + ".reset", [ps](auto&) {return ps->reset(), ps->value(); });
	}
	for(auto& pa : _actuators) {
		string obj = pa->name();
		for(auto& paction : pa->actions()) {
			_parser.set(obj + "." + paction->name(), [paction](auto& p) {return paction->activate(p), value_t{ 1 }; });
		}
	}
}

void RoomEngine::update_rules(const vec_rules& rules)
{
	_rules = rules;
}

void RoomEngine::update_rules(const string& rules)
{
	auto json = JsonObject::Parse(ref new Platform::String(s2ws(rules).c_str()));
	auto jrules = json->GetNamedArray(L"rules");
	vec_rules vr;
	for(auto jri = jrules->First(); jri->HasCurrent; jri->MoveNext())
	{
		auto jr = jri->Current->GetObject();
		vr.emplace_back(
			ws2s(jr->GetNamedString(L"name")->Data()),
			ws2s(jr->GetNamedString(L"condition")->Data()),
			ws2s(jr->GetNamedString(L"body")->Data())
		);
	}
	update_rules(vr);
}

value_t RoomEngine::eval(const char *expr)
{
	return _parser.eval(expr);
}

void RoomEngine::run()
{
	for(auto& sensor : _sensors) sensor->update();

	for(auto& rule : _rules)
	{
		auto result = _parser.eval(rule.condition);
		auto status = result.type == value_tag::error ? rule_status::error :
			result.value == 0 ? rule_status::inactive : rule_status::active;

		if(status != rule.status && status == rule_status::active) {
			result = _parser.eval(rule.action);
			if(result.type == value_tag::error)	status = rule_status::error;
		}
		rule.status = status;
	}
}

}