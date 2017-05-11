#include "pch.h"
#include "sensors.h"
#include "log_manager.h"

using namespace winrt::Windows::System::Threading;
using namespace winrt::Windows::Devices::I2c;
using namespace winrt::Windows::Devices::Gpio;
using namespace winrt::Windows::Devices::Enumeration;
using namespace std::chrono;

namespace roomctrl {

GpioPin init_pin(int32_t pin, GpioPinDriveMode mode, GpioPinValue init = GpioPinValue::Low) {
	auto gpio = GpioController::GetDefault();
	if(gpio) {
		auto gpio_pin = gpio.OpenPin(pin);
		gpio_pin.SetDriveMode(mode);
		if(mode == GpioPinDriveMode::Output)	gpio_pin.Write(init);
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
	_motion_pin = init_pin(motion_pin, GpioPinDriveMode::Input);

	if(_motion_pin) {
		_motion_pin.ValueChanged([this](auto&& pin, auto&& args) {
			if(args.Edge() == GpioPinEdge::FallingEdge)	_last_activity_time = system_clock::now();
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

std::future<void> tmp75_sensor::start()
{
	try	{
		auto controller = co_await I2cController::GetDefaultAsync();
		auto device = controller.GetDevice(I2cConnectionSettings(_address));

		device.Write({ 0x01, byte(_res << 5) });
		device.Write({ 0 });

		_tmp75 = device;
	} catch(const winrt::hresult_error&)	{
		logger.error(L"TM75", L"I2C initialization error");
	}
}

void tmp75_sensor::update()
{
	if(_tmp75) {
		byte data[2];
		_tmp75.Read(data);
		value_type temp_sum = (((data[0] << 8) | data[1]) >> 4);
		set(temp_sum >> _res);
	} else {
		set(error_t::not_implemented);
	}
}

// Beeper

beeper::beeper(std::wstring_view name, int32_t beeper_pin) : actuator(name)
{
	_beeper_pin = init_pin(beeper_pin, GpioPinDriveMode::Output, GpioPinValue::Low);
}

template<class R, class P> void beeper::make_beep(std::chrono::duration<R, P> duration) 
{
	if(_beeper_pin) {
		_beeper_pin.Write(GpioPinValue::High);
		std::this_thread::sleep_for(duration);
		_beeper_pin.Write(GpioPinValue::Low);
	}
}

// LED

led::led(std::wstring_view name, int32_t led_pin) : actuator(name)
{
	_led_pin = init_pin(led_pin, GpioPinDriveMode::Output, GpioPinValue::High);
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

