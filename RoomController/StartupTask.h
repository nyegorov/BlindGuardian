#include "ActivationFactory.h"

#include "roomctrl_h.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::ApplicationModel::Background;
using namespace winrt::Windows::Devices::Gpio;
using namespace winrt::Windows::System::Threading;

namespace roomctrl {

	class StartupTask : public winrt::implements<StartupTask, IBackgroundTask>
	{
	public:
		StartupTask();
		void Run(IBackgroundTaskInstance taskInstance);
	private:
		void InitGpio();

		IBackgroundTaskDeferral Deferral;
		ThreadPoolTimer Timer{ nullptr };
		GpioPinValue pinValue;
		const int LED_PIN = 5;
		GpioPin pin{ nullptr };
	};

	ACTIVATABLE_OBJECT(StartupTask, RuntimeClass_roomctrl_StartupTask);

}
