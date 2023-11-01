#define DLL_EXPORT
#include "rgstring.h"

namespace Engine {
	String GetCategoryStr(int category) {
		switch (category) {
			case RG_LOG_GAME:   return " game ";
			case RG_LOG_SYSTEM: return "system";
			case RG_LOG_RENDER: return "render";
			case RG_LOG_ERROR:  return "error";
			case RG_LOG_ASSERT: return "assert";
			case RG_LOG_AUDIO:  return "audio";
			case RG_LOG_DEBUG:  return "DEBUG";
			default:            return "app";
		}
	}

	String GetPriorityStr(SDL_LogPriority priority) {
		switch (priority) {
			case SDL_LOG_PRIORITY_VERBOSE:  return "Verb ";
			case SDL_LOG_PRIORITY_DEBUG:    return "DEBUG";
			case SDL_LOG_PRIORITY_INFO:     return "Info ";
			case SDL_LOG_PRIORITY_WARN:     return "Warn ";
			case SDL_LOG_PRIORITY_ERROR:    return "Error";
			case SDL_LOG_PRIORITY_CRITICAL: return "Crit ";
			default:                        return "Msg  ";
		}
	}

	String GetCpuNameStr(int cores) {
		switch (cores) {
			case 0:  return "WTF?";
			case 1:  return "single-core";
			case 2:  return "dual-core";
			case 3:  return "triple-core";
			case 4:  return "quad-core";
			case 5:  return "5 cores";
			case 6:  return "6 cores";
			case 7:  return "7 cores";
			case 8:  return "octal-core";
			case 9:  return "9 cores";
			case 10: return "10 cores";
			case 11: return "11 cores";
			case 12: return "12 cores";
			case 13: return "13 cores";
			case 14: return "14 cores";
			case 15: return "15 cores";
			case 16: return "16 cores";
			case 17: return "17 cores";
			case 18: return "18 cores";
			case 19: return "19 cores";
			case 20: return "20 cores";
			default: return "unknown";
		}
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