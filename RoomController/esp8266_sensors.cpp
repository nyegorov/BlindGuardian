#include "pch.h"
#include "esp8266_sensors.h"

const wchar_t module_name[] = L"EXTS";

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Networking::Sockets;
using namespace winrt::Windows::Storage::Streams;

namespace roomctrl {

esp8266_sensors::esp8266_sensors(std::wstring_view udp_port, std::wstring_view multicast_group, sensor& temp, sensor& light) : 
	_udp_port(udp_port), _multicast_group(wstring(multicast_group)), _temp(temp), _light(light)
{
}

esp8266_sensors::~esp8266_sensors()
{
}

task<void> esp8266_sensors::start()
{
	try {
		_socket = DatagramSocket();
		_socket.MessageReceived([this](auto& socket, auto& args) { on_message(socket, args); });
		_socket.Control().MulticastOnly(true);
		co_await _socket.BindServiceNameAsync(_udp_port);
		_socket.JoinMulticastGroup(_multicast_group);
	} catch(winrt::hresult_error& hr) {
		logger.error(module_name, hr);
	}
}

void esp8266_sensors::on_message(const DatagramSocket &, const DatagramSocketMessageReceivedEventArgs &args)
{
	try {
		auto reader = args.GetDataReader();
		const auto byteCount = reader.UnconsumedBufferLength();

		if(byteCount < 7)				throw winrt::hresult_out_of_bounds();

		const auto cmd  = reader.ReadByte();
		const auto size = reader.ReadByte();
		if(cmd != 's' || size != 5)		throw winrt::hresult_invalid_argument();

		reader.ByteOrder(ByteOrder::LittleEndian);
		auto t_out = reader.ReadByte();
		auto light = reader.ReadInt32();

		_temp.set(t_out);
		_light.set(light);
		_remote_ip = args.RemoteAddress();
		_last_status_time = steady_clock::now();

		logger.message(module_name, L"> temp = %d°C, light = %d lux", (int)t_out, (int)light);
	} catch(const winrt::hresult_error& hr) {
		logger.error(module_name, hr);
	}
}

}