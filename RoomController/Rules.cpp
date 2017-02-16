#include "pch.h"
#include "Rules.h"

namespace BlindGuardian {

RulesEngine::RulesEngine(const vec_sensors &sensors, const vec_actuators &actuators) : _sensors(sensors), _actuators(actuators)
{
	for(auto& ps : _sensors) {
		_parser.set(ps->GetName(), [ps](value_t) {return ps->GetValue(); });
	}
	for(auto& pa : _actuators) {
		_parser.set(pa->GetName(), [pa](value_t v) {return pa->Activate(v), value_t{ 1 }; });
	}
}

void RulesEngine::UpdateRules(const vec_rules& rules)
{
	_rules = rules;
}

void RulesEngine::Run()
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