#pragma once

#include "pch.h"
#include "Sensor.h"

namespace BlindGuardian
{
	[Windows::Foundation::Metadata::WebHostHidden]
    public ref class StartupTask sealed : public Windows::ApplicationModel::Background::IBackgroundTask
    {
    public:
		StartupTask();
        virtual void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ taskInstance);

	private:
		void InitGpio();
	private:
		Platform::Agile<Windows::ApplicationModel::Background::BackgroundTaskDeferral> Deferral;
		Windows::ApplicationModel::Background::IBackgroundTaskInstance^ TaskInstance;
		Windows::System::Threading::ThreadPoolTimer ^Timer;
		Windows::Devices::Gpio::GpioPinValue pinValue;
		const int LED_PIN = 5;
		Windows::Devices::Gpio::GpioPin ^pin;

/*		TemperatureSensor _tempIn{ "tin" };
		TemperatureSensor _tempOut{ "tout" };
		LightSensor _lightOut{ "light" };*/
    };
}
