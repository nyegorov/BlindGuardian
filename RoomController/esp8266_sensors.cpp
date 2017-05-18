#include "pch.h"
#include "esp8266_sensors.h"

const wchar_t module_name[] = L"EXTS";

namespace roomctrl {

esp8266_sensors::esp8266_sensors(std::wstring_view udp_port, std::wstring_view multicast_group, sensor& temp, sensor& light) : 
	_udp_port(udp_port), _multicast_group(wstring(multicast_group)), _temp(temp), _light(light)
{

}

esp8266_sensors::~esp8266_sensors()
{
}

std::future<void> esp8266_sensors::start()
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
	co_return;
}

void esp8266_sensors::on_message(const DatagramSocket &, const DatagramSocketMessageReceivedEventArgs &args)
{
	auto reader = args.GetDataReader();
	auto byteCount = reader.UnconsumedBufferLength();

	if(byteCount < 7)	return;
	uint8_t cmd  = reader.ReadByte();
	uint8_t size = reader.ReadByte();
	if(cmd != 's' || size != 5)		return;

	auto temp  = reader.ReadByte();
	auto light = reader.ReadInt32();

	_temp.set(temp);
	_light.set(light);
	logger.message(module_name, L"> temp = %d°C, light = %d lux", (int)temp, (int)light);
}

}