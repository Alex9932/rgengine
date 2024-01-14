/*
 * rgEngine core/rgstb.h
 *
 *  Created on: Aug 29, 2022
 *      Author: alex9932
 */
#ifndef _RGSTB_H
#define _RGSTB_H

#define STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.h>
#include "filesystem.h"

struct RG_STB_VORBIS {
    stb_vorbis* stream;
    Resource* resource;
};

// STB Image
RG_DECLSPEC Uint8* RG_STB_load_from_file(String path, int* width, int* height, int* components, int required_components);
RG_DECLSPEC void   RG_STB_image_free(Uint8* data);

// STB Vorbis
RG_DECLSPEC RG_STB_VORBIS   RG_STB_vorbis_open_file(String path, int* error, const stb_vorbis_alloc* alloc);
RG_DECLSPEC stb_vorbis_info RG_STB_vorbis_get_info(stb_vorbis* f);
RG_DECLSPEC void            RG_STB_vorbis_get_info_ptr(stb_vorbis* f, stb_vorbis_info* d);
RG_DECLSPEC Uint32          RG_STB_vorbis_stream_length_in_samples(stb_vorbis* f);
RG_DECLSPEC Float32         RG_STB_vorbis_stream_length_in_seconds(stb_vorbis* f);
RG_DECLSPEC Sint32          RG_STB_vorbis_get_samples_short_interleaved(stb_vorbis* f, int channels, short* buffer, int num_shorts);
RG_DECLSPEC void            RG_STB_vorbis_close(RG_STB_VORBIS* f);
RG_DECLSPEC void            RG_STB_vorbis_seek_start(stb_vorbis* f);
RG_DECLSPEC int             RG_STB_vorbis_get_error(stb_vorbis* f);


#endif