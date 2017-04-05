#include "pch.h"

#include "debug_stream.h"
#include "motor_ctrl.h"
#include "udns_resolver.h"

using namespace winrt;
using namespace std;

wdebugstream wdebug;
debugstream debug;

std::future<void> test() {
/*	motor_ctrl mot;
	auto ip = co_await mot.resolve(L"motctrl");
	co_await 5s;
	debug << "IP: " << ip.addr[0] << "." << ip.addr[1] << "." << ip.addr[2] << "." << ip.addr[3] << endl; // */
	udns_resolver udns;
	co_await udns.refresh();
	co_await 5s;
	auto host = udns.get_address(L"motctrl");
	if(host)
		wdebug << wstring(host.DisplayName()) << std::endl; // */
	co_return; 
}

int main()
{
	init_apartment();
	test().get();

}
