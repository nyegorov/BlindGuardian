#include "pch.h"
#include "motor_ctrl.h"
#include "debug_stream.h"

using namespace winrt;
using namespace std::chrono_literals;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Networking;
using namespace winrt::Windows::Networking::Connectivity;
using namespace winrt::Windows::Networking::Sockets;
using namespace winrt::Windows::Storage::Streams;

const wchar_t cmd_port[] = L"4760";

namespace roomctrl {

motor_ctrl::motor_ctrl(std::wstring_view name, std::wstring_view remote_host, udns_resolver& udns) : actuator(name), _udns(udns), _host(remote_host)
{

}

motor_ctrl::~motor_ctrl()
{
}

union uint_buf {
	uint32_t u32;
	uint8_t u8[4];
};

std::future<bool> motor_ctrl::send_cmd(HostName host, uint8_t cmd, winrt::array_view<const uint8_t> inbuf, winrt::array_view<uint8_t> outbuf)
{
	try {
		if(!host)	co_return false;
		StreamSocket socket;
		auto conn = socket.ConnectAsync(host, cmd_port, SocketProtectionLevel::PlainSocket);
		auto start = std::chrono::high_resolution_clock::now();
		while(true) {
			auto status = conn.Status();
			if(status == AsyncStatus::Completed)	break;
			if(status == AsyncStatus::Error)		co_return false;
			if(std::chrono::high_resolution_clock::now() - start > 500ms) {
				OutputDebugStringW(L"MOTC: timeout\n");
				conn.Cancel();
				co_return false;
			}
			co_await 10ms;
		}
		DataWriter writer(socket.OutputStream());
		writer.WriteByte(cmd);
		writer.WriteBytes(inbuf);
		co_await writer.StoreAsync();

		DataReader reader(socket.InputStream());
		co_await reader.LoadAsync(outbuf.size());
		reader.ByteOrder(ByteOrder::LittleEndian);
		reader.ReadBytes(outbuf);
		socket.Close();
		co_return true;
	} catch(winrt::hresult_error& hr) {
		OutputDebugStringW(L"MOTC.send_cmd: ");
		OutputDebugStringW(wstring(hr.message()).c_str());
		OutputDebugStringW(L"\n");
	}
	co_return false;
}

std::future<value_t> motor_ctrl::get_sensor_async(uint8_t sensor)
{
	co_await winrt::resume_background();
	uint_buf buf;
	bool ok = co_await send_cmd(_udns.get_address(_host), sensor, {}, buf.u8);
	if(ok)	co_return buf.u32;

	co_await _udns.refresh();
	co_return error_t::not_implemented;
}

value_t motor_ctrl::get_sensor(uint8_t sensor)
{
	try {
		auto host = _udns.get_address(_host);
		StreamSocket socket;
		socket.ConnectAsync(host, cmd_port, SocketProtectionLevel::PlainSocket).get();
		DataWriter writer(socket.OutputStream());
		writer.WriteByte(sensor);
		writer.StoreAsync().get();

		DataReader reader(socket.InputStream());
		reader.LoadAsync(4).get();
		reader.ByteOrder(ByteOrder::LittleEndian);
		auto value = reader.ReadUInt32();
		return value;
	} catch(winrt::hresult_error& hr) {
		OutputDebugStringW(L"MOTC.get_sensor: ");
		OutputDebugStringW(wstring(hr.message()).c_str());
		OutputDebugStringW(L"\n");
	}
	_udns.refresh();
	return error_t::not_implemented;
}

std::future<void> motor_ctrl::do_action_async(uint8_t action)
{
	co_await send_cmd(_udns.get_address(_host), action, {}, {});
	co_return;
}

void motor_ctrl::send_command(uint8_t sensor)
{
	try {
		auto host = _udns.get_address(_host);
		StreamSocket socket;
		socket.ConnectAsync(host, cmd_port, SocketProtectionLevel::PlainSocket).get();
		DataWriter writer(socket.OutputStream());
		writer.WriteByte(sensor);
		writer.StoreAsync().get();
	} catch(winrt::hresult_error& hr) {
		OutputDebugStringW(L"MOTC.send_command: ");
		OutputDebugStringW(wstring(hr.message()).c_str());
		OutputDebugStringW(L"\n");
	}
	_udns.refresh();
}

void remote_sensor::update()
{
	set(_remote.get_sensor_async(_cmdbyte).get());
	//set(_remote.get_sensor(_cmdbyte));
}

}