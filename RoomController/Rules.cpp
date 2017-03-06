#include "pch.h"
#include "Rules.h"

namespace BlindGuardian {

RoomEngine::RoomEngine(const vec_sensors &sensors, const vec_actuators &actuators) : _sensors(sensors), _actuators(actuators)
{
	for(auto& ps : _sensors) {
		_parser.set(ps->name(), [ps](value_t) {return ps->value(); });
		_parser.set(ps->name() + ".min", [ps](value_t) {return ps->min(); });
		_parser.set(ps->name() + ".max", [ps](value_t) {return ps->max(); });
		_parser.set(ps->name() + ".reset", [ps](value_t) {return ps->reset(), ps->value(); });
	}
	for(auto& pa : _actuators) {
		string obj = pa->name();
		for(auto& paction : pa->actions()) {
			_parser.set(obj + "." + paction->name(), [paction](value_t v) {return paction->activate(v), value_t{ 1 }; });
		}
	}
}

void RoomEngine::update_rules(const vec_rules& rules)
{
	_rules = rules;
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