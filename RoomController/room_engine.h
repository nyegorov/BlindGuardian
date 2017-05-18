#pragma once

#include "log_manager.h"
#include "udns_resolver.h"
#include "esp8266_motor.h"
#include "esp8266_sensors.h"
#include "dm35le_motor.h"
#include "motor_ctrl.h"
#include "sensors.h"
#include "parser.h"
#include "http_server.h"
#include "rules_db.h"

#define TX_PIN		17
#define RX_PIN		27
#define LED_PIN		4
#define	BEEPER_PIN	22
#define MOTION_PIN	18
#define TMP_ADDRESS	0x48
#define ESP_PORT	L"4760"
#define ESP_GROUP	L"224.0.0.100"

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

	sensor			_temp_out{ L"temp_out" };
	sensor			_light{ L"light" };
	sensor			_position{ L"position" };
	time_sensor		_time{ L"time" };
	tmp75_sensor	_tmp75{ L"temp_in", tmp75_sensor::res12bit, TMP_ADDRESS };
	hcsr501_sensor	_hcsr501{ L"inactivity", MOTION_PIN };
	esp8266_sensors	_ext{ ESP_PORT, ESP_GROUP, _temp_out, _light };

	dm35le_motor	_dm35le{ RX_PIN, TX_PIN, _position, _config };
	motor_ctrl		_motor{ L"blind" , {&_dm35le} };
	beeper			_beeper{ L"", BEEPER_PIN };
	led				_led{ L"led", LED_PIN };

	vec_sensors		_sensors{ &_tmp75, &_temp_out, &_light, &_position, &_hcsr501, &_time };
	vec_actuators	_actuators{ &_motor, &_beeper, &_led };
};

}