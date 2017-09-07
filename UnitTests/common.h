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

inline wstring to_str(std::thread::id id)
{
	std::wstringstream wss;
	wss << id;
	return wss.str();
}

inline std::wstring to_wstring(const winrt::hstring& hs)
{
	return { hs.begin(), hs.end() };
}

class DumbRemote
{
	value_type				_temp;
	value_type				_light;
public:
	DumbRemote(value_type temp, value_type light) : _temp(temp), _light(light) {}

	std::future<void> set_temp(value_type val) { _temp = val; return update(); }
	std::future<void> set_light(value_type val) { _light = val; return update(); }
	std::future<void> update()
	{
		try
		{
			DatagramSocket socket;
			socket.Control().MulticastOnly(true);
			auto os = co_await socket.GetOutputStreamAsync({ L"224.0.0.100" }, L"4760");
			DataWriter writer(os);

			writer.ByteOrder(ByteOrder::LittleEndian);
			writer.WriteByte('s');
			writer.WriteByte(5);
			writer.WriteByte((int8_t)_temp);
			writer.WriteUInt32(_light);
			co_await writer.StoreAsync();

		} catch(const std::exception& ex) {
			debug << ex.what();
		} catch(const winrt::hresult_error& hr)	{
			wdebug << wstring(hr.message());
		}
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
	void stop()  { };
	void setpos(value_t v)	{ _value = v; }
	DumbMotor() { }
	value_t value() { return _value; };
	int32_t pos()   { return std::get<int32_t>(_value); };
};

