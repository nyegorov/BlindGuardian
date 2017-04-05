#include "pch.h"
#include "udns_resolver.h"
#include "debug_stream.h"

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

	if(byteCount < 5)	return;
	uint8_t cmd = reader.ReadByte();
	if(cmd != '#')		return;

	uint8_t ip_addr[4], name_len;
	reader.ReadBytes(ip_addr);
	name_len = reader.ReadByte();
	wstring name = reader.ReadString(name_len);
	std::wstringstream ws;
	ws << ip_addr[0] << '.' << ip_addr[1] << '.' << ip_addr[2] << '.' << ip_addr[3];
	wdebug << L"# - uDNS announce: " << name << L" -> " << ws.str() << std::endl;

	lock_t lock(_mutex);
	_names.emplace(name, ws.str());
}

udns_resolver::udns_resolver()
{
	try {
		_socket.MessageReceived([this](auto& socket, auto& args) { on_message(socket, args); });
		_socket.Control().MulticastOnly(true);
		_socket.BindServiceNameAsync(udns_port_in).get();
		_socket.JoinMulticastGroup(_multicast_group);
	} catch(const winrt::hresult_error& hr) {
		OutputDebugStringW(wstring(hr.message()).c_str());
	}
}

udns_resolver::~udns_resolver()
{
}

std::future<void> udns_resolver::refresh()
{
	try {
		auto os = co_await _socket.GetOutputStreamAsync(_multicast_group, udns_port_out);
		DataWriter writer(os);
		writer.WriteByte('$');
		co_await writer.StoreAsync();
	} catch(const winrt::hresult_error& hr) {
		OutputDebugStringW(wstring(hr.message()).c_str());
	}
	co_return;
}

HostName udns_resolver::get_address(const std::wstring& name) const
{
	lock_t lock(_mutex);
	auto it = _names.find(name);
	return it == _names.end() ? HostName{ nullptr } : it->second;
}
