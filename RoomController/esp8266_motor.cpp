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

using cmd_version	= cmd_base<'v', 0, empty, 8, char[8]>;
using cmd_reset		= cmd_base<'r', 0, empty, 0, empty>;
using cmd_status	= cmd_base<'s', 0, empty, 6, status_out>;
using cmd_open		= cmd_base<'o', 0, empty, 6, status_out>;
using cmd_close		= cmd_base<'c', 0, empty, 6, status_out>;
using cmd_setpos	= cmd_base<'p', 1, uint8_t, 6, status_out>;


esp8266_motor::esp8266_motor(std::wstring_view remote_host, udns_resolver& udns) : _udns(udns), _host(remote_host)
{
	_light.set(error_t::not_implemented);
	_temp.set(error_t::not_implemented);
}

esp8266_motor::~esp8266_motor()
{
}

void esp8266_motor::start()
{
}

bool esp8266_motor::wait_timeout(IAsyncInfo action, milliseconds timeout)
{
	auto start = std::chrono::high_resolution_clock::now();
	while(true) {
		auto status = action.Status();
		if(status == AsyncStatus::Completed)	break;
		if(status == AsyncStatus::Error)		throw winrt::hresult_error(action.ErrorCode());
		if(std::chrono::high_resolution_clock::now() - start > timeout) {
			logger.error(module_name, L"tcp connect timeout");
			action.Cancel();
			return false;
		}
		std::this_thread::sleep_for(10ms);
	}
	return true;
}

bool esp8266_motor::connect(HostName host, milliseconds timeout)
{
	logger.message(module_name, L"connection attempt");
	_socket = StreamSocket();
	//_socket.Control().KeepAlive(true);
	if(!wait_timeout(_socket.ConnectAsync(host, cmd_port, SocketProtectionLevel::PlainSocket), timeout)) return false;
	if(!_socket) {
		logger.error(module_name, L"connection failed.");
		return false;
	}
	if(_version == L"Unknown")	query_version();
	logger.message(module_name, L"connected to %s (%s)", host.DisplayName().c_str(), _version.c_str());
	return true;
}

bool esp8266_motor::send_cmd(HostName host, uint8_t cmd, winrt::array_view<const uint8_t> inbuf, winrt::array_view<uint8_t> outbuf, milliseconds timeout)
{
	try {
		watch w;
		if(!host)											return false;
		if(!_socket && !connect(host, timeout))				return false;

		DataWriter writer(_socket.OutputStream());
		writer.WriteByte(cmd);
		if(!inbuf.empty())	writer.WriteBytes(inbuf);
		if(!wait_timeout(writer.StoreAsync(), timeout))		return false;
		writer.DetachStream();

		if(!outbuf.empty()) {
			DataReader reader(_socket.InputStream());
			if(!wait_timeout(reader.LoadAsync(outbuf.size()), timeout))	return false;
			reader.ByteOrder(ByteOrder::LittleEndian);
			reader.ReadBytes(outbuf);
			reader.DetachStream();
		}
		if(cmd == 's' || cmd == 'o' || cmd == 'c' || cmd == 'p') {
			status_out *ps = reinterpret_cast<status_out*>(outbuf.data());
			logger.message(module_name, L"%c: pos=%d, t=%d, light=%d (%lld ms)", cmd, (int)ps->status, (int)ps->temp, ps->light, w.elapsed_ms().count());
		} else if(cmd == 'v') {
			char *pv = reinterpret_cast<char*>(outbuf.data());
			logger.message(module_name, L"%c: version=%hs (%lld ms)", cmd, pv, w.elapsed_ms().count());
		} else
			logger.message(module_name, L"%c.", cmd);

		return true;
	} catch(winrt::hresult_error& hr) {
		logger.error(module_name, hr);
	}
	return false;
}

template<class CMD> bool esp8266_motor::send_cmd(HostName host, CMD& cmd, milliseconds timeout)
{
	return send_cmd(host, cmd.command, cmd.out_buf, cmd.in_buf, timeout);
}

template<class CMD> void esp8266_motor::do_action(typename CMD::out_type param = {})
{
//	std::async(std::launch::async, [=](){
		CMD action(param);
		auto host = _udns.get_address(_host);
		for(unsigned i = 0; i < 5; i++) {
			if(send_cmd(host, action, _timeout_actions)) {
				_position.set(action.in.status);
				_retries = 0;
				break;
			}
			_retries++;
			_socket = nullptr;
		}
//	});
}

void esp8266_motor::open()	{ do_action<cmd_open>({}); }
void esp8266_motor::close() { do_action<cmd_close>({}); }
void esp8266_motor::reset() { _udns.reset(); }

void esp8266_motor::update_sensors()
{
	cmd_status cmd;
	bool ok = send_cmd(_udns.get_address(_host), cmd, _timeout_sensors);
	if(ok) {
		_light.set(cmd.in.light);
		_temp.set(cmd.in.temp);
		_position.set(cmd.in.status);
		_retries = 0;
	} else {
		_retries++;
		_socket = nullptr;
		if(_retries > MAX_RETRIES_BEFORE_RESET) {
			reset();
			_retries = 5;
			logger.info(module_name, L"too much retries, reset controller");
		}
	}
}

void esp8266_motor::query_version() {
	cmd_version ver;
	if(send_cmd(_udns.get_address(_host), ver, _timeout_sensors)) {
		wchar_t tmp[20];
		swprintf(tmp, _countof(tmp), L"%hs", ver.in);
		_version = tmp;
	}
}

void esp8266_motor::remote_sensor::update()
{
	if(_master)	_remote.update_sensors();
}

}