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

#define RG_COMMAND_LENGTH 1024
#define RG_R2D_MAX_VERTICES 16384

#define FONT_SIZE 16

namespace Engine {

    static Bool         is_shown = false;

    static Uint16       command_buffer[RG_COMMAND_LENGTH];
    static Uint32       cursor = 0;

    static R2D_Vertex   vertices[RG_R2D_MAX_VERTICES];
    static Uint32       txt_vertices = 0;
    static Bool         bufferUpdateRequest = true;

    static UTF8Decoder  utf8_decoder;
    static UTF8Encoder  utf8_encoder;

    static Renderer*    r_ctx;

    static R2D_Texture* font_tex;

    static R2D_Buffer*  buff1; // bg
    static R2D_Buffer*  buff2; // text

    static ivec2 scr_size = { 0, 0 };

    static Font* font;

    static vec4 colors[] = {
        {1, 0, 0, 1},
        {1, 0.3, 0.3, 1},
        {0.9, 0.8, 0, 1},
        {0.2, 1, 0.4, 1},
        {0.3, 0.4, 0.9, 1},
        {1, 1, 1, 1}
    };

    static vec4* GetColor(SDL_LogPriority p) {
        switch (p) {
            case SDL_LOG_PRIORITY_VERBOSE:  return &colors[4];
            case SDL_LOG_PRIORITY_DEBUG:    return &colors[5];
            case SDL_LOG_PRIORITY_INFO:     return &colors[3];
            case SDL_LOG_PRIORITY_WARN:     return &colors[2];
            case SDL_LOG_PRIORITY_ERROR:    return &colors[1];
            case SDL_LOG_PRIORITY_CRITICAL: return &colors[0];
            default:                        return &colors[4];
        }
    }

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
        R2D_Vertex v5 = { xpos + w, ypos + h, u + su, v     , color->x, color->y, color->z, color->w };
        vertices[(*idx)++] = v0;
        vertices[(*idx)++] = v1;
        vertices[(*idx)++] = v2;
        vertices[(*idx)++] = v0;
        vertices[(*idx)++] = v2;
        vertices[(*idx)++] = v5;
        *x += (g.advance_x >> 6) * scale;

    }

    static void AppendBuffer(String s, Uint32* idx, Float32* x, Float32* y, vec4* color) {
        Uint32 len = SDL_strlen(s);

        Float32 scale = 1;

        for (Uint32 i = 0; i < len; i++) {
            char c = s[i];
            if (c == '\n') {
                continue;
            }
            PushChar(c, color, scale, idx, x, y);
        }
    }

    static void UpdateVertexBuffer() {

        Float32 x = 0;
        Float32 y = 0;

        // Update local buffer
        txt_vertices = 0;

        Uint32 lines = Logger_GetLines();
        for (Sint32 i = lines; i >= 0; i--) {
            String s = Logger_GetLine(i);
            vec4* color = GetColor(Logger_GetLinePriority(i));
            AppendBuffer(s, &txt_vertices, &x, &y, color);
            x = 0;
            y -= FONT_SIZE;
        }

        // Command line
        vec4 wcolor = { 1, 1, 1, 1 };
        vec4 ycolor = { 1, 0.75f, 0.27f, 1 };
        AppendBuffer("> ", &txt_vertices, &x, &y, &wcolor);
        //AppendBuffer(s, &txt_vertices, &x, &y);

        x = scr_size.x - 150;
        char msgs_buffer[64];
        SDL_snprintf(msgs_buffer, 64, "[%d]", Logger_GetMessages());
        AppendBuffer(msgs_buffer, &txt_vertices, &x, &y, &ycolor);

        // Update GPU buffer
        R2DBufferDataInfo info = {};
        info.buffer = buff2;
        info.offset = 0;
        info.length = txt_vertices;
        info.data   = vertices;
        r_ctx->R2D_BufferData(&info);

    }

    void InitializeConsole() {
        RegisterEventHandler(Input);
        r_ctx = Render::GetRenderContext();

        font = RG_NEW(Font)("gamedata/fonts/UbuntuMono-R.ttf", FONT_SIZE);

        R2DCreateMemTextureInfo tinfo = {};
        tinfo.data = font->GetBitmap();
        tinfo.type = RG_TEXTURE_U8_RGBA;
        tinfo.width  = RG_FONT_ATLAS_WIDTH * FONT_SIZE;
        tinfo.height = RG_FONT_ATLAS_HEIGHT * FONT_SIZE;

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

        binfo.initial_data = vertices;
        binfo.length = RG_R2D_MAX_VERTICES;
        buff2 = r_ctx->R2D_CreateBuffer(&binfo);

    }

    void DestroyConsole() {
        FreeEventHandler(Input);

        RG_DELETE(Font, font);

        r_ctx->R2D_DestroyBuffer(buff1);
        r_ctx->R2D_DestroyBuffer(buff2);
    }

    void UpdateConsole() {
        if (!is_shown) { return; }

        // Update data if needed
        if (bufferUpdateRequest) {
            UpdateVertexBuffer();
            //bufferUpdateRequest = false;
        }

        GetWindowSize(&scr_size);

        r_ctx->R2D_ResetStack();

        // Set projection mode
        mat4 mat = {};
        mat4_ortho(&mat, 0, (Float32)scr_size.x, 0, (Float32)scr_size.y, -1, 1);
        r_ctx->R2D_PushMatrix(&mat);

        // Draw background
        Float32 c_size = (Logger_GetLines() + 2) * FONT_SIZE;
        mat4_model(&mat, { 0, (Float32)(scr_size.y - c_size), 0 }, { 0, 0, 0 }, { (Float32)scr_size.x, c_size, 1 });
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

        // Draw text
        mat4_model(&mat, { 5, (Float32)(scr_size.y - 5), 0 }, { 0, 0, 0 }, { 1, 1, 1 });
        r_ctx->R2D_PushMatrix(&mat);

        binfo.buffer  = buff2;
        binfo.texture = font_tex;
        dinfo.offset  = 0;
        dinfo.count   = txt_vertices;

        r_ctx->R2D_Bind(&binfo);
        r_ctx->R2D_Draw(&dinfo);
    }

    void ShowConsole() { is_shown = true; }
    void HideConsole() { is_shown = false; }
    void ToggleConsole() { is_shown = !is_shown; }
    Bool IsConsoleShown() { return is_shown; }

}