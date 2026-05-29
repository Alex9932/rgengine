#define DLL_EXPORT
#include "rmodelmanager.h"
#include "filesystem.h"
#include "allocator.h"
#include "render.h"
#include "pm2importer.h"

#include <map>
#include <mutex>

#define RG_KM_POOL 1024

namespace Engine {

	static std::mutex s_sm_mtx;
	static std::mutex s_rm_mtx;
	static std::map<Uint64, R3D_StaticModel*> s_staticmodels;
	static std::map<Uint64, R3D_RiggedModel*> s_riggedmodels;
	static std::map<Uint64, KinematicsModel*> s_kinematics;   // Save KinematicsModel instance for each rigged model to avoid loading skeleton multiple times

	static PoolAllocator s_km_allocator("KinematicsModelPool", RG_KM_POOL, sizeof(KinematicsModel));

	static PM2Importer pm2Importer;

	static R3D_StaticModel* SMakeStaticModel(String name) {
		R3DStaticModelInfo model = {};

		char s_path[256];
		char s_file[128];
		
		SDL_snprintf(s_path, 128, "%s/models", GetGamedataPath());
		SDL_snprintf(s_file, 128, "%s.pm2", name);

		rgLogInfo(RG_LOG_SYSTEM, "Loading model: %s", name);

		ImportModelInfo i = {};
		i.path = s_path;
		i.file = s_file;
		i.info.as_static = &model;
		pm2Importer.ImportModel(&i);

		R3D_StaticModel* hmdl = Render::CreateStaticModel(&model);
		FreeModelInfo fminfo = {};
		fminfo.info.as_static = &model;
		pm2Importer.FreeModelData(&fminfo);

		return hmdl;
	}

	static R3D_RiggedModel* SMakeRiggedModel(String name, Uint64 hash) {
		R3DRiggedModelInfo model = {};

		char s_path[256];
		char s_file[128];

		SDL_snprintf(s_path, 128, "%s/models", GetGamedataPath());
		SDL_snprintf(s_file, 128, "%s.pm2", name);

		rgLogInfo(RG_LOG_SYSTEM, "Loading model: %s", name);

		ImportModelInfo i = {};
		i.path = s_path;
		i.file = s_file;
		i.info.as_rigged = &model;
		pm2Importer.ImportRiggedModel(&i);
		KinematicsModel* km = pm2Importer.LoadSkeleton(&i);
		s_kinematics[hash] = km;

		R3D_RiggedModel* hrmdl = Render::CreateRiggedModel(&model);
		FreeModelInfo fminfo = {};
		fminfo.info.as_rigged = &model;
		fminfo.extra = i.extra;
		fminfo.userdata = i.userdata;
		pm2Importer.FreeRiggedModelData(&fminfo);

		return hrmdl;
	}

	R3D_StaticModel* GetStaticModel(String name) {
		Uint64 hash = rgHash(name, SDL_strlen(name));

		std::lock_guard<std::mutex> lk(s_sm_mtx);
		auto it = s_staticmodels.find(hash);
		if (it != s_staticmodels.end()) {
			// Increment reference count and return existing model
			it->second->refcounter += 1;
			return it->second;
		}

		// Create new model instance and cache it with refcount 1
		R3D_StaticModel* mdl = SMakeStaticModel(name);
		s_staticmodels[hash] = mdl;
		return mdl;
	}

	void FreeStaticModel(R3D_StaticModel* mdl) {
		if (!mdl) return;

		std::lock_guard<std::mutex> lk(s_sm_mtx);

		// Find entry by pointer
		for (auto it = s_staticmodels.begin(); it != s_staticmodels.end(); ++it) {
			if (it->second == mdl) {
				if (it->second->refcounter > 1) {
					it->second->refcounter -= 1;
				} else {
					// refcount reaches zero -> delete and erase entry
					Render::DestroyStaticModel(it->second);
					s_staticmodels.erase(it);
				}
				return;
			}
		}
		// pointer not found -> no-op
	}

	R3D_RiggedModel* GetRiggedModel(String name) {
		Uint64 hash = rgHash(name, SDL_strlen(name));

		std::lock_guard<std::mutex> lk(s_rm_mtx);
		auto it = s_riggedmodels.find(hash);
		if (it != s_riggedmodels.end()) {
			// Increment reference count and return existing model
			it->second->s_model.refcounter += 1;
			return it->second;
		}

		// Create new model instance and cache it with refcount 1
		R3D_RiggedModel* mdl = SMakeRiggedModel(name, hash);
		s_riggedmodels[hash] = mdl;
		return mdl;
	}

	void FreeRiggedModel(R3D_RiggedModel* rmdl) {
		if (!rmdl) return;

		std::lock_guard<std::mutex> lk(s_rm_mtx);

		// Find entry by pointer
		for (auto it = s_riggedmodels.begin(); it != s_riggedmodels.end(); ++it) {
			if (it->second == rmdl) {
				if (it->second->s_model.refcounter > 1) {
					it->second->s_model.refcounter -= 1;
				}
				else {
					// refcount reaches zero -> delete and erase entry
					Render::DestroyRiggedModel(it->second);
					s_riggedmodels.erase(it);
				}
				return;
			}
		}
	}

	KinematicsModel* GetUniqueKinematicsModel(R3D_RiggedModel* rmdl) {
		if (!rmdl) return NULL;
		std::lock_guard<std::mutex> lk(s_rm_mtx);
		// Find entry by pointer
		Uint64 hash = 0;
		for (auto it = s_riggedmodels.begin(); it != s_riggedmodels.end(); ++it) {
			if (it->second == rmdl) {
				// Model's hash found
				hash = it->first;
				break;
			}
		}

		// Base model
		KinematicsModel* base_instance = s_kinematics[hash];

		// Clone that model
		KinematicsModel* km = (KinematicsModel*)s_km_allocator.Allocate();
		SDL_memcpy(km, base_instance, sizeof(KinematicsModel));
		// WARN: Do not use Animator of base instance!
		// Make new animator
		km->SetAnimator(NULL);
		km->MakeAnimator();

		return km;

	}

	void FreeUniqueKinematicsModel(KinematicsModel* km) {
		if (!km) return;
		RG_DELETE_CLASS((&s_km_allocator), KinematicsModel, km);
	}

}