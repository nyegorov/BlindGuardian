#pragma once

#include "Sensor.h"
#include "parser.h"

namespace BlindGuardian {

//using Windows::Data::Json;

class RulesEngine
{
	enum class rule_status {error, active, inactive};
	struct rule {
		std::string name;
		std::string condition;
		std::string action;
		rule_status	status;
	};

	std::vector<rule>		_rules;
	std::vector<ISensor*>	_sensors;
	std::vector<IActuator*>	_actuators;
	NScript					_parser;
public:
	RulesEngine(std::vector<ISensor*> &sensors, std::vector<IActuator*> &actuators);
	void UpdateRules(int source);
	void Run();
};

}