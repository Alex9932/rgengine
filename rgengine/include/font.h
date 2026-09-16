/*
 * rgEngine font.h
 *
 *  Created on: Dec 26, 2024
 *      Author: alex9932
 */

#ifndef _FONT_H
#define _FONT_H

#include "rgtypes.h"

#define RG_FONT_ATLAS_WIDTH  128
#define RG_FONT_ATLAS_HEIGHT 128

namespace Engine {

	// TODO: Use 16-bit ints
	typedef struct Glyph {
		Sint32 size_x;
		Sint32 size_y;
		Sint32 bearing_x;
		Sint32 bearing_y;
		Uint32 advance_x;
		Uint32 advance_y;
	} Glyph;

	class Font {

		public:
			Font(String font, Uint32 size);
			virtual ~Font();

			RG_DECLSPEC Float32 GetRawStringLength(Uint16* str, Uint32 len);
			RG_INLINE Float32 GetRawStringLength(Uint16* str) { GetRawStringLength(str, -1); }

			RG_INLINE Uint32 GetScale()  { return m_scale; }
			RG_INLINE Glyph* GetGlyphs() { return m_glyphs; }
			RG_INLINE Uint8* GetBitmap() { return m_bitmap; }

		private:
			Uint32 m_scale;
			Glyph* m_glyphs;
			Uint8* m_bitmap;

	};
}

#endif