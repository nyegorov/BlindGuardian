#pragma once

#include "value.h"
#include "udns_resolver.h"

using winrt::Windows::Networking::Sockets::DatagramSocket;
using winrt::Windows::Networking::Connectivity::NetworkAdapter;

namespace roomctrl {

class motor_ctrl
{
public:
	motor_ctrl(std::wstring_view name, udns_resolver& udns);
	~motor_ctrl();

	std::future<value_t> get_sensor(uint8_t sensor);
private:
	wstring			_name;
	udns_resolver&	_udns;
};

}