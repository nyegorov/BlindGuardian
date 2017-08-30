#include "pch.h"
#include "udns_resolver.h"
#include "common.h"
#include "debug_stream.h"

using namespace winrt::Windows::Networking;
using namespace winrt::Windows::Networking::Connectivity;
using namespace winrt::Windows::Networking::Sockets;
using namespace winrt::Windows::Networking::Connectivity;
using namespace winrt::Windows::Storage::Streams;

const wchar_t module_name[] = L"UDNS";
const wchar_t udns_port_in[] = L"4762";
const wchar_t udns_port_out[] = L"4761";

void udns_resolver::on_message(const DatagramSocket &, const DatagramSocketMessageReceivedEventArgs &args)
{
	auto reader = args.GetDataReader();
	const auto byteCount = reader.UnconsumedBufferLength();

	if(byteCount < 5)	return;
	const uint8_t cmd = reader.ReadByte();
	if(cmd != '#')		return;

	uint8_t ip_addr[4], name_len;
	reader.ReadBytes(ip_addr);
	name_len = reader.ReadByte();
	wstring name = reader.ReadString(name_len);
	std::wstringstream ws;
	ws << ip_addr[0] << '.' << ip_addr[1] << '.' << ip_addr[2] << '.' << ip_addr[3];
	wstring ip = ws.str();
	logger.message(module_name, L"# - announce: %s -> %s", name.c_str(), ip.c_str());

	// persistence
	if(_config) {
		_config->set(name.c_str(), ip.c_str());
		_config->save();
	}

	lock_t lock(_mutex);
	_names.emplace(name, ip);
}

udns_resolver::udns_resolver(config_manager* config) : _config(config)
{
	_names.emplace(L"localhost", HostName(L"localhost"));
}

udns_resolver::~udns_resolver()
{
}

task<void> udns_resolver::start()
{
	try {
		_socket = DatagramSocket();
		_socket.MessageReceived([this](auto& socket, auto& args) { on_message(socket, args); });
		_socket.Control().MulticastOnly(true);
		co_await _socket.BindServiceNameAsync(udns_port_in);
		_socket.JoinMulticastGroup(_multicast_group);
		co_await refresh();
	} catch(winrt::hresult_error& hr) {
		logger.error(module_name, hr);
	}
}

task<void> udns_resolver::post_cmd(uint8_t cmd)
{
	try {
		auto os = co_await _socket.GetOutputStreamAsync(_multicast_group, udns_port_out);
		DataWriter writer(os);
		writer.WriteByte(cmd);
		co_await writer.StoreAsync();
	} catch(winrt::hresult_error& hr) {
		logger.error(module_name, hr);
	}
}

task<void> udns_resolver::refresh()	{ return post_cmd('$'); }
task<void> udns_resolver::reset()	{ return post_cmd('r'); }

HostName udns_resolver::get_address(const std::wstring& name) const
{
	lock_t lock(_mutex);
	auto it = _names.find(name);
	if(it != _names.end()) return it->second;
	if(_config) {
		auto ip = _config->get(name.c_str(), L"");
		if(!ip.empty()) return HostName{ ip };
	}
	return HostName{ nullptr };
}
