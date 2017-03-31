#pragma once

#include "Sensor.h"
#include "parser.h"
#include "rules_db.h"

namespace roomctrl {

using std::experimental::filesystem::path;

class RoomEngine
{
public:
	using vec_rules = std::vector<rule>;
	using vec_sensors = std::vector<i_sensor*>;
	using vec_actuators = std::vector<i_actuator*>;

	RoomEngine(const path& db_path, const vec_sensors &sensors, const vec_actuators &actuators);
	void update_rules(const vec_rules& rules);
	void update_rules(const wstring& rules);
	wstring get_rules();
	void run();
	value_t eval(const wchar_t *expr);

private:
	rules_db			_rules;
	vec_sensors			_sensors;
	vec_actuators		_actuators;
	NScript				_parser;
};

}