#pragma once

#include "Sensor.h"
#include "parser.h"

namespace BlindGuardian {

//using Windows::Data::Json;

enum class rule_status { error, active, inactive };

struct rule {
	rule(const string& name, const string& condition, const string& action) : name(name), condition(condition), action(action) {}
	string name;
	string condition;
	string action;
	rule_status	status = rule_status::inactive;
};

class RoomEngine
{
public:
	using vec_rules = std::vector<rule>;
	using vec_sensors = std::vector<ISensor*>;
	using vec_actuators = std::vector<IActuator*>;

	RoomEngine(const vec_sensors &sensors, const vec_actuators &actuators);
	void update_rules(const vec_rules& rules);
	void run();
	value_t eval(const char *expr);

private:
	std::vector<rule>	_rules;
	vec_sensors			_sensors;
	vec_actuators		_actuators;
	NScript				_parser;
};

}