#pragma once

#include "value.h"
#include "sensors.h"
#include "log_manager.h"
#include "udns_resolver.h"
#include "config_manager.h"

using std::wstring;
using namespace std::chrono_literals;

namespace roomctrl {

class motor_ctrl;

class remote_sensor : public sensor
{
	bool		_master;
	motor_ctrl& _remote;
public:
	remote_sensor(const wchar_t *name, motor_ctrl& remote, bool master) : sensor(name), _remote(remote), _master(master) { }
	void update() override;

};

class motor_ctrl : public actuator
{
	using IAsyncInfo = winrt::Windows::Foundation::IAsyncInfo;
	using StreamSocket = winrt::Windows::Networking::Sockets::StreamSocket;
	using milliseconds = std::chrono::milliseconds;
public:
	friend class remote_sensor;
	motor_ctrl(std::wstring_view name, std::wstring_view remote_host, udns_resolver& udns, log_manager& log);
	~motor_ctrl();

	wstring host_name() const { return _host; }
	action open{  L"open",  [this](auto&) { do_action('o'); } };
	action close{ L"close", [this](auto&) { do_action('c'); } };
	std::vector<const i_action*> actions() const { return{ &open, &close }; }

	i_sensor* get_light()	{ return &_light; }
	i_sensor* get_temp()	{ return &_temp; }
	bool online()			{ return _retries < 3; }
	void reset()			{ do_action('r'); _udns.reset(); }
	void set_timeout(milliseconds timeout) { _timeout = timeout; }

protected:
	bool wait_timeout(IAsyncInfo action);
	bool connect(HostName host);
	void do_action(uint8_t command);
	bool send_cmd(HostName host, uint8_t cmd, winrt::array_view<const uint8_t> inbuf, winrt::array_view<uint8_t> outbuf);
	void update_sensors();

	log_manager&		_log;
	udns_resolver&		_udns;
	wstring				_host;
	milliseconds		_timeout{ 1s };
	StreamSocket		_socket{ nullptr };
	std::atomic<bool>	_inprogress{ false };
	std::atomic<int>	_retries{ 0 };
	remote_sensor		_temp{ L"temp_out", *this, true };
	remote_sensor		_light{ L"light", *this, false };
};

}