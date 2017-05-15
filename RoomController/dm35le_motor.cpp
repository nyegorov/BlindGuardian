#include "pch.h"
#include "dm35le_motor.h"

using namespace winrt::Windows::Devices::Gpio;
using namespace winrt::Windows::System::Threading;
using namespace std::chrono;

namespace roomctrl {

void delay(std::chrono::microseconds microSeconds)
{
	auto start = std::chrono::high_resolution_clock().now();
	std::chrono::microseconds time_us;
	do {
		auto end = std::chrono::high_resolution_clock().now();
		time_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	} while(time_us < microSeconds);
}

dm35le_motor::dm35le_motor(int32_t tx_pin)
{
	auto gpio = GpioController::GetDefault();
	if(gpio) {
		auto gpio_pin = gpio.OpenPin(tx_pin);
		gpio_pin.Write(GpioPinValue::Low);
		gpio_pin.SetDriveMode(GpioPinDriveMode::Output);
		_tx_pin = gpio_pin;
	}
}


dm35le_motor::~dm35le_motor()
{
}

void dm35le_motor::send_cmd(byte channel, command code, uint32_t repeat)
{
	auto task = ThreadPool::RunAsync([=](auto&& handler) {
		const auto period = 362us;
		byte data[5] = { 0x2e, 0x31, 0xc6, byte(0xc0 | channel), byte((code << 4) | code) };
		if(!_tx_pin)	return;

		for(unsigned i = 0; i < repeat; i++) {
			_tx_pin.Write(GpioPinValue::Low);
			Sleep(9);
			_tx_pin.Write(GpioPinValue::High);
			Sleep(5);
			_tx_pin.Write(GpioPinValue::Low);
			delay(1000us);
			for(auto& d : data) {
				for(int b = 7; b >= 0; b--) {
					_tx_pin.Write(GpioPinValue::Low);
					delay(period);
					_tx_pin.Write(GpioPinValue::High);
					delay(period);
					_tx_pin.Write((d >> b) & 1 ? GpioPinValue::High : GpioPinValue::Low);
					delay(period);
				}
			}
			_tx_pin.Write(GpioPinValue::Low);
			//Sleep(9);
		}
	}, WorkItemPriority::High, WorkItemOptions::None);
	std::this_thread::sleep_for(repeat * 60ms);
	task.get();
}

}