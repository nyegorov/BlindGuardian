#pragma once

#include "Sensor.h"
#include "parser.h"

namespace BlindGuardian {

//using Windows::Data::Json;

enum class rule_status { error, active, inactive };

struct rule {
	rule(const char *name, const char *condition, const char *action) : name(name), condition(condition), action(action) {}
	std::string name;
	std::string condition;
	std::string action;
	rule_status	status = rule_status::inactive;
};

class RulesEngine
{
public:
	using vec_rules = std::vector<rule>;
	using vec_sensors = std::vector<ISensor*>;
	using vec_actuators = std::vector<IActuator*>;

	RulesEngine(const vec_sensors &sensors, const vec_actuators &actuators);
	void UpdateRules(const vec_rules& rules);
	void Run();

private:
	std::vector<rule>	_rules;
	vec_sensors			_sensors;
	vec_actuators		_actuators;
	NScript				_parser;
};

}