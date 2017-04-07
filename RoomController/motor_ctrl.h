#pragma once

#include "value.h"
#include "sensors.h"
#include "udns_resolver.h"

using std::wstring;

namespace roomctrl {

class motor_ctrl;

class remote_sensor : public sensor
{
	motor_ctrl& _remote;
public:
	remote_sensor(const wchar_t *name, motor_ctrl& remote) : sensor(name), _remote(remote) { }
	void update() override;

};

class motor_ctrl : public actuator
{
public:
	friend class remote_sensor;
	motor_ctrl(std::wstring_view name, std::wstring_view remote_host, udns_resolver& udns);
	~motor_ctrl();

	wstring host_name() const { return _host; }
	action open{  L"open",  [this](auto&) { do_action('o'); } };
	action close{ L"close", [this](auto&) { do_action('c'); } };
	std::vector<const i_action*> actions() const { return{ &open, &close }; }

	i_sensor* get_light()	{ return &_light; }
	i_sensor* get_temp()	{ return &_temp; }
	bool online()			{ return _online; }

	std::future<void> do_action(uint8_t command);
protected:
	std::future<void> update_sensors();
	std::future<bool> send_cmd(HostName host, uint8_t cmd, winrt::array_view<const uint8_t> inbuf, winrt::array_view<uint8_t> outbuf);
	//bool send_cmd(HostName host, uint8_t cmd, winrt::array_view<const uint8_t> inbuf, winrt::array_view<uint8_t> outbuf);

	udns_resolver&		_udns;
	wstring				_host;
	std::atomic<bool>	_inprogress{ false };
	std::atomic<bool>	_online{ false };
	remote_sensor		_light{ L"light", *this };
	remote_sensor		_temp{ L"temp_out", *this };
};

}