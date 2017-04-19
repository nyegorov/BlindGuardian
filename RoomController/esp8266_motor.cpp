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

#pragma pack(push, 1)
template <char C, size_t OSIZE, class O, size_t ISIZE, class I> struct cmd_base {
	char command = C;
	typedef typename O out_type;
	typedef typename I in_type;
	cmd_base() {}
	cmd_base(const O& params) : out(params) {}
	union {
		O out;
		std::array<uint8_t, OSIZE> out_buf;
	};
	union {
		I in;
		std::array<uint8_t, ISIZE> in_buf;
	};
	bool ok = false;
};

struct empty {};

struct status_out {
	uint8_t status;
	uint8_t temp;
	uint32_t light;
};
#pragma pack(pop)

using cmd_version = cmd_base<'v', 0, empty, 8, char[8]>;
using cmd_reset = cmd_base<'r', 0, empty, 0, empty>;
using cmd_status = cmd_base<'s', 0, empty, 6, status_out>;
using cmd_open = cmd_base<'o', 0, empty, 1, uint8_t>;
using cmd_close = cmd_base<'c', 0, empty, 1, uint8_t>;
using cmd_setpos = cmd_base<'p', 1, uint8_t, 1, uint8_t>;


esp8266_motor::esp8266_motor(std::wstring_view remote_host, udns_resolver& udns, log_manager& log) :
	_udns(udns), _host(remote_host), _log(log)
{
	_light.set(error_t::not_implemented);
	_temp.set(error_t::not_implemented);
}

esp8266_motor::~esp8266_motor()
{
}

void esp8266_motor::start()
{
	cmd_version ver;
	if(send_cmd(_udns.get_address(_host), ver)) {
		_log.info(module_name, L"%7hs connected.", (char*)ver.in);
	}
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

template<class CMD> bool esp8266_motor::send_cmd(HostName host, CMD& cmd)
{
	return send_cmd(host, cmd.command, cmd.out_buf, cmd.in_buf);
}

void esp8266_motor::open()  { cmd_open cmd;  if(send_cmd(_udns.get_address(_host), cmd)) _position.set(cmd.in); }
void esp8266_motor::close() { cmd_close cmd; if(send_cmd(_udns.get_address(_host), cmd)) _position.set(cmd.in); }
void esp8266_motor::setpos(value_t pos) 
{ 
	if(is_error(pos))	return;
	cmd_setpos cmd{ (uint8_t)as<value_type>(*pos) }; 
	if(send_cmd(_udns.get_address(_host), cmd))	_position.set(cmd.in); 
}
void esp8266_motor::reset() { send_cmd(_udns.get_address(_host), cmd_reset()); _udns.reset(); }

void esp8266_motor::update_sensors()
{
	watch w;
	cmd_status cmd;
	bool ok = send_cmd(_udns.get_address(_host), cmd);
	if(ok) {
		_light.set(cmd.in.light);
		_temp.set(cmd.in.temp);
		_position.set(cmd.in.status);
		_log.message(module_name, L"status: pos=%d, temp=%d, light=%d (%lld ms)", cmd.in.status, cmd.in.temp, cmd.in.light, w.elapsed_ms().count());
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