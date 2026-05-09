#define DLL_EXPORT
#include "animexporter.h"
#include "filesystem.h"
#include "pm2anim.h"

// Structures
// [Header]
//  char   magic[4] "RGA "
//  Float32 fps;    Framerate
//  Uint16 tracks;  Animation tracks count
//  Uint16 version; Version

// [Track]
//  char name[32]; Track name (Bone name)
//  Uint32 keyframes; Keyframes count

// [Keyframe]
//  Float32 timestamp; Keyframe timestamp in seconds
//  quat    rotation; Bone rotation
//  vec3    translation; Bone translation
//  vec3    scale; Bone scale
//  Sint8   interp[16]; Interpolation data for Bezier curve (2 control points (x,y) for each component x, y, z and r) ( 2 * 2 * 4 = 16 bytes )

// Animation format
//  - Header
//  - Track
//      - Keyframe[]
//  - Track
//      - Keyframe[]
//  - Track
//      - Keyframe[]
// ...

namespace Engine {

	static void WriteTrack(FSWriter* writer, AnimationTrack* track) {
		AnimTrack atrack = {};
		SDL_snprintf(atrack.name, 32, "%s", track->GetName());
		atrack.keyframes = track->GetKeyframeCount();
		writer->Write(&atrack, sizeof(AnimTrack));
		for (Uint32 i = 0; i < atrack.keyframes; i++) {
			BoneKeyFrame* frame = track->GetKeyFrame(i);
			writer->WriteF32(frame->timestamp);
			writer->Write4F32(&frame->rotation.v4);
			writer->Write3F32(&frame->translation);
			writer->Write3F32(&frame->scale);
			// Calculate and write interpolation data for Bezier curve (float -1 - 1 => uint8 -128 - 127)
			writer->WriteS8((Sint8)(frame->interp_x.x * 127));
			writer->WriteS8((Sint8)(frame->interp_x.y * 127));
			writer->WriteS8((Sint8)(frame->interp_x.z * 127));
			writer->WriteS8((Sint8)(frame->interp_x.w * 127));
			writer->WriteS8((Sint8)(frame->interp_y.x * 127));
			writer->WriteS8((Sint8)(frame->interp_y.y * 127));
			writer->WriteS8((Sint8)(frame->interp_y.z * 127));
			writer->WriteS8((Sint8)(frame->interp_y.w * 127));
			writer->WriteS8((Sint8)(frame->interp_z.x * 127));
			writer->WriteS8((Sint8)(frame->interp_z.y * 127));
			writer->WriteS8((Sint8)(frame->interp_z.z * 127));
			writer->WriteS8((Sint8)(frame->interp_z.w * 127));
			writer->WriteS8((Sint8)(frame->interp_r.x * 127));
			writer->WriteS8((Sint8)(frame->interp_r.y * 127));
			writer->WriteS8((Sint8)(frame->interp_r.z * 127));
			writer->WriteS8((Sint8)(frame->interp_r.w * 127));
		}
	}

	void AnimExporter::ExportAnimation(String path, Animation* anim) {
		FSWriter writer(path);
		
		AnimHeader header = {};
		header.magic[0] = 'R';
		header.magic[1] = 'G';
		header.magic[2] = 'A';
		header.magic[3] = ' ';
		header.fps      = anim->GetFramerate();
		header.tracks   = anim->GetAnimationTrackCount();
		header.version  = 1;
		writer.Write(&header, sizeof(AnimHeader));
		
		for (Uint32 i = 0; i < header.tracks; i++) {
			WriteTrack(&writer, anim->GetBoneAnimationTracks()[i]);
		}

	}

}