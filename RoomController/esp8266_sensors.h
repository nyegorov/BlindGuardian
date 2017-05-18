#pragma once

#include "value.h"
#include "sensors.h"
#include "log_manager.h"
#include "udns_resolver.h"
#include "config_manager.h"

using winrt::Windows::Networking::Sockets::DatagramSocket;
using winrt::Windows::Networking::Sockets::DatagramSocketMessageReceivedEventArgs;
using namespace std::chrono_literals;

namespace roomctrl {

class esp8266_sensors
{
public:
	esp8266_sensors(std::wstring_view udp_port, std::wstring_view multicast_group, sensor& temp, sensor& light);
	~esp8266_sensors();
	std::future<void> start();

protected:
	void on_message(const DatagramSocket&, const DatagramSocketMessageReceivedEventArgs&);
	DatagramSocket		_socket;
	HostName			_multicast_group;
	std::wstring		_udp_port;
	sensor&				_temp;
	sensor&				_light;
	std::chrono::steady_clock _last_time;
};

}