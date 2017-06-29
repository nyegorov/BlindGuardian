#include "pch.h"
#include "CppUnitTest.h"

#include "../RoomController/udns_resolver.h"
#include "../RoomController/esp8266_motor.h"
#include "../RoomController/esp8266_sensors.h"
#include "../RoomController/debug_stream.h"

using namespace std;
using namespace std::chrono;
using namespace std::experimental;
using namespace std::string_literals;
using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Web::Http;
using namespace winrt::Windows::Storage::Streams;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace roomctrl;

namespace UnitTests
{

udns_resolver udns;

TEST_CLASS(NodeMCU)
{
public:
	TEST_METHOD(RemoteSensors)
	{
/*		sensor	temp{ L"temp_out" }, light{ L"light" };
		esp8266_sensors nodemcu(L"4760", L"224.0.0.100", temp, light);
		Assert::IsFalse(nodemcu.online());
		[&]() -> future<void> {
			co_await nodemcu.start();
			for(int i = 0; i < 60; i++) {
				if(nodemcu.online())	break;
				co_await 100ms;
			}
			co_return;
		}().get();
		Assert::IsTrue(nodemcu.online());
		if(nodemcu.online()) {
			auto t = get<int32_t>(temp.value());
			auto l = get<int32_t>(light.value());
			Assert::IsTrue(t >= 10 && t < 40);
			Assert::IsTrue(l >= 100 && l < 77000);
		}*/
	}

	/*TEST_CLASS_INITIALIZE(Init)
	{
		async([]() {
			udns.start();
			for(int i = 0; i < 100; i++) {
				if(udns.get_address(L"motctrl") != nullptr)	return;
				this_thread::sleep_for(10ms);
			}
			udns.get_address(L"motctrl");
		}).get();
	}

	TEST_METHOD(MicroDNS)
	{
		udns_resolver udns1;
		async([&udns1]() { udns1.start().get(); Sleep(500); }).get();
		Assert::IsTrue(udns1.get_address(L"motctrl") != nullptr);
	}

	TEST_METHOD(RemoteSensors)
	{
		Assert::IsTrue(udns.get_address(L"motctrl") != nullptr);
		esp8266_motor mot(L"motctrl", udns );
		value_t l, t;
		mot.get_temp()->update();
		mot.get_light()->update();
		auto temp = get<int32_t>(mot.get_temp()->value());
		auto light = get<int32_t>(mot.get_light()->value());
		Assert::IsTrue(temp >= 42 && temp < 50);
		Assert::IsTrue(light >= 76000 && temp < 77000);
	}

	TEST_METHOD(Esp8266Motor)
	{
		Assert::IsTrue(udns.get_address(L"motctrl") != nullptr);
		esp8266_motor mot(L"motctrl", udns );
		mot.set_timeout(1s, 5s);
		mot.start();
		Assert::AreEqual(L"ESP"s, mot.version().substr(0, 3));
		Assert::AreEqual(L"motctrl"s, mot.host_name().substr(0, 7));
		mot.open();
		Assert::AreEqual(100, as<int32_t>(mot.get_pos()->value()));
		mot.close();
		Assert::AreEqual(  0, as<int32_t>(mot.get_pos()->value()));
		Assert::IsTrue(mot.online());
	}*/

};

}