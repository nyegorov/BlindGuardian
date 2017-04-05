#pragma once

using winrt::Windows::Networking::Sockets::DatagramSocket;
using winrt::Windows::Networking::Sockets::DatagramSocketMessageReceivedEventArgs;
using winrt::Windows::Networking::HostName;

using std::wstring;

class udns_resolver
{
	void on_message(const DatagramSocket&, const DatagramSocketMessageReceivedEventArgs&);
	using lock_t = std::lock_guard<std::mutex>;
	mutable std::mutex			_mutex;
	DatagramSocket				_socket;
	std::map<wstring, HostName>	_names;
	HostName					_multicast_group{ L"239.255.1.2" };
public:
	udns_resolver();
	~udns_resolver();
	std::future<void> refresh();
	HostName get_address(const std::wstring& name) const;
};

