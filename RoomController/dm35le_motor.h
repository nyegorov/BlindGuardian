#pragma once

#include "value.h"
#include "sensors.h"
#include "log_manager.h"

namespace roomctrl {

class dm35le_motor final : public i_motor
{
public:
	enum command : byte { cmdUp = 1, cmdDown = 3, cmdStop = 5 };
	const unsigned repeat_count = 10;

	dm35le_motor(int32_t tx_pin);
	~dm35le_motor();
	void start() override	{}
	void open() override	{ send_cmd(0, cmdUp, repeat_count); }
	void close() override	{ send_cmd(0, cmdDown, repeat_count); }

	unsigned long long read_cmd(int32_t rx_pin);
	void send_cmd(byte channel, command code, uint32_t repeat);

private:
	winrt::Windows::Devices::Gpio::GpioPin	_tx_pin{ nullptr };
};

}