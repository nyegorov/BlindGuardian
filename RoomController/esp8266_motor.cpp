#include "pch.h"
#include "common.h"
#include "esp8266_motor.h"
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

esp8266_motor::esp8266_motor(std::wstring_view remote_host, udns_resolver& udns, log_manager& log) : 
	_udns(udns), _host(remote_host), _log(log)
{
	_light.set(error_t::not_implemented);
	_temp.set(error_t::not_implemented);
	uint8_t ver[8] = { 0 };
	if(send_cmd(_udns.get_address(_host), 'v', {}, ver)) {
		_log.info(module_name, L"%8hs connected.", (char*)ver);
	}
}

esp8266_motor::~esp8266_motor()
{
}

bool esp8266_motor::wait_timeout(IAsyncInfo action)
{
	auto start = std::chrono::high_resolution_clock::now();
	while(true) {
		auto status = action.Status();
		if(status == AsyncStatus::Completed)	break;
		if(status == AsyncStatus::Error)		throw winrt::hresult_error(action.ErrorCode());
		if(std::chrono::high_resolution_clock::now() - start > _timeout) {
			_log.error(module_name, L"tcp connect timeout");
			action.Cancel();
			return false;
		}
		std::this_thread::sleep_for(10ms);
	}
	return true;
}

bool esp8266_motor::connect(HostName host)
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

bool esp8266_motor::send_cmd(HostName host, uint8_t cmd, winrt::array_view<const uint8_t> inbuf, winrt::array_view<uint8_t> outbuf)
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

		_log.message(module_name, L"sent command '%c'", cmd);
		return true;
	} catch(winrt::hresult_error& hr) {
		_log.error(module_name, hr);
	}
	return false;
}

void esp8266_motor::update_sensors()
{
	cmd_status cmd;
	watch w;
	bool ok = send_cmd(_udns.get_address(_host), 's', {}, cmd.data);
	if(ok) {
		_light.set(cmd.light);
		_temp.set(cmd.temp);
		_position.set(cmd.status);
		_log.message(module_name, L"status: pos=%d, temp=%d, light=%d (%lld ms)", cmd.status, cmd.temp, cmd.light, w.elapsed_ms().count());
		_retries = 0;
	} else {
		_retries++;
		_socket = nullptr;
	}
}

void esp8266_motor::remote_sensor::update()
{
	if(_master)	_remote.update_sensors();
}

}