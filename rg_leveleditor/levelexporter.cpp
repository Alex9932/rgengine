#include "levelexporter.h"
#include <levelloader.h>

#include <filesystem.h>

#include <world.h>
#include <entity.h>
#include <engine.h>

namespace Engine {
	void ExportLevel(String name) {
		char path[256];
		char levels[256];
		GetPath(levels, 256, RG_PATH_GAMEDATA, "levels");
		SDL_snprintf(path, 256, "%s/%s", levels, name);
		FS_MakeDir(path);

		char f_level[256];
		char f_light[256];
		char f_lgeom[256];

		rgLogInfo(RG_LOG_SYSTEM, "Level: %s", path);

		World* world = GetWorld();
		Uint32 c = world->GetEntityCount();

		for (Uint32 i = 0; i < c; i++) {
			Entity* ent = world->GetEntity(i);

			//ent->

		}


	}
}