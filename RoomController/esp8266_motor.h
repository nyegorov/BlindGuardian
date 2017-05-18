#pragma once

#include "value.h"
#include "sensors.h"
#include "log_manager.h"
#include "udns_resolver.h"
#include "config_manager.h"

#define MAX_RETRIES_BEFORE_RESET	20
#define MAX_RETRIES_BEFORE_OFFLINE	3

using std::wstring;
using namespace std::chrono_literals;

namespace roomctrl {

class esp8266_motor final : public i_motor
{
	using IAsyncInfo = winrt::Windows::Foundation::IAsyncInfo;
	using StreamSocket = winrt::Windows::Networking::Sockets::StreamSocket;
	using milliseconds = std::chrono::milliseconds;
public:
	esp8266_motor(std::wstring_view remote_host, udns_resolver& udns);
	~esp8266_motor();

	wstring host_name() const { return _host; }
	wstring version()   const { return _version; }
	void start() override;
	void open() override;
	void stop() override {};
	void close() override;
	void reset();

	i_sensor* get_light()	{ return &_light; }
	i_sensor* get_temp()	{ return &_temp; }
	i_sensor* get_pos()		{ return &_position; }
	bool online()			{ return _retries < MAX_RETRIES_BEFORE_OFFLINE; }
	void set_timeout(milliseconds sensors, milliseconds actions) { _timeout_sensors = sensors; _timeout_actions = actions; }

protected:
	bool wait_timeout(IAsyncInfo action, milliseconds timeout);
	bool connect(HostName host, milliseconds timeout);
	bool send_cmd(HostName host, uint8_t cmd, winrt::array_view<const uint8_t> inbuf, winrt::array_view<uint8_t> outbuf, milliseconds timeout);
	template<class CMD> bool send_cmd(HostName host, CMD& cmd, milliseconds timeout);
	template<class CMD> void do_action(typename CMD::out_type param);
	void update_sensors();
	void query_version();

	class remote_sensor : public sensor
	{
		bool			_master;
		esp8266_motor&	_remote;
	public:
		remote_sensor(const wchar_t *name, esp8266_motor& remote, bool master) : sensor(name), _remote(remote), _master(master) { }
		void update() override;
	};

	udns_resolver&		_udns;
	wstring				_host;
	wstring				_version = L"Unknown";
	milliseconds		_timeout_sensors{ 1s };
	milliseconds		_timeout_actions{ 1s };
	StreamSocket		_socket{ nullptr };
	std::atomic<int>	_retries{ 0 };
	remote_sensor		_temp{ L"temp_out", *this, true };
	remote_sensor		_light{ L"light", *this, false };
	remote_sensor		_position{ L"position", *this, false };
};

}