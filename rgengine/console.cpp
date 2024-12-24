#define DLL_EXPORT
#include "console.h"

#include "rendertypes.h"
#include "utf8.h"

#include "logger.h"
#include "event.h"

#include "render.h"
#include "window.h"

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

    static R2D_Buffer* buff1; // bg
    static R2D_Buffer* buff2; // log console
    static R2D_Buffer* buff3; // command line

    static ivec2 scr_size = { 0, 0 };


    static Bool Input(SDL_Event* event) {

        return true;
    }

    void InitializeConsole() {
        RegisterEventHandler(Input);
        r_ctx = Render::GetRenderContext();

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

        //binfo.initial_data = NULL;
        //binfo.length = RG_COMMAND_LENGTH * 6 * Logger_GetLines();
        //buff2 = r_ctx->R2D_CreateBuffer(&binfo);

        //binfo.length = RG_COMMAND_LENGTH * 6;
        //buff3 = r_ctx->R2D_CreateBuffer(&binfo);
    }

    void DestroyConsole() {
        FreeEventHandler(Input);

        r_ctx->R2D_DestroyBuffer(buff1);
        //r_ctx->R2D_DestroyBuffer(buff2);
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

        R2DBindInfo binfo = {};
        binfo.buffer  = buff1;
        binfo.texture = NULL;
        binfo.color   = { 1.0f, 1.0f, 1.0f, 1.0f };
        r_ctx->R2D_Bind(&binfo);

        R2DDrawInfo dinfo = {};
        dinfo.offset = 0;
        dinfo.count  = 6;
        r_ctx->R2D_Draw(&dinfo);
    }

    void ShowConsole() { is_shown = true; }
    void HideConsole() { is_shown = false; }
    void ToggleConsole() { is_shown = !is_shown; }
    Bool IsConsoleShown() { return is_shown; }

}