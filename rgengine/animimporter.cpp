#define DLL_EXPORT
#include "animimporter.h"

#include "engine.h"

#include "filesystem.h"
#include "allocator.h"

#include "pm2anim.h"

namespace Engine {

	static Float32 ReadTrack(AnimationTrack* atrack, AnimTrack* track, FSReader* reader) {
		Float32 time = 0;

		for (Uint32 i = 0; i < track->keyframes; i++) {
			AnimKeyframe frame = {};
			frame.timestamp = reader->ReadF32();
			if (frame.timestamp > time) {
				time = frame.timestamp;
			}
			reader->Read4F32(frame.rotation.v4);
			reader->Read3F32(frame.translation);
			reader->Read3F32(frame.scale);
			reader->Read(frame.interp, 16);

			BoneKeyFrame keyframe = {};
			keyframe.timestamp   = frame.timestamp;
			keyframe.rotation    = frame.rotation;
			keyframe.translation = frame.translation;
			keyframe.scale       = frame.scale;
			keyframe.interp_x.x  = (Float32)frame.interp[0] / 127.0f;
			keyframe.interp_x.y  = (Float32)frame.interp[1] / 127.0f;
			keyframe.interp_x.z  = (Float32)frame.interp[2] / 127.0f;
			keyframe.interp_x.w  = (Float32)frame.interp[3] / 127.0f;
			keyframe.interp_y.x  = (Float32)frame.interp[4] / 127.0f;
			keyframe.interp_y.y  = (Float32)frame.interp[5] / 127.0f;
			keyframe.interp_y.z  = (Float32)frame.interp[6] / 127.0f;
			keyframe.interp_y.w  = (Float32)frame.interp[7] / 127.0f;
			keyframe.interp_z.x  = (Float32)frame.interp[8] / 127.0f;
			keyframe.interp_z.y  = (Float32)frame.interp[9] / 127.0f;
			keyframe.interp_z.z  = (Float32)frame.interp[10] / 127.0f;
			keyframe.interp_z.w  = (Float32)frame.interp[11] / 127.0f;
			keyframe.interp_r.x  = (Float32)frame.interp[12] / 127.0f;
			keyframe.interp_r.y  = (Float32)frame.interp[13] / 127.0f;
			keyframe.interp_r.z  = (Float32)frame.interp[14] / 127.0f;
			keyframe.interp_r.w  = (Float32)frame.interp[15] / 127.0f;
			atrack->AddKeyFrame(&keyframe);
		}

		return time;
	}

	Animation* AnimImporter::ImportAnimation(String path) {

		char _path[256];
		char _file[128];
		FS_SeparatePathFile(_path, 256, _file, 128, path);

		FSReader reader(path);

		AnimHeader header;
		reader.Read(&header, sizeof(AnimHeader));
		if (header.magic[0] != 'R' ||
			header.magic[1] != 'G' ||
			header.magic[2] != 'A' ||
			header.magic[3] != ' ') {
			char buffer[128];
			SDL_snprintf(buffer, 128, "%s is not a PM2 animation file!", reader.GetResourcePath());
			RG_ERROR_MSG(buffer);
		}

		AnimationTrack* tracks = (AnimationTrack*)GetDefaultAllocator()->Allocate(sizeof(AnimationTrack) * header.tracks);
		Animation* anim = RG_NEW(Animation)(_file, tracks);
		anim->SetFramerate(header.fps);

		Float32 lastframe = 0;

		for (Uint32 i = 0; i < header.tracks; i++) {
			AnimTrack track = {};
			reader.Read(&track, sizeof(AnimTrack));
			AnimationTrack* atrack = new(&tracks[i]) AnimationTrack(track.name);
			anim->AddBoneAnimationTrack(atrack);
			Float32 time = ReadTrack(atrack, &track, &reader);
			if (lastframe < time) {
				lastframe = time;
			}
		}

		anim->Finish(lastframe);
		return anim;
	}

}