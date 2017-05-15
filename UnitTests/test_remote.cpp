#include "pch.h"
#include "CppUnitTest.h"

#include "../RoomController/udns_resolver.h"
#include "../RoomController/esp8266_motor.h"
#include "../RoomController/debug_stream.h"

using namespace std;
using namespace roomctrl;
using namespace std::experimental;
using namespace std::string_literals;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Web::Http;
using namespace winrt::Windows::Storage::Streams;

namespace UnitTests
{

udns_resolver udns;

TEST_CLASS(MotorController)
{
public:
	TEST_CLASS_INITIALIZE(Init)
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
	}

};

}