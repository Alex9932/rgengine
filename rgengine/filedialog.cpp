#define DLL_EXPORT
#include "filedialog.h"

#include "window.h"
#include "engine.h"

#include <nfd.h>
#include <nfd_sdl2.h>

namespace Engine {

	static void GetNativeWindowHandle(nfdwindowhandle_t* handle) {
		SDL_Window* hwnd = GetWindow();
		NFD_GetNativeWindowFromSDLWindow(hwnd, handle);
	}

	void RG_NFDInit() {
		if (NFD_Init() != NFD_OKAY) {
			rgLogError(RG_LOG_SYSTEM, "NFD_Init failed: %s\n", NFD_GetError());
			RG_ERROR_MSG("NFD_Init failed! Check log for more information.");
		}
	}

	void RG_NFDDestroy() {
		NFD_Quit();
	}

	Bool ShowOpenDialog(char* dst_path, Uint32 maxlen, FD_Filter* filters, Uint32 filter_count) {

		rgLogInfo(RG_LOG_GAME, "Show OpenDialog");
		nfdopendialogu8args_t args = {};
		GetNativeWindowHandle(&args.parentWindow);
		//nfdu8filteritem_t filters[4] = { {"PM2 Model file", "pm2"}, {"Wavefront model", "obj"}, {"MMD Polygon model", "pmd"}, {"MMD Extended polygon model", "pmx"} };
		//args.filterList = filters;
		//args.filterCount = 4;
		//args.defaultPath = "/";

		args.filterList  = (nfdu8filteritem_t*)filters;
		args.filterCount = filter_count;

		nfdu8char_t* outPath;
		nfdresult_t res = NFD_OpenDialogU8_With(&outPath, &args);

		switch (res) {
			case NFD_OKAY: {
				rgLogInfo(RG_LOG_GAME, "File selected: %s", outPath);
				SDL_snprintf(dst_path, maxlen, "%s", outPath);
				NFD_FreePathU8(outPath);
				return true;
			}

			case NFD_CANCEL: { rgLogInfo(RG_LOG_GAME, "NFD: Cancel"); break;         }
			case NFD_ERROR:  { rgLogInfo(RG_LOG_GAME, "NFD: Internal error"); break; }
			default:         { rgLogInfo(RG_LOG_GAME, "NFD: Default case"); break;   }
		}

		return false;

	}

}