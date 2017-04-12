#include "pch.h"

#include "debug_stream.h"
#include "motor_ctrl.h"
#include "udns_resolver.h"
#include "room_engine.h"
#include "log_manager.h"
#include "winrt/ppl.h"


using namespace winrt;
using namespace winrt::ppl;
using namespace std;
using namespace std::experimental;
using namespace roomctrl;

wdebugstream wdebug;
debugstream debug;

using namespace winrt::Windows::Networking::Sockets;

std::future<void> test() 
{
	room_server srv(".");
	srv.config().set(L"motctrl", L"192.168.6.53");
	srv.config().set(L"enable_debug", true);
	co_await srv.start();

	for(;;) {
		co_await 1s;
		srv.run();
		cout << ".";
	}
}

int main()
{
	init_apartment();
	test().get();
}
