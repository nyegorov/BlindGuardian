#pragma once

#include "../RoomController/value.h"
#include "../RoomController/sensors.h"

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
	value_type				_value;
	StreamSocketListener	_listener;
public:
	DumbRemote(uint8_t cmd, value_type val) : _value(val), _cmd_byte(cmd) { listen(); }

	void set(value_type val) { _value = val; }
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
		if(cmd == _cmd_byte) {
			DataWriter writer(args.Socket().OutputStream());
			writer.ByteOrder(ByteOrder::LittleEndian);
			writer.WriteUInt32(cmd == _cmd_byte ? _value : 0);
			co_await writer.StoreAsync();
		}
	}
};

class DumbSensor : public sensor
{
public:
	DumbSensor(const wchar_t *name, value_type val) : sensor(name, val) { }
	void update() {}
};

class DumbMotor : public actuator
{
public:
	value_t	_value = { 0 };
	action open{ L"open", [this](auto&) { _value = 100; } };
	action close{ L"close", [this](auto&) { _value = 0; } };
	action setpos{ L"set_pos", [this](auto& v) { _value = v.empty() ? 0 : v.front(); } };
	DumbMotor(const wchar_t *name) : actuator(name) { }
	value_t value() { return _value; };
	std::vector<const i_action*> actions() const { return{ &open, &close, &setpos }; }
};

