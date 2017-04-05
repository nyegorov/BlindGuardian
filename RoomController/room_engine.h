#pragma once

#include "sensors.h"
#include "parser.h"
#include "http_server.h"
#include "rules_db.h"

namespace roomctrl {

using std::experimental::filesystem::path;

class room_server
{
public:
	using vec_rules = std::vector<rule>;
	using vec_sensors = std::vector<i_sensor*>;
	using vec_actuators = std::vector<i_actuator*>;

	room_server(const path& db_path, const vec_sensors &sensors, const vec_actuators &actuators);
	std::future<void> start();
	wstring get_rules();
	wstring get_sensors();
	void run();
	value_t eval(const wchar_t *expr);

private:
	http_server			_server{ L"80", L"Room configuration server" };
	rules_db			_rules;
	vec_sensors			_sensors;
	vec_actuators		_actuators;
	NScript				_parser;
};

}