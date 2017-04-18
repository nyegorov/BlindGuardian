#pragma once

#include "value.h"
#include "sensors.h"
#include "log_manager.h"
#include "udns_resolver.h"
#include "config_manager.h"

using std::wstring;
using namespace std::chrono_literals;

namespace roomctrl {

#pragma pack(push, 1)
union cmd_status {
	struct {
		uint8_t status;
		uint8_t temp;
		uint32_t light;
	};
	uint8_t data[6];
};

union cmd_pos {
	struct {
		uint8_t position;
	};
	uint8_t data[1];
};
#pragma pack(pop)

class esp8266_motor : public i_motor
{
	using IAsyncInfo = winrt::Windows::Foundation::IAsyncInfo;
	using StreamSocket = winrt::Windows::Networking::Sockets::StreamSocket;
	using milliseconds = std::chrono::milliseconds;
public:
	esp8266_motor(std::wstring_view remote_host, udns_resolver& udns, log_manager& log);
	~esp8266_motor();

	wstring host_name() const { return _host; }
	void open()				{ cmd_pos cmd; send_cmd(_udns.get_address(_host), 'o', {}, cmd.data); _position.set(cmd.position); }
	void close()			{ cmd_pos cmd; send_cmd(_udns.get_address(_host), 'c', {}, cmd.data); _position.set(cmd.position); }
	void setpos(value_t pos) {
		auto it = std::get_if<value_type>(&pos);
		if(it) {
			cmd_pos cmd{ (uint8_t)*it };
			send_cmd(_udns.get_address(_host), 'p', cmd.data, cmd.data);
			_position.set(cmd.position);
		}
	}
	void reset()			{ send_cmd(_udns.get_address(_host), 'r', {}, {}); _udns.reset(); }

	i_sensor* get_light()	{ return &_light; }
	i_sensor* get_temp()	{ return &_temp; }
	i_sensor* get_pos()		{ return &_position; }
	bool online()			{ return _retries < 3; }
	void set_timeout(milliseconds timeout) { _timeout = timeout; }

protected:
	bool wait_timeout(IAsyncInfo action);
	bool connect(HostName host);
	bool send_cmd(HostName host, uint8_t cmd, winrt::array_view<const uint8_t> inbuf, winrt::array_view<uint8_t> outbuf);
	void update_sensors();

	class remote_sensor : public sensor
	{
		bool			_master;
		esp8266_motor&	_remote;
	public:
		remote_sensor(const wchar_t *name, esp8266_motor& remote, bool master) : sensor(name), _remote(remote), _master(master) { }
		void update() override;
	};

	log_manager&		_log;
	udns_resolver&		_udns;
	wstring				_host;
	milliseconds		_timeout{ 1s };
	StreamSocket		_socket{ nullptr };
	std::atomic<int>	_retries{ 0 };
	remote_sensor		_temp{ L"temp_out", *this, true };
	remote_sensor		_light{ L"light", *this, false };
	remote_sensor		_position{ L"position", *this, false };
};

}