#include "pch.h"
#include "dm35le_motor.h"
#include "debug_stream.h"

const wchar_t module_name[] = L"DM35";

using namespace winrt;
using namespace Windows::Foundation;
using namespace winrt::Windows::Devices::Gpio;
using namespace winrt::Windows::System::Threading;
using namespace std::chrono;

namespace roomctrl {

void delay(microseconds microSeconds)
{
	auto start = std::chrono::high_resolution_clock().now();
	while(high_resolution_clock().now() - start < microSeconds);
}

bool wait_sync(GpioPin& pin, milliseconds timeout)
{
	auto start = high_resolution_clock::now();
restart:
	// wait until signal goes LOW
	while(pin.Read() == GpioPinValue::High)
		if(high_resolution_clock::now() - start > timeout)	return false;

	// it should stay LOW for 8.4ms...
	do {
		if(pin.Read() != GpioPinValue::Low)	goto restart;
	} while(high_resolution_clock().now() - start < 8ms);

	// ...then it should go HIGH...
	while(pin.Read() == GpioPinValue::Low)
		if(high_resolution_clock::now() - start > timeout)	return false;


	// ...and remain HIGH for 4.4ms
	start = high_resolution_clock().now();
	do {
		if(pin.Read() != GpioPinValue::High)	goto restart;
	} while(high_resolution_clock().now() - start < 4ms);

	// after signal goes LOW, the command packet begins
	while(pin.Read() == GpioPinValue::High)
		if(high_resolution_clock::now() - start > timeout)	return false;

	return true;
}

dm35le_motor::dm35le_motor(int32_t rx_pin, int32_t tx_pin, sensor& position, config_manager& config) : _config(config), _position(position)
{
	auto gpio = GpioController::GetDefault();
	if(gpio) {
		auto gpio_pin = gpio.OpenPin(tx_pin);
		gpio_pin.Write(GpioPinValue::Low);
		gpio_pin.SetDriveMode(GpioPinDriveMode::Output);
		_tx_pin = gpio_pin;
		gpio_pin = gpio.OpenPin(rx_pin);
		gpio_pin.SetDriveMode(GpioPinDriveMode::Input);
		_rx_pin = gpio_pin;
	}
	_position.set(100);
	_remote_id = _config.get(L"remote_id", 0x2e31c6c0);
}


dm35le_motor::~dm35le_motor()
{
}

void dm35le_motor::send_cmd(command code, uint32_t repeat)
{
	const auto period = 362us;
	if(!_tx_pin)	return;

	_tx_pin.Write(GpioPinValue::Low);
	auto default_priority = GetThreadPriority(GetCurrentThread());
	auto cmd = (unsigned long long)_remote_id << 8 | (code << 4) | code;
	std::bitset<40> data{ cmd };

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	for(unsigned n = 0; n < repeat; n++) {
		_tx_pin.Write(GpioPinValue::Low);
		Sleep(9);
		_tx_pin.Write(GpioPinValue::High);
		Sleep(5);
		_tx_pin.Write(GpioPinValue::Low);
		delay(1000us);
		for(unsigned i = 0; i < data.size(); i++)	{
			_tx_pin.Write(GpioPinValue::Low);
			delay(period);
			_tx_pin.Write(GpioPinValue::High);
			delay(period);
			_tx_pin.Write(data[40 - i - 1] ? GpioPinValue::High : GpioPinValue::Low);
			delay(period);
		}
		_tx_pin.Write(GpioPinValue::Low);
	}
	SetThreadPriority(GetCurrentThread(), default_priority);

	logger.info(module_name, L"send command %05llx (%s)", cmd, code == cmdUp ? L"Up" : code == cmdDown ? L"Down" : L"Stop");
}

unsigned long long dm35le_motor::read_cmd(milliseconds timeout)
{
	auto reader = GpioChangeReader(_rx_pin);
	reader.Polarity(GpioChangePolarity::Both);

	if(!wait_sync(_rx_pin, timeout))	return 0;

	reader.Start();
	auto task = reader.WaitForItemsAsync(80);
	auto start = high_resolution_clock::now();
	while(task.Status() != winrt::Windows::Foundation::AsyncStatus::Completed)
		if(high_resolution_clock::now() - start > timeout) return 0;
	reader.Stop();

	std::bitset<80> bits;

	for(int i = 0; i < 40; ++i)
	{
		auto rise = reader.GetNextItem();
		auto fall = reader.GetNextItem();
		if(rise.Edge != GpioPinEdge::RisingEdge || fall.Edge != GpioPinEdge::FallingEdge)	return 0;
		auto pulse = fall.RelativeTime - rise.RelativeTime;
		if(pulse < 300us || pulse > 800us) return 0;
		if(pulse > 540us) bits[40 - i - 1] = true;
	}

	logger.message(module_name, L"received command %05llx", bits.to_ullong());
	return bits.to_ullong();
}

bool dm35le_motor::pair_remote(milliseconds timeout, std::function<void(int)> progress)
{
	auto start = high_resolution_clock().now();
	std::vector<unsigned long long>	received_commands;
	while(received_commands.size() < 11) {
		if(high_resolution_clock().now() - start > timeout)	return false;
		auto cmd = read_cmd(5s) & 0xFFFFFFFF00;
		if(cmd) {
			received_commands.push_back(cmd);
			progress(received_commands.size());
		}
	}
	std::sort(begin(received_commands), end(received_commands));
	auto cmd = received_commands[5];
	logger.info(module_name, L"remote id = %05llx", cmd);
	_config.set(L"remote_id", _remote_id = int(cmd >> 8));
	_config.save();
	return true;
}

}