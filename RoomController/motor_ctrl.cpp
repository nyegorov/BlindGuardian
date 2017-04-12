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
}

motor_ctrl::~motor_ctrl()
{
}

bool motor_ctrl::wait_timeout(IAsyncInfo action)
{
	auto start = std::chrono::high_resolution_clock::now();
	auto timeout = std::chrono::milliseconds(_config.get(L"socket_timeout", 500));
	while(true) {
		auto status = action.Status();
		if(status == AsyncStatus::Completed)	break;
		if(status == AsyncStatus::Error)		throw winrt::hresult_error(action.ErrorCode());
		if(std::chrono::high_resolution_clock::now() - start > timeout) {
			_log.error(module_name, L"tcp connect timeout");
			action.Cancel();
			return false;
		}
		std::this_thread::sleep_for(10ms);
	}
	return true;
}

bool motor_ctrl::connect(HostName host)
{
	_log.message(module_name, L"connection attempt");
	_socket = StreamSocket();
	//_socket.Control().KeepAlive(true);
	if(!wait_timeout(_socket.ConnectAsync(host, cmd_port, SocketProtectionLevel::PlainSocket))) return false;
	if(!_socket) {
		_log.error(module_name, L"connection failed.");
		return false;
	}
	_log.message(module_name, L"connection established with host %s", host.DisplayName().c_str());
	return true;
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

bool motor_ctrl::send_cmd(HostName host, uint8_t cmd, winrt::array_view<const uint8_t> inbuf, winrt::array_view<uint8_t> outbuf)
{
	try {
		if(!host)						return false;
		if(!_socket && !connect(host))	return false;
			
		DataWriter writer(_socket.OutputStream());
		writer.WriteByte(cmd);
		if(!inbuf.empty())	writer.WriteBytes(inbuf);
		if(!wait_timeout(writer.StoreAsync()))	return false;
		writer.DetachStream();

		if(!outbuf.empty()) {
			DataReader reader(_socket.InputStream());
			if(!wait_timeout(reader.LoadAsync(outbuf.size())))	return false;
			reader.ByteOrder(ByteOrder::LittleEndian);
			reader.ReadBytes(outbuf);
			reader.DetachStream();
		}
		return true;
	} catch(winrt::hresult_error& hr) {
		_log.error(module_name, hr);
	}
	return false;
}

void motor_ctrl::update_sensors()
{
	cmd_status cmd;
	watch w;
	bool ok = send_cmd(_udns.get_address(_host), 's', {}, cmd.data);
	if(ok) {
		_light.set(cmd.light);
		_temp.set(cmd.temp);
		_log.message(module_name, L"status: temp=%d, light=%d (%lld ms)", cmd.temp, cmd.light, w.elapsed_ms().count());
		_retries = 0;
	} else {
		_retries++;
		_socket = nullptr;
	}
}

void motor_ctrl::do_action(uint8_t action)
{
	send_cmd(_udns.get_address(_host), action, {}, {});
}

void remote_sensor::update()
{
	if(_master)	_remote.update_sensors();
}

}