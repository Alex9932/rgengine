#define DLL_EXPORT
#include "console.h"

#include "rendertypes.h"
#include "utf8.h"

#include "logger.h"
#include "event.h"

#include "render.h"
#include "window.h"

#include "font.h"
#include "allocator.h"

#define RG_FONT_VBUFFER_SIZE 512
#define RG_COMMAND_LENGTH 1024
#define RG_R2D_MAX_VERTICES 16384

namespace Engine {

    static Bool is_shown = false;

    static Uint16 command_buffer[RG_COMMAND_LENGTH];
    static Uint32 cursor = 0;

    static R2D_Vertex vertices[RG_R2D_MAX_VERTICES];

    static UTF8Decoder utf8_decoder;
    static UTF8Encoder utf8_encoder;

    static Renderer* r_ctx;

    static R2D_Texture* font_tex;

    static R2D_Buffer* buff1; // bg
    static R2D_Buffer* buff2; // log console
    static R2D_Buffer* buff3; // command line

    static ivec2 scr_size = { 0, 0 };

    static Font* font;


    static Bool Input(SDL_Event* event) {

        return true;
    }

    static void PushChar(char c, vec4* color, Float32 scale, Uint32* idx, Float32* x, Float32* y) {

        Glyph g = font->GetGlyphs()[c];
        Float32 xpos = *x + g.bearing_x * scale;
        Float32 ypos = *y - (g.size_y - g.bearing_y) * scale;
        Float32 w = g.size_x * scale;
        Float32 h = g.size_y * scale;
        Float32 u = (Float32)(c % RG_FONT_ATLAS_WIDTH) / (Float32)RG_FONT_ATLAS_WIDTH;
        Float32 v = (Float32)(c / RG_FONT_ATLAS_WIDTH) / (Float32)RG_FONT_ATLAS_HEIGHT;
        Float32 su = (Float32)g.size_x / (Float32)(RG_FONT_ATLAS_WIDTH * font->GetScale());
        Float32 sv = (Float32)g.size_y / (Float32)(RG_FONT_ATLAS_HEIGHT * font->GetScale());
        R2D_Vertex v0 = { xpos    , ypos + h, u     , v     , color->x, color->y, color->z, color->w };
        R2D_Vertex v1 = { xpos    , ypos    , u     , v + sv, color->x, color->y, color->z, color->w };
        R2D_Vertex v2 = { xpos + w, ypos    , u + su, v + sv, color->x, color->y, color->z, color->w };
        R2D_Vertex v3 = { xpos    , ypos + h, u     , v     , color->x, color->y, color->z, color->w };
        R2D_Vertex v4 = { xpos + w, ypos    , u + su, v + sv, color->x, color->y, color->z, color->w };
        R2D_Vertex v5 = { xpos + w, ypos + h, u + su, v     , color->x, color->y, color->z, color->w };
        vertices[(*idx)++] = v0;
        vertices[(*idx)++] = v1;
        vertices[(*idx)++] = v2;
        vertices[(*idx)++] = v3;
        vertices[(*idx)++] = v4;
        vertices[(*idx)++] = v5;
        *x += (g.advance_x >> 6) * scale;

    }

    static void SetVertexBuffer(String s) {
        Uint32 len = SDL_strlen(s);

        Float32 x = 0;
        Float32 y = 0;
        Float32 scale = 1;

        Uint32 idx = 0;

        vec4 color = {1, 1, 1, 1};
        for (Uint32 i = 0; i < len; i++) {
            PushChar(s[i], &color, scale, &idx, &x, &y);
        }
    }

    void InitializeConsole() {
        RegisterEventHandler(Input);
        r_ctx = Render::GetRenderContext();

        font = RG_NEW(Font)("gamedata/fonts/UbuntuMono-R.ttf", 12);

        R2DCreateMemTextureInfo tinfo = {};
        tinfo.data = font->GetBitmap();
        tinfo.type = RG_TEXTURE_U8_R_ONLY;
        tinfo.width  = RG_FONT_ATLAS_WIDTH * 12;
        tinfo.height = RG_FONT_ATLAS_HEIGHT * 12;

        font_tex = r_ctx->R2D_CreateMemTexture(&tinfo);

        R2DCreateBufferInfo binfo = {};

        R2D_Vertex bg_vertices[] = {
            {0, 0, 0, 0, 0.01f, 0.01f, 0.01f, 0.8f},
            {1, 0, 0, 0, 0.01f, 0.01f, 0.01f, 0.8f},
            {1, 1, 0, 0, 0.01f, 0.01f, 0.01f, 0.8f},
            {1, 1, 0, 0, 0.01f, 0.01f, 0.01f, 0.8f},
            {0, 1, 0, 0, 0.01f, 0.01f, 0.01f, 0.8f},
            {0, 0, 0, 0, 0.01f, 0.01f, 0.01f, 0.8f}
        };

        binfo.initial_data = bg_vertices;
        binfo.length = 6;
        buff1 = r_ctx->R2D_CreateBuffer(&binfo);

        SetVertexBuffer("Hello, world!");

        binfo.initial_data = vertices;
        binfo.length = RG_R2D_MAX_VERTICES;
        //binfo.length = RG_COMMAND_LENGTH * 6 * Logger_GetLines();
        buff2 = r_ctx->R2D_CreateBuffer(&binfo);

        //binfo.length = RG_COMMAND_LENGTH * 6;
        //buff3 = r_ctx->R2D_CreateBuffer(&binfo);
    }

    void DestroyConsole() {
        FreeEventHandler(Input);

        RG_DELETE(Font, font);

        r_ctx->R2D_DestroyBuffer(buff1);
        r_ctx->R2D_DestroyBuffer(buff2);
        //r_ctx->R2D_DestroyBuffer(buff3);
    }

    void UpdateConsole() {
        if (!is_shown) { return; }

        GetWindowSize(&scr_size);

        r_ctx->R2D_ResetStack();

        mat4 mat = {};
        mat4_ortho(&mat, 0, (Float32)scr_size.x, (Float32)scr_size.y, 0, -1, 1);
        r_ctx->R2D_PushMatrix(&mat);

        mat4_model(&mat, { 0, 0, 0 }, { 0, 0, 0 }, { (Float32)scr_size.x, 350, 1 });
        r_ctx->R2D_PushMatrix(&mat);

        R2DDrawInfo dinfo = {};
        R2DBindInfo binfo = {};

        binfo.buffer  = buff1;
        binfo.texture = NULL;
        binfo.color   = { 1.0f, 1.0f, 1.0f, 1.0f };
        dinfo.offset  = 0;
        dinfo.count   = 6;

        r_ctx->R2D_Bind(&binfo);
        r_ctx->R2D_Draw(&dinfo);

        r_ctx->R2D_PopMatrix();

        mat4_model(&mat, { 25, 25, 0 }, { 0, 0, 0 }, { 1, 1, 1 });
        r_ctx->R2D_PushMatrix(&mat);

        binfo.buffer = buff2;
        binfo.texture = font_tex;
        dinfo.offset = 0;
        dinfo.count  = 78;

        r_ctx->R2D_Bind(&binfo);
        r_ctx->R2D_Draw(&dinfo);
    }

    void ShowConsole() { is_shown = true; }
    void HideConsole() { is_shown = false; }
    void ToggleConsole() { is_shown = !is_shown; }
    Bool IsConsoleShown() { return is_shown; }

}