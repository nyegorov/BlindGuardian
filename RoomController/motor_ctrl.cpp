#include "pch.h"
#include "motor_ctrl.h"

using namespace winrt;
using namespace std::chrono_literals;
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

std::future<value_t> motor_ctrl::get_sensor_async(uint8_t sensor)
{
	try {
		co_await winrt::resume_background();
		auto host = _udns.get_address(_host);
		StreamSocket socket;
		co_await socket.ConnectAsync(host, cmd_port, SocketProtectionLevel::PlainSocket);
		DataWriter writer(socket.OutputStream());
		writer.WriteByte(sensor);
		co_await writer.StoreAsync();

		DataReader reader(socket.InputStream());
		co_await reader.LoadAsync(4);
		reader.ByteOrder(ByteOrder::LittleEndian);
		auto value = reader.ReadUInt32();
		co_return value;
	} catch(winrt::hresult_error& hr) {
		OutputDebugStringW(L"MOTC.get_sensor_async: ");
		OutputDebugStringW(wstring(hr.message()).c_str());
		OutputDebugStringW(L"\n");
	}
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

std::future<void> motor_ctrl::send_command_async(uint8_t sensor)
{
	try {
		co_await winrt::resume_background();
		auto host = _udns.get_address(_host);
		StreamSocket socket;
		co_await socket.ConnectAsync(host, cmd_port, SocketProtectionLevel::PlainSocket);
		DataWriter writer(socket.OutputStream());
		writer.WriteByte(sensor);
		co_await writer.StoreAsync();

		DataReader reader(socket.InputStream());
		co_await reader.LoadAsync(4);
		reader.ByteOrder(ByteOrder::LittleEndian);
		auto value = reader.ReadUInt32();
		co_return;
	} catch(winrt::hresult_error& hr) {
		OutputDebugStringW(L"MOTC.send_command_async: ");
		OutputDebugStringW(wstring(hr.message()).c_str());
		OutputDebugStringW(L"\n");
	}
	co_await _udns.refresh();
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
	//set(_remote.get_sensor_async(_cmdbyte).get());
	set(_remote.get_sensor(_cmdbyte));
}

}