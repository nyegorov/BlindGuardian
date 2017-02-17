#include "pch.h"
#include "Rules.h"

namespace BlindGuardian {

RoomEngine::RoomEngine(const vec_sensors &sensors, const vec_actuators &actuators) : _sensors(sensors), _actuators(actuators)
{
	for(auto& ps : _sensors) {
		_parser.set(ps->name(), [ps](value_t) {return ps->value(); });
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

void RoomEngine::run()
{
	for(auto& pr : _rules)
	{
		auto result = _parser.eval(pr.condition);
		auto status = result.type == value_tag::error ? rule_status::error :
			result.value == 0 ? rule_status::inactive : rule_status::active;

		if(status != pr.status && status == rule_status::active) {
			_parser.eval(pr.action);
		}
		pr.status = status;
	}
}

}