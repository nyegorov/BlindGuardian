#pragma once

#include "value.h"
#include "sensors.h"
#include "udns_resolver.h"

using std::wstring;

namespace roomctrl {

class motor_ctrl : public actuator
{
public:
	motor_ctrl(std::wstring_view name, std::wstring_view remote_host, udns_resolver& udns);
	~motor_ctrl();

	wstring host_name() const { return _host; }
	action open{  L"open",  [this](auto&) { send_command('o'); } };
	action close{ L"close", [this](auto&) { send_command('c'); } };
	std::vector<const i_action*> actions() const { return{ &open, &close }; }

	std::future<value_t> get_sensor_async(uint8_t sensor);
	value_t get_sensor(uint8_t sensor);
	std::future<void> do_action_async(uint8_t command);
	void send_command(uint8_t command);
private:
	std::future<bool> send_cmd(HostName host, uint8_t cmd, winrt::array_view<const uint8_t> inbuf, winrt::array_view<uint8_t> outbuf);
	udns_resolver&	_udns;
	wstring			_host;
};

class remote_sensor : public sensor
{
	uint8_t		_cmdbyte;
	motor_ctrl& _remote;
public:
	remote_sensor(const wchar_t *name, uint8_t cmd_byte, motor_ctrl& remote) : sensor(name), _cmdbyte(cmd_byte), _remote(remote) { }
	void update() override;

};

}