#define DLL_EXPORT
#include "rgstring.h"

namespace Engine {
	String GetCategoryStr(int category) {
		switch (category) {
			case RG_LOG_GAME:   return " game ";
			case RG_LOG_ERROR:  return "error";
			case RG_LOG_ASSERT: return "assert";
			case RG_LOG_SYSTEM: return "system";
			case RG_LOG_AUDIO:  return "audio";
			case RG_LOG_RENDER: return "render";
			case RG_LOG_DEBUG:  return "DEBUG";
			case RG_LOG_MEMORY: return "memory";
			case RG_LOG_SCRIPT: return "script";
			default:            return "app";
		}
	}

	String GetPriorityStr(SDL_LogPriority priority) {
		switch (priority) {
			case SDL_LOG_PRIORITY_VERBOSE:  return " @@";
			case SDL_LOG_PRIORITY_DEBUG:    return "DBG";
			case SDL_LOG_PRIORITY_INFO:     return " **";
			case SDL_LOG_PRIORITY_WARN:     return " !!";
			case SDL_LOG_PRIORITY_ERROR:    return " EE";
			case SDL_LOG_PRIORITY_CRITICAL: return " EE";
			default:                        return "MSG";
		}
	}

	String GetCpuNameStr(int cores) {
		if (cores <= 0) { return "unknown";     } // Where are you found this CPU?
		if (cores == 1) { return "single-core"; } // 
		if (cores == 2) { return "dual-core";   } // WTF Why do you use this CPUs?
		if (cores == 3) { return "triple-core"; } // 
		if (cores == 4) { return "quad-core";   }
		if (cores == 8) { return "octal-core";  }
		
		static char str[32];
		SDL_snprintf(str, 32, "%d-core", cores);
		return str;
	}

	Bool rg_streql(String left, String right) {
		size_t lenpre = SDL_strlen(left);
		size_t lenstr = SDL_strlen(right);
		return ((lenstr == lenpre) && (SDL_memcmp(left, right, lenpre) == 0)) ? true : false;
	}

	Bool rg_strstw(String left, String right) {
		size_t llen = SDL_strlen(left);  // Src string
		size_t rlen = SDL_strlen(right); // Dst string
		if (llen < rlen) { return false; }

		for (size_t i = 0; i < rlen; ++i) {
			if (left[i] != right[i]) {
				return false;
			}
		}
		return true;
	}

	Bool rg_strenw(String left, String right) {
		size_t llen = SDL_strlen(left);  // Src string
		size_t rlen = SDL_strlen(right); // Dst string
		if (llen < rlen) { return false; }
		String strptr = &left[llen - rlen];
		return rg_streql(strptr, right);
	}

	Sint32 rg_strcharats(String str, char c) {
		Sint32 lenstr = (Sint32)SDL_strlen(str);
		for (Sint32 i = 0; i < lenstr; ++i) {
			if (str[i] == c) { return i; }
		}
		return -1;
	}

	Sint32 rg_strcharate(String str, char c) {
		// TODO: Optimize of possible
		Sint32 lenstr = (Sint32)SDL_strlen(str);
		for (Sint32 i = lenstr - 1; i >= 0; --i) {
			if (str[i] == c) { return i; }
		}
		return -1;
	}
}