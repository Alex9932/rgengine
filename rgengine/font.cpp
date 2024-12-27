/*
 * rgEngine font.cpp
 *
 *  Created on: Dec 26, 2024
 *      Author: alex9932
 */

#define DLL_EXPORT

#include "font.h"
#include "allocator.h"
#include "filesystem.h"

#include "engine.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Engine {

	static Sint32        font_count = 0;
	static FT_Library    ft         = NULL;
	static STDAllocator* allocator  = NULL;

	static FT_Library GetFTLibrary() {
		if (ft == NULL) {
			rgLogInfo(RG_LOG_DEBUG, "Init library");
			if (FT_Init_FreeType(&ft)) {
				rgLogError(RG_LOG_RENDER, "Could not initialize freetype library!");
				return NULL;
			}

			FT_Int maj, min, p;
			FT_Library_Version(ft, &maj, &min, &p);

			rgLogInfo(RG_LOG_DEBUG, "Freetype: %d.%d.%d", maj, min, p);

			allocator = RG_NEW(STDAllocator)("FontAlloc");
		}

		font_count++;

		return ft;
	}

	static void FreeFTLibrary() {
		font_count--;
		if (font_count == 0) {
			rgLogInfo(RG_LOG_DEBUG, "~Destroy library");
			FT_Done_FreeType(ft);
			RG_DELETE(STDAllocator, allocator);
		}
	}

	static void SubData(Uint32 offsetx, Uint32 offsety, Uint8* dst, Uint8* data, Uint32 dstw, Uint32 w, Uint32 h) {
		for (Uint32 y = 0; y < h; y++) {
			for (Uint32 x = 0; x < w; x++) {
				Uint32 data_ptr = w * y + x;
				Uint32 dst_ptr = dstw * (offsety + y) + offsetx + x;

#if 0
				// 1-channel texture (r-only)
				dst[dst_ptr] = data[data_ptr];
#else
				dst[dst_ptr * 4 + 0] = data[data_ptr];
				dst[dst_ptr * 4 + 1] = data[data_ptr];
				dst[dst_ptr * 4 + 2] = data[data_ptr];
				dst[dst_ptr * 4 + 3] = data[data_ptr];
#endif
			}
		}
	}

	Font::Font(String file, Uint32 scale) {
		FT_Library ftlib = GetFTLibrary();

		Uint32 glyph_count = RG_FONT_ATLAS_WIDTH * RG_FONT_ATLAS_HEIGHT;
		Uint32 glyph_size  = scale * scale;

		m_scale  = scale;
		m_glyphs = (Glyph*)allocator->Allocate(glyph_count * sizeof(Glyph));
		m_bitmap = (Uint8*)allocator->Allocate(glyph_count * glyph_size * 4); // TODO: Use 1-channel texture

		// Load fontfile
		FT_Face face;

		rgLogInfo(RG_LOG_RENDER, "Loading font: %s", file);
		Resource* f_res = GetResource(file);
		if (FT_New_Memory_Face(ft, (const FT_Byte*)f_res->data, f_res->length, 0, &face)) {
			RG_ASSERT_MSG(NULL, "Could not open font!");
		}

		if (face->charmap->encoding != 0x756e6963) {
			RG_ASSERT_MSG(NULL, "Font encoding must be a unicode!");
		}

		FT_Set_Pixel_Sizes(face, 0, m_scale);

		// Render glyphs
		for (wchar_t i = 0; i < glyph_count; i++) {
			if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
				SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Failed to load Glyph!");
			}

			m_glyphs[i].advance_x = face->glyph->advance.x;
			m_glyphs[i].advance_y = face->glyph->advance.y;
			m_glyphs[i].bearing_x = face->glyph->bitmap_left;
			m_glyphs[i].bearing_y = face->glyph->bitmap_top;
			m_glyphs[i].size_x = face->glyph->bitmap.width;
			m_glyphs[i].size_y = face->glyph->bitmap.rows;

			Sint32 xoffset = i % RG_FONT_ATLAS_WIDTH;
			Sint32 yoffset = i / RG_FONT_ATLAS_WIDTH;

			FT_Bitmap* bitmap = &face->glyph->bitmap;
			SubData(xoffset * m_scale, yoffset * m_scale, m_bitmap, bitmap->buffer, m_scale * RG_FONT_ATLAS_WIDTH, bitmap->width, bitmap->rows);
		}

		FT_Done_Face(face);
		Engine::FreeResource(f_res);

	}

	Font::~Font() {
		allocator->Deallocate(m_glyphs);
		allocator->Deallocate(m_bitmap);
		FreeFTLibrary();
	}

	Float32 Font::GetRawStringLength(Uint16* str, Uint32 len) {
		Float32 x = 0;
		Uint16 c = 0;
		for (Uint32 i = 0; ((c = str[i]) != 0 && i < len); i++) {
			x += (m_glyphs[c].advance_x >> 6);
		}
		return x;
	}

}