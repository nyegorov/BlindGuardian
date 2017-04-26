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

	esp8266_motor	_esp8266{ L"motctrl", _udns };
	time_sensor		_time{ L"time" };
	tmp75_sensor	_tmp75{ L"temp_in", tmp75_sensor::res12bit };
	hcsr501_sensor	_hcsr501{ L"inactivity", 18 };
	motor_ctrl		_motors{ L"blind" , {&_esp8266} };
	beeper			_beeper{ L"beeper", 24 };
	led				_led{ L"beeper", 23 };

	vec_sensors		_sensors{ &_tmp75, _esp8266.get_temp(), _esp8266.get_light(), _esp8266.get_pos(), &_hcsr501, &_time };
	vec_actuators	_actuators{ &_motors, &_beeper, &_led };
};

}