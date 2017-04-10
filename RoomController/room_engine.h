#pragma once

#include "log_manager.h"
#include "udns_resolver.h"
#include "motor_ctrl.h"
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

	room_server(const path& path);
	void init(const vec_sensors &sensors, const vec_actuators &actuators);
	std::future<void> start();
	wstring get_log();
	wstring get_rules();
	wstring get_sensors();
	void run();
	value_t eval(const wchar_t *expr);
	config_manager& config() { return _config; }

private:
	rules_db		_rules;
	vec_sensors		_sensors;
	vec_actuators	_actuators;
	NScript			_parser;
	log_manager		_log;
	config_manager	_config;
	http_server		_server{ L"80", L"Room configuration server", _log };
	udns_resolver	_udns{ _config, _log };
	motor_ctrl		_motctrl{ L"blind", L"motctrl", _udns, _config, _log };

	time_sensor		_time{ L"time" };
	missing_sensor	_temp_in{ L"temp_in" };
	missing_sensor	_motion{ L"inactivity" };
};

}