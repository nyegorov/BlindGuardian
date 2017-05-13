#include "ActivationFactory.h"

#include "roomctrl_h.h"
#include "room_engine.h"
#include "http_server.h"

using namespace winrt::Windows::Data::Json;
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
		std::unique_ptr<room_server>	_server;
		IBackgroundTaskDeferral			_deferral;
	};

	ACTIVATABLE_OBJECT(StartupTask, RuntimeClass_roomctrl_StartupTask);

}
