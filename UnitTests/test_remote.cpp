#include "pch.h"
#include "CppUnitTest.h"

#include "../RoomController/udns_resolver.h"
#include "../RoomController/motor_ctrl.h"
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
		thread([&udns]() { udns.start().get(); Sleep(500); }).join();
		Assert::IsTrue(udns.get_address(L"motctrl") != nullptr);
		udns_resolver udns1{ cm, log };
		thread([&udns1]() { udns1.start().get(); Sleep(500); }).join();
		Assert::IsTrue(udns1.get_address(L"motctrl") != nullptr);
		filesystem::remove("config.db" );
	}

	TEST_METHOD(RemoteSensors)
	{
		log_manager log;
		config_manager cm{ "config.db" };
		udns_resolver udns{ cm, log };
		thread([&udns]() { udns.start().get(); Sleep(100); }).join();
		Assert::IsTrue(udns.get_address(L"motctrl") != nullptr);
		motor_ctrl mot(L"blind", L"motctrl", udns, cm, log);
		value_t l, t;
		thread([&mot]() { mot.get_light()->update(); Sleep(500); }).join();
		Assert::AreEqual(42,   get<int32_t>(mot.get_temp()->value()));
		Assert::AreEqual(76000,get<int32_t>(mot.get_light()->value()));
		filesystem::remove("config.db");
	}

};
}