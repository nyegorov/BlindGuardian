#include "pch.h"

#include "debug_stream.h"
#include "motor_ctrl.h"
#include "udns_resolver.h"

using namespace winrt;
using namespace std;
using namespace roomctrl;

wdebugstream wdebug;
debugstream debug;

std::future<void> test() {
/*	motor_ctrl mot;
	auto ip = co_await mot.resolve(L"motctrl");
	co_await 5s;
	debug << "IP: " << ip.addr[0] << "." << ip.addr[1] << "." << ip.addr[2] << "." << ip.addr[3] << endl; // */
	udns_resolver udns;
	co_await udns.refresh();
	co_await 1s;
	motor_ctrl mot(L"motctrl", udns);
	auto t = co_await mot.get_sensor('t');
	auto l = co_await mot.get_sensor('l');
	if(is_error(t) || is_error(l)) {
		debug << "Error reading sensors." << std::endl;
	} else {
		debug << "Temp: " << get<int32_t>(t) << ", Light: " << get<int32_t>(l) << std::endl;
	}
	co_return;
}

int main()
{
	init_apartment();
	test().get();

}
