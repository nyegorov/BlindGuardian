#include "ActivationFactory.h"

#include "roomctrl_h.h"
#include "room_engine.h"

using namespace winrt::Windows::ApplicationModel::Background;

namespace roomctrl {

	class StartupTask : public winrt::implements<StartupTask, IBackgroundTask>
	{
	public:
		StartupTask();
		void Run(IBackgroundTaskInstance taskInstance);
	private:
		std::unique_ptr<room_server>	_server;
		BackgroundTaskDeferral			_deferral{ nullptr };
	};

	ACTIVATABLE_OBJECT(StartupTask, RuntimeClass_roomctrl_StartupTask);

}
