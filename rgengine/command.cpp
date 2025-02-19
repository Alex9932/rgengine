#include "command.h"

#include "engine.h"
#include "rgstring.h"

namespace Engine {

	void ExecuteCommand(String cmd) {
		rgLogInfo(RG_LOG_GAME, "@ %s", cmd);

		if (rg_strstw(cmd, "quit")) {
			Quit();
		}
	}

}