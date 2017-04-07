#include "pch.h"
#include "udns_resolver.h"
#include "common.h"
#include "debug_stream.h"

using namespace winrt::Windows::Networking;
using namespace winrt::Windows::Networking::Connectivity;
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
	wdebug << L"# - announce: " << name << L" -> " << ws.str() << std::endl;

	lock_t lock(_mutex);
	_names.emplace(name, ws.str());
}

udns_resolver::udns_resolver()
{
	_names.emplace(L"localhost", HostName(L"localhost"));
	//_names.emplace(L"motctrl", HostName(L"192.168.6.53"));
}

udns_resolver::~udns_resolver()
{
}

std::future<void> udns_resolver::start()
{
	try {
		_socket = DatagramSocket();
		_socket.MessageReceived([this](auto& socket, auto& args) { on_message(socket, args); });
		_socket.Control().MulticastOnly(true);
		co_await _socket.BindServiceNameAsync(udns_port_in);
		_socket.JoinMulticastGroup(_multicast_group);
		co_await refresh();
	} catch(winrt::hresult_error& hr) {
		log_hresult(L"UDNS", hr);
	}
	co_return;
}

std::future<void> udns_resolver::refresh()
{
	try {
		auto os = co_await _socket.GetOutputStreamAsync(_multicast_group, udns_port_out);
		DataWriter writer(os);
		writer.WriteByte('$');
		co_await writer.StoreAsync();
	} catch(winrt::hresult_error& hr) {
		log_hresult(L"UDNS", hr);
	}
	co_return;
}

HostName udns_resolver::get_address(const std::wstring& name) const
{
	lock_t lock(_mutex);
	auto it = _names.find(name);
	return it == _names.end() ? HostName{ nullptr } : it->second;
}
