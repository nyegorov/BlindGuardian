#pragma once

#include "value.h"
#include "sensors.h"
#include "log_manager.h"
#include "udns_resolver.h"
#include "config_manager.h"

using winrt::Windows::Networking::HostName;
using winrt::Windows::Networking::Sockets::DatagramSocket;
using winrt::Windows::Networking::Sockets::DatagramSocketMessageReceivedEventArgs;
using namespace std::chrono_literals;
using std::chrono::steady_clock;

namespace roomctrl {

class esp8266_sensors
{
public:
	esp8266_sensors(std::wstring_view udp_port, std::wstring_view multicast_group, sensor& temp, sensor& light);
	~esp8266_sensors();
	winrt::Windows::Foundation::IAsyncAction start();
	HostName remote_ip() { return _remote_ip; }
	bool online()		 { return steady_clock::now() - _last_status_time.load() < 30s; }

protected:
	void on_message(const DatagramSocket&, const DatagramSocketMessageReceivedEventArgs&);
	DatagramSocket	_socket;
	HostName		_multicast_group;
	std::wstring	_udp_port;
	sensor&			_temp;
	sensor&			_light;
	HostName		_remote_ip{ nullptr };
	std::atomic<steady_clock::time_point>	_last_status_time{ steady_clock::time_point() };
};

}