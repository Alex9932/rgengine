#define DLL_EXPORT
#include "rgstb.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#undef STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.h>

Uint8* RG_STB_load_from_file(String path, int* width, int* height, int* components, int required_components) {
    rgLogInfo(RG_LOG_SYSTEM, "Loading image: %s", path);
    Resource* res = Engine::GetResource(path);
    Uint8* data = stbi_load_from_memory((stbi_uc*)res->data, res->length, width, height, components, required_components);
    Engine::FreeResource(res);
    //    rgLogInfo(RG_LOG_SYSTEM, " Image -> %dx%d %d", *width, *height, *components);
    return data;
}

void RG_STB_image_free(Uint8* data) {
    stbi_image_free(data);
}

RG_STB_VORBIS RG_STB_vorbis_open_file(String path, int* error, const stb_vorbis_alloc* alloc) {
    rgLogInfo(RG_LOG_SYSTEM, "Loading audio: %s", path);
#if 1
    RG_STB_VORBIS res;
    res.resource = Engine::GetResource(path);
    res.stream = stb_vorbis_open_memory((unsigned char*)res.resource->data, res.resource->length, error, alloc);
    return res;
#else
    FILE* f = fopen(path, "rb");
    return stb_vorbis_open_file(f, 1, error, alloc);
#endif
}

stb_vorbis_info RG_STB_vorbis_get_info(stb_vorbis* f) {
    return stb_vorbis_get_info(f);
}

Uint32 RG_STB_vorbis_stream_length_in_samples(stb_vorbis* f) {
    return stb_vorbis_stream_length_in_samples(f);
}

Float32 RG_STB_vorbis_stream_length_in_seconds(stb_vorbis* f) {
    return stb_vorbis_stream_length_in_seconds(f);
}

Sint32 RG_STB_vorbis_get_samples_short_interleaved(stb_vorbis* f, int channels, short* buffer, int num_shorts) {
    return stb_vorbis_get_samples_short_interleaved(f, channels, buffer, num_shorts);
}

void RG_STB_vorbis_close(RG_STB_VORBIS* f) {
    stb_vorbis_close(f->stream);
    Engine::FreeResource(f->resource);
}
void RG_STB_vorbis_seek_start(stb_vorbis* f) {
    stb_vorbis_seek_start(f);
}

int RG_STB_vorbis_get_error(stb_vorbis* f) {
    return stb_vorbis_get_error(f);
}
