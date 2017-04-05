#pragma once

using winrt::Windows::Networking::Sockets::DatagramSocket;
using winrt::Windows::Networking::Connectivity::NetworkAdapter;

struct ip_address {
	bool empty() { return *(uint32_t*)addr == 0u; }
	uint8_t	addr[4];
};

class motor_ctrl
{
public:
	motor_ctrl();
	~motor_ctrl();

	std::future<ip_address> resolve(std::wstring_view name);
private:
	DatagramSocket udp;
};

