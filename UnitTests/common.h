#pragma once

#include "../RoomController/value.h"
#include "../RoomController/sensors.h"
#include "../RoomController/debug_stream.h"

using namespace roomctrl;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Networking::Sockets;

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {
template<> inline std::wstring ToString<value_t>(const value_t& v) {
	if(auto pv = std::get_if<value_type>(&v))	return std::to_wstring(*pv);
	if(auto pe = std::get_if<error_t>(&v))		return L"error: "s + std::to_wstring((int)*pe);
	return L"unknown type!";
}
}
}
}

class DumbRemote
{
	uint8_t					_cmd_byte;
	value_type				_temp;
	value_type				_light;
	StreamSocketListener	_listener;
public:
	DumbRemote(value_type temp, value_type light) : _temp(temp), _light(light) { listen(); }

	void set_temp(value_type val) { _temp = val; }
	void set_light(value_type val) { _light = val; }
	std::future<void> listen() {
		_listener.ConnectionReceived([this](auto&& listener, auto&& args) { on_connect(listener, args); });
		co_await _listener.BindServiceNameAsync(L"4760");
		co_return;
	}

	std::future<void> on_connect(StreamSocketListener listener, StreamSocketListenerConnectionReceivedEventArgs args)
	{
		co_await winrt::resume_background();
		DataReader reader(args.Socket().InputStream());
		co_await reader.LoadAsync(sizeof(uint8_t));
		auto cmd = reader.ReadByte();
		if(cmd == 's') {
			DataWriter writer(args.Socket().OutputStream());
			writer.ByteOrder(ByteOrder::LittleEndian);
			writer.WriteByte(0);
			writer.WriteByte((int8_t)_temp);
			writer.WriteUInt32(_light);
			co_await writer.StoreAsync();
			writer.DetachStream();
		}
		else if(cmd == 'v') {
			DataWriter writer(args.Socket().OutputStream());
			writer.WriteString(L"DUMB1.1");
			co_await writer.StoreAsync();
			writer.DetachStream();
		}
		wdebug << L"received command: '" << (char)cmd << '\'' << std::endl;
		reader.DetachStream();
		co_await on_connect(listener, args);
	}
};

class DumbSensor : public sensor
{
public:
	DumbSensor(const wchar_t *name, value_type val) : sensor(name, val) { }
	void update() {}
};

class DumbMotor : public i_motor
{
public:
	value_t	_value = { 0 };
	void start() { }
	void open()  { _value = 100; }
	void close() { _value = 0; };
	void setpos(value_t v)	{ _value = v; }
	DumbMotor() { }
	value_t value() { return _value; };
	int32_t pos()   { return std::get<int32_t>(_value); };
};

