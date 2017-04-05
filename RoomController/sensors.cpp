#include "pch.h"
#include "sensors.h"

using namespace winrt::Windows::Networking;
using namespace winrt::Windows::Networking::Sockets;
using namespace winrt::Windows::Storage::Streams;

const wchar_t const_remote_port[] = L"4760";

namespace roomctrl {

std::future<value_t> remote_sensor::value_async()
{
	try {
		co_await winrt::resume_background();
		StreamSocket socket;
		co_await socket.ConnectAsync(HostName(L"localhost"), const_remote_port, SocketProtectionLevel::PlainSocket);
		DataWriter writer(socket.OutputStream());
		auto cmd = _cmdbyte;
		writer.WriteByte(cmd);
		co_await writer.StoreAsync();

		DataReader reader(socket.InputStream());
		co_await reader.LoadAsync(sizeof(uint32_t));
		auto val = reader.ReadUInt32();
		co_return val;
	} catch(...) {
		co_return error_t::not_implemented;
	}
}

void remote_sensor::update()
{
	_value = value_async().get();
}

}

