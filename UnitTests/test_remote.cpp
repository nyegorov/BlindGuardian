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
TEST_CLASS(MotorController)
{
public:

	TEST_METHOD(MicroDNS)
	{
		config_manager cm{ "config.db" };
		log_manager log;
		udns_resolver udns{ cm, log };
		thread([&udns]() { udns.start().get(); Sleep(1000); }).join();
		Assert::IsTrue(udns.get_address(L"motctrl") != nullptr);
		udns_resolver udns1{ cm, log };
		thread([&udns1]() { udns1.start().get(); Sleep(1000); }).join();
		Assert::IsTrue(udns1.get_address(L"motctrl") != nullptr);
		filesystem::remove("config.db" );
	}

	TEST_METHOD(RemoteSensors)
	{
		log_manager log;
		config_manager cm{ "config.db" };
		udns_resolver udns{ cm, log };
		thread([&udns]() { udns.start().get(); Sleep(500); }).join();
		Assert::IsTrue(udns.get_address(L"motctrl") != nullptr);
		esp8266_motor mot(L"motctrl", udns, log);
		value_t l, t;
		mot.get_temp()->update();
		auto temp = get<int32_t>(mot.get_temp()->value());
		auto light = get<int32_t>(mot.get_light()->value());
		Assert::IsTrue(temp >= 42 && temp < 50);
		Assert::IsTrue(light >= 76000 && temp < 77000);
		filesystem::remove("config.db");
	}

};
}