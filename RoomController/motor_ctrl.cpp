#include "pch.h"
#include "common.h"
#include "motor_ctrl.h"
#include "debug_stream.h"

using namespace winrt;
using namespace std::chrono_literals;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Networking;
using namespace winrt::Windows::Networking::Connectivity;
using namespace winrt::Windows::Networking::Sockets;
using namespace winrt::Windows::Storage::Streams;

const wchar_t module_name[] = L"MOTC";
const wchar_t cmd_port[] = L"4760";

namespace roomctrl {

motor_ctrl::motor_ctrl(std::wstring_view name, std::wstring_view remote_host, udns_resolver& udns, config_manager& config, log_manager& log) : 
	actuator(name), _udns(udns), _host(remote_host), _config(config), _log(log)
{
	_light.set(error_t::not_implemented);
	_temp.set(error_t::not_implemented);
	_timeout = std::chrono::milliseconds(_config.get(L"poll_interval", 500));
}

motor_ctrl::~motor_ctrl()
{
}

#pragma pack(push, 1)
union cmd_status {
	struct {
		uint8_t status;
		uint8_t temp;
		uint32_t light;
	};
	uint8_t data[6];
};
#pragma pack(pop)
/*
bool motor_ctrl::send_cmd(HostName host, uint8_t cmd, winrt::array_view<const uint8_t> inbuf, winrt::array_view<uint8_t> outbuf)
{
	try {
		if(!host)	return false;
		StreamSocket socket;
		auto conn_action = socket.ConnectAsync(host, cmd_port, SocketProtectionLevel::PlainSocket);
		auto start = std::chrono::high_resolution_clock::now();
		while(true) {
			auto status = conn_action.Status();
			if(status == AsyncStatus::Completed)	break;
			if(status == AsyncStatus::Error)		return false;
			if(std::chrono::high_resolution_clock::now() - start > 500ms) {
				log_hresult(L"MOTC", winrt::hresult_canceled());
				conn_action.Cancel();
				return false;
			}
			Sleep(10);
		}
		DataWriter writer(socket.OutputStream());
		writer.WriteByte(cmd);
		if(inbuf.size())	writer.WriteBytes(inbuf);
		writer.StoreAsync().get();

		if(outbuf.size() > 0)	{
			DataReader reader(socket.InputStream());
			reader.LoadAsync(outbuf.size()).get();
			reader.ByteOrder(ByteOrder::LittleEndian);
			reader.ReadBytes(outbuf);
		}
		//socket.Close();
		return true;
	} catch(winrt::hresult_error& hr) {
		log_hresult(L"MOTC", hr);
	}
	return false;
}*/

std::future<bool> motor_ctrl::send_cmd(HostName host, uint8_t cmd, winrt::array_view<const uint8_t> inbuf, winrt::array_view<uint8_t> outbuf)
{
	try {
		if(!host)	co_return false;
		StreamSocket socket;
		auto conn_action = socket.ConnectAsync(host, cmd_port, SocketProtectionLevel::PlainSocket);
		auto start = std::chrono::high_resolution_clock::now();
		while(true) {
			auto status = conn_action.Status();
			if(status == AsyncStatus::Completed)	break;
			if(status == AsyncStatus::Error)		throw winrt::hresult_error(conn_action.ErrorCode());
			if(std::chrono::high_resolution_clock::now() - start > 500ms) {
				_log.error(module_name, L"tcp connect timeout");
				conn_action.Cancel();
				co_return false;
			}
			co_await 10ms;
		}
		DataWriter writer(socket.OutputStream());
		writer.WriteByte(cmd);
		if(!inbuf.empty())	writer.WriteBytes(inbuf);
		co_await writer.StoreAsync();

		if(!outbuf.empty()) {
			DataReader reader(socket.InputStream());
			co_await reader.LoadAsync(outbuf.size());
			reader.ByteOrder(ByteOrder::LittleEndian);
			reader.ReadBytes(outbuf);
		}
		//socket.Close();
		co_return true;
	} catch(winrt::hresult_error& hr) {
		_log.error(module_name, hr);
	}
	co_return false;
}

std::future<void> motor_ctrl::update_sensors()
{
	if(_inprogress)	co_return;
	_inprogress = true;
	co_await winrt::resume_background();
	cmd_status cmd;
	bool ok = co_await send_cmd(_udns.get_address(_host), 's', {}, cmd.data);
	if(ok) {
		_light.set(cmd.light);
		_temp.set(cmd.temp);
		_log.message(module_name, L"status: temp=%d, light=%d", cmd.temp, cmd.light);
		_retries = 0;
	} else {
		_retries++;
	}
	_inprogress = false;
}

std::future<void> motor_ctrl::do_action(uint8_t action)
{
	co_await send_cmd(_udns.get_address(_host), action, {}, {});
	co_return;
}

void remote_sensor::update()
{
	_remote.update_sensors();
}

}