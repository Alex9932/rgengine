/*
 * rgEngine rgstring.h
 *
 *  Created on: Feb 22, 2022
 *      Author: alex9932
 */
#ifndef _RGSTRING_H
#define _RGSTRING_H

#include "rgtypes.h"

namespace Engine {

	RG_DECLSPEC String GetCategoryStr(int category);
	RG_DECLSPEC String GetPriorityStr(SDL_LogPriority priority);
	RG_DECLSPEC String GetCpuNameStr(int cores);

	RG_DECLSPEC Bool rg_streql(String left, String right); // Is strings equal
	RG_DECLSPEC Bool rg_strstw(String left, String right); // Starts with
	RG_DECLSPEC Bool rg_strenw(String left, String right); // Ends with

	RG_DECLSPEC Sint32 rg_strcharats(String str, char c); // Return char position from start of string
	RG_DECLSPEC Sint32 rg_strcharate(String str, char c); // Return char position from end of string

}

#endif