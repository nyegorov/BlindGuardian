#include "pch.h"
#include "sensors.h"
#include "log_manager.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::System::Threading;
using namespace winrt::Windows::Devices::I2c;
using namespace winrt::Windows::Devices::Gpio;
using namespace winrt::Windows::Devices::Enumeration;
using namespace std::chrono;

namespace roomctrl {

const wchar_t module_name[] = L"SENS";

GpioPin init_pin(int32_t pin, GpioPinDriveMode mode, GpioPinValue init = GpioPinValue::Low, GpioSharingMode share = GpioSharingMode::Exclusive) {
	auto gpio = GpioController::GetDefault();
	if(gpio) {
		auto gpio_pin = gpio.OpenPin(pin, share);
		if(share == GpioSharingMode::Exclusive) {
			gpio_pin.Write(init);
			gpio_pin.SetDriveMode(mode);
		}
		return gpio_pin;
	}
	return nullptr;
}

void sensor::set(value_t val) { 
	_value = val; 
	if(is_error(_min) || (value_t)_min > (value_t)_value)	_min = (value_t)_value;
	if(is_error(_max) || (value_t)_max < (value_t)_value)	_max = (value_t)_value;
}

void sensor::reset() {
	_min = (value_t)_value;
	_max = (value_t)_value;
}

// HC-SR501 PIR motion detector

hcsr501_sensor::hcsr501_sensor(wstring_view name, int32_t motion_pin) : sensor(name)
{
	_motion_pin = init_pin(motion_pin, GpioPinDriveMode::Input, GpioPinValue::Low, GpioSharingMode::SharedReadOnly);

	if(_motion_pin) {
		_motion_pin.ValueChanged([this](auto&& pin, auto&& args) {
			_last_activity_time = system_clock::now();
			if(args.Edge() == GpioPinEdge::RisingEdge)	logger.info(module_name, L"activity detected");
		});
	}
}

void hcsr501_sensor::update()
{
	system_clock::time_point last = _last_activity_time;
	auto duration = system_clock::now() - last;
	set(duration_cast<minutes>(duration).count());
}


// TMP75 temperature sensor

task<void> tmp75_sensor::start()
{
	try	{
		auto controller = co_await I2cController::GetDefaultAsync();
		if(!controller)	return;
		auto device = controller.GetDevice(I2cConnectionSettings(_address));

		device.Write({ 0x01, byte(_res << 5) });
		device.Write({ 0 });

		_i2c = device;
	} catch(const winrt::hresult_error&)	{
		logger.error(L"TM75", L"I2C initialization error");
	}
}

void tmp75_sensor::update()
{
	if(_i2c) {
		byte data[2];
		_i2c.Read(data);
		auto temp = (((data[0] << 8) | data[1]) >> 4) >> _res;
		set(temp);
	} else {
		set(error_t::not_implemented);
	}
}

// MCP9808 temperature sensor

task<void> mcp9808_sensor::start()
{
	try {
		auto controller = co_await I2cController::GetDefaultAsync();
		if(!controller)	return;
		auto device = controller.GetDevice(I2cConnectionSettings(_address));

		device.Write({ 0x08, _res });
		device.Write({ 0x05 });

		_i2c = device;
	} catch(const winrt::hresult_error&) {
		logger.error(L"MCP9", L"I2C initialization error");
	}
}

void mcp9808_sensor::update()
{
	if(_i2c) {
		byte data[2];
		_i2c.Read(data);
		auto temp = ((data[0] & 0x0F) << 4) | (data[1] >> 4);
		if(data[0] & 0x10) temp = 256 - temp;
		set(temp);
	} else {
		set(error_t::not_implemented);
	}
}

// Beeper

beeper::beeper(std::wstring_view name, int32_t beeper_pin) : actuator(name)
{
	_beeper_pin = init_pin(beeper_pin, GpioPinDriveMode::Output);
}

task<void> beeper::beep(std::chrono::milliseconds duration)
{
	if(_beeper_pin) {
		_beeper_pin.Write(GpioPinValue::High);
		co_await 300ms;
		_beeper_pin.Write(GpioPinValue::Low);
	}
}

task<void> beeper::fail() {
	co_await beep(100ms);
	co_await 200ms;
	co_await beep(100ms);
	co_await 200ms;
	co_await beep(100ms);
}

// LED

led::led(std::wstring_view name, int32_t led_pin) : actuator(name)
{
	_led_pin = init_pin(led_pin, GpioPinDriveMode::Output);
}

void led::set(bool state)
{
	if(_led_pin)	_led_pin.Write(state ? GpioPinValue::High : GpioPinValue::Low);
}

bool led::get()
{
	return _led_pin && _led_pin.Read() == GpioPinValue::High;
}


}

