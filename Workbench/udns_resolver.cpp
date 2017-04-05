#include "pch.h"
#include "udns_resolver.h"
#include "debug_stream.h"

extern wdebugstream wdebug;

using namespace winrt::Windows::Networking;
using namespace winrt::Windows::Networking::Sockets;
using namespace winrt::Windows::Networking::Connectivity;
using namespace winrt::Windows::Storage::Streams;

const wchar_t udns_port_in[] = L"4762";
const wchar_t udns_port_out[] = L"4761";

void udns_resolver::on_message(const DatagramSocket &, const DatagramSocketMessageReceivedEventArgs &args)
{
	auto reader = args.GetDataReader();
	auto byteCount = reader.UnconsumedBufferLength();

	wdebug << L"IP: " << std::wstring(args.RemoteAddress().DisplayName()) << L", Bytes: " << (unsigned)byteCount << std::endl;

	if(byteCount < 5)	return;
	uint8_t cmd = reader.ReadByte();
	if(cmd != '#')		return;

	uint8_t ip_addr[4], name_len;
	reader.ReadBytes(ip_addr);
	name_len = reader.ReadByte();
	wstring name = reader.ReadString(name_len);
	std::wstringstream ws;
	ws << ip_addr[0] << '.' << ip_addr[1] << '.' << ip_addr[2] << '.' << ip_addr[3];
	_names.emplace(name, ws.str());

	wdebug << name << L" -> " << ws.str() << std::endl;
}

udns_resolver::udns_resolver()
{
	try {
		for(auto host : NetworkInformation::GetHostNames()) {
			if(host.Type() != HostNameType::Ipv4)	continue;
			IPInformation ip_info = host.IPInformation();
			if(!ip_info)	continue;
			NetworkAdapter adapter = ip_info.NetworkAdapter();
			if(!adapter)	continue;
			DatagramSocket listener;
			listener.MessageReceived([this](auto& socket, auto& args) { on_message(socket, args); });
			wdebug << (int)adapter.NetworkItem().GetNetworkTypes() << std::endl;
			wdebug << "listen to: " << wstring(host.DisplayName()) << L":" << udns_port_out << std::endl;
			listener.Control().MulticastOnly(true);
			listener.BindServiceNameAsync(udns_port_in, adapter);
			listener.JoinMulticastGroup(_multicast_group);
			_listeners.emplace_back(host, listener);
		}
	} catch(const winrt::hresult_error& hr) {
		OutputDebugStringW(wstring(hr.message()).c_str());
	}
	//refresh();
}


udns_resolver::~udns_resolver()
{
}

std::future<void> udns_resolver::refresh()
{
	for(auto& n : _listeners) {
		//DatagramSocket udp;
		//auto os = co_await udp.GetOutputStreamAsync(EndpointPair(n.first, L"", _multicast_group, udns_port_out));
		//auto os = co_await n.second.GetOutputStreamAsync(EndpointPair(n.first, L"", _multicast_group, udns_port_out));
		auto os = co_await n.second.GetOutputStreamAsync(_multicast_group, udns_port_out);
		DataWriter writer(os);
		writer.WriteByte('$');
		co_await writer.StoreAsync();
	}
	co_return;
}

HostName udns_resolver::get_address(const std::wstring& name) const
{
	auto it = _names.find(name);
	return it == _names.end() ? HostName{ nullptr } : it->second;
}
