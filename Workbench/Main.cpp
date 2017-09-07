#include "pch.h"

#include "debug_stream.h"
#include "udns_resolver.h"
#include "room_engine.h"
#include "log_manager.h"


using namespace winrt;
using namespace std;
using namespace std::experimental;
using namespace roomctrl;

wdebugstream wdebug;
debugstream debug;
log_manager logger;

using namespace winrt::Windows::Networking::Sockets;

std::future<void> test() 
{
	room_server srv(".");
	srv.config().set(L"motctrl", L"192.168.6.53");
	srv.config().set(L"enable_debug", true);
	srv.config().set(L"poll_interval",  200);
	srv.config().set(L"socket_timeout", 15000);
	srv.rules().save({ L"r1", L"temp_out>30", L"blind.open()", true }, false);
	co_await srv.start();
	for(;;) {
		co_await 1000ms;
		srv.run();
		cout << ".";
	}
}

int main()
{
	init_apartment();
	test().get();
}
