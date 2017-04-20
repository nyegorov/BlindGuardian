#pragma once

#include "log_manager.h"
#include "udns_resolver.h"
#include "esp8266_motor.h"
#include "motor_ctrl.h"
#include "sensors.h"
#include "parser.h"
#include "http_server.h"
#include "rules_db.h"

namespace roomctrl {

using path_t = std::experimental::filesystem::path;

class room_server
{
public:
	using vec_rules = std::vector<rule>;
	using vec_sensors = std::vector<i_sensor*>;
	using vec_actuators = std::vector<i_actuator*>;
	using cqueue = concurrency::concurrent_queue<std::function<void()>>;

	room_server(const path_t& path = L".");
	void init(const vec_sensors &sensors, const vec_actuators &actuators);
	std::future<void> start();
	wstring get_rules();
	wstring get_sensors();
	void run();
	value_t eval(const wchar_t *expr);
	config_manager& config()	{ return _config; }
	rules_db& rules()			{ return _rules; }

private:
	std::atomic<bool> _inprogress{ false };
	rules_db		_rules;
	cqueue			_tasks;
	NScript			_parser;
	config_manager	_config;
	http_server		_http{ L"80", L"Room configuration server"};
	udns_resolver	_udns{ &_config };

	esp8266_motor	_motor{ L"motctrl", _udns };
	time_sensor		_time{ L"time" };
	missing_sensor	_temp_in{ L"temp_in" };
	missing_sensor	_motion{ L"inactivity" };
	motor_ctrl		_motors{ L"blind" , {&_motor} };

	vec_sensors		_sensors{ &_temp_in, _motor.get_temp(), _motor.get_light(), _motor.get_pos(), &_motion, &_time };
	vec_actuators	_actuators{ &_motors };
};

}