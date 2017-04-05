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

const wchar_t udns_port[] = L"4761";

motor_ctrl::motor_ctrl()
{
}


motor_ctrl::~motor_ctrl()
{
};
/*
std::future<ip_address> motor_ctrl::udns_query(std::wstring_view name, const NetworkAdapter& adapter)
{
	HostName localhost{ nullptr }, broadcast{ L"255.255.255.255" };
	ip_address remote_ip{ 0 };
	for(auto host : NetworkInformation::GetHostNames()) {
		auto ip_info = host.IPInformation();
		if(ip_info) {
			auto host_adapter = host.IPInformation().NetworkAdapter();
			if(host_adapter && host_adapter.NetworkAdapterId() == adapter.NetworkAdapterId()) {
				localhost = host;
				break;
			}

		}
	}
	if(!localhost)	co_return remote_ip;

	DatagramSocket sender, receiver;
	receiver.MessageReceived([&remote_ip](auto&, auto& args) {
		auto reader = args.GetDataReader();
		auto byteCount = reader.UnconsumedBufferLength();
		wdebug << L"IP: " << std::wstring(args.RemoteAddress().DisplayName()) << L", Bytes: " << (unsigned)byteCount << std::endl;
		auto l = reader.ReadByte();
		wstring s = reader.ReadString(l);
		wdebug << L"ANSWER: " << s << std::endl;
		//remotehost = args.RemoteAddress();
	});
	co_await receiver.BindServiceNameAsync(udns_port, adapter);

	EndpointPair remote(localhost, L"", broadcast, udns_port);
	auto os = co_await sender.GetOutputStreamAsync(remote);

	DataWriter writer(os);
	writer.WriteByte((uint8_t)name.size());
	for(auto c : name) 	writer.WriteByte((uint8_t)c);
	co_await writer.StoreAsync();
	co_return remote_ip;
}
*/

std::future<ip_address> motor_ctrl::resolve(std::wstring_view name)
{
	udp.MessageReceived([](auto&, auto& args) {
		auto reader = args.GetDataReader();
		auto byteCount = reader.UnconsumedBufferLength();
		wdebug << L"IP: " << std::wstring(args.RemoteAddress().DisplayName()) << L", Bytes: " << (unsigned)byteCount << std::endl;
		//auto l = reader.ReadByte();
		//wstring s = reader.ReadString(l);
		//wdebug << L"ANSWER: " << s << std::endl;
	});

	auto profiles = NetworkInformation::GetConnectionProfiles();

	for(auto p : profiles) {
		//wdebug << wstring(p.ProfileName()) << std::endl;

	}

	auto hosts = NetworkInformation::GetHostNames();
	NetworkAdapter ad{ nullptr };
	for(auto h : hosts) {
		if(h.DisplayName() == L"192.168.6.49") ad = h.IPInformation().NetworkAdapter();
		//wdebug << wstring(h.DisplayName()) << std::endl;
	}

	//udp.Control().MulticastOnly(true);
	co_await udp.BindServiceNameAsync(L"4762", ad);
	//udp.JoinMulticastGroup(HostName(L"239.255.1.2"));

	DatagramSocket udp2;
	//auto os = co_await udp2.GetOutputStreamAsync(HostName(L"255.255.255.255"), L"4761");
	EndpointPair remote(HostName(L"192.168.6.49"), L"", HostName(L"255.255.255.255"), udns_port);
	auto os = co_await udp2.GetOutputStreamAsync(remote);

	//auto dsi = udp2.Information();
	//wstring s1 = dsi.LocalPort();
	//wstring s2 = dsi.RemotePort();
	//wdebug << wstring(dsi.LocalAddress().DisplayName()) << std::endl;
	//wdebug << wstring(dsi.LocalPort()) << std::endl;
	//wdebug << wstring(dsi.RemoteAddress().DisplayName()) << std::endl;
	//wdebug << wstring(dsi.RemotePort()) << std::endl;

	//udp2.mul
//	auto os = co_await udp2.GetOutputStreamAsync(HostName(L"255.255.255.255"), L"4761");
	DataWriter writer(os);
	writer.WriteByte('$');
	co_await writer.StoreAsync();

	ip_address ip;
	co_await 1s;
	co_return ip;
	//co_return ip_address{ 0 };
}
