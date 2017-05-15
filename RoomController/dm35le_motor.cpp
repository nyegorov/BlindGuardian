#include "pch.h"
#include <bitset>
#include "dm35le_motor.h"
#include "debug_stream.h"

using namespace winrt::Windows::Devices::Gpio;
using namespace winrt::Windows::System::Threading;
using namespace std::chrono;

namespace roomctrl {

void delay(microseconds microSeconds)
{
	auto start = std::chrono::high_resolution_clock().now();
	microseconds time_us;
	do {
		auto end = high_resolution_clock().now();
		time_us = duration_cast<microseconds>(end - start);
	} while(time_us < microSeconds);
}

void wait_sync(GpioPin& pin)
{
restart:
	while(pin.Read() == GpioPinValue::High);

	auto start = high_resolution_clock().now();
	do {
		if(pin.Read() != GpioPinValue::Low)	goto restart;
	} while(high_resolution_clock().now() - start < 8ms);

	while(pin.Read() == GpioPinValue::Low);

	start = high_resolution_clock().now();
	do {
		if(pin.Read() != GpioPinValue::High)	goto restart;
	} while(high_resolution_clock().now() - start < 4ms);

	while(pin.Read() == GpioPinValue::High);
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
		}
	}, WorkItemPriority::High, WorkItemOptions::None);
	std::this_thread::sleep_for(repeat * 60ms);
	task.get();
}

unsigned long long dm35le_motor::read_cmd(int32_t rx_pin)
{
	auto gpio = GpioController::GetDefault();
	auto pin = gpio.OpenPin(rx_pin);
	auto reader = GpioChangeReader(pin);

	pin.SetDriveMode(GpioPinDriveMode::Input);
	reader.Polarity(GpioChangePolarity::Both);

	wait_sync(pin);

	reader.Start();
	reader.WaitForItemsAsync(80).get();
	reader.Stop();

	std::bitset<80> bits;

	for(int i = 0; i < 40; ++i)
	{
		auto first = reader.GetNextItem();
		auto second = reader.GetNextItem();
		if(first.Edge != GpioPinEdge::RisingEdge || second.Edge != GpioPinEdge::FallingEdge)	return 0;
		auto pulse = second.RelativeTime - first.RelativeTime;
		//debug << "bit " << i << ": " << duration_cast<microseconds>(second.RelativeTime - first.RelativeTime).count() << std::endl;
		if(pulse < 300us || pulse > 800us) return 0;
		if(pulse > 540us) bits[40 - i - 1] = true;
	}

	return bits.to_ullong();
}

}