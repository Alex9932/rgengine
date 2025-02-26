#include "command.h"

#include "engine.h"
#include "rgstring.h"

#include "scriptengine.h"

namespace Engine {

	void ExecuteCommand(String cmd) {
		rgLogInfo(RG_LOG_GAME, "@ %s", cmd);

		if (rg_strstw(cmd, "quit")) { Quit(); }

		if (rg_strstw(cmd, "script")) {
			Sint32 separator_idx = rg_strcharats(cmd, ' ');
			if (separator_idx < 0) { rgLogWarn(RG_LOG_SYSTEM, "Invalid command!"); return; }

			String filename = &cmd[separator_idx + 1];

			rgLogInfo(RG_LOG_SYSTEM, "Loading script: %s", filename);

			RGScriptState* s = Script_MakeState();
			Script_ExecuteFile(s, filename);
			Script_FreeState(s);

		}

	}

}