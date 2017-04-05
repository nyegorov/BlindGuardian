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
		udns_resolver udns;
		thread([&udns]() { udns.refresh().get(); Sleep(100); }).join();
		Assert::IsTrue(udns.get_address(L"motctrl") != nullptr);
		udns_resolver udns1;
		thread([&udns1]() { udns1.refresh().get(); Sleep(100); }).join();
		Assert::IsTrue(udns1.get_address(L"motctrl") != nullptr);
	}

	TEST_METHOD(RemoteSensors)
	{
		udns_resolver udns;
		thread([&udns]() { udns.refresh().get(); Sleep(100); }).join();
		Assert::IsTrue(udns.get_address(L"motctrl") != nullptr);
		motor_ctrl mot(L"motctrl", udns);
		value_t l, t;
		thread([&mot, &t, &l]() { t = mot.get_sensor('t').get(); l = mot.get_sensor('l').get(); }).join();
		Assert::AreEqual(42,   get<int32_t>(t));
		Assert::AreEqual(7600, get<int32_t>(l));
	}

};
}