#include "pch.h"
#include "motor_ctrl.h"

#include "debug_stream.h"
extern wdebugstream wdebug;

using std::wstring;
using namespace winrt;
using namespace std::chrono_literals;
using namespace winrt::Windows::Networking;
using namespace winrt::Windows::Networking::Connectivity;
using namespace winrt::Windows::Networking::Sockets;
using namespace winrt::Windows::Storage::Streams;

const wchar_t cmd_port[] = L"4760";

namespace roomctrl {

motor_ctrl::motor_ctrl(std::wstring_view name, udns_resolver& udns) : _name(name), _udns(udns)
{

}

motor_ctrl::~motor_ctrl()
{
}

std::future<value_t> motor_ctrl::get_sensor(uint8_t sensor)
{
	try {
		auto host = _udns.get_address(_name);
		StreamSocket socket;
		co_await socket.ConnectAsync(host, cmd_port, SocketProtectionLevel::PlainSocket);
		DataWriter writer(socket.OutputStream());
		writer.WriteByte(sensor);
		co_await writer.StoreAsync();
		DataReader reader(socket.InputStream());
		auto count = co_await reader.LoadAsync(4);
		if(count == 4) {
			uint8_t value[4];
			reader.ReadBytes(value);
			co_return *(uint32_t*)value;
		}
	} catch(const winrt::hresult_error& hr) {
		OutputDebugStringW(wstring(hr.message()).c_str());
	}
	co_await _udns.refresh();
	co_return error_t::not_implemented;
}

}