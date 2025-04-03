#include "renderer.h"

#include <SDL2/SDL.h>
#include <engine.h>
#include <rgstb.h>
#include "texture.h"
#include "vertexbuffer.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>
#include "imgui_impl_opengl3.h"

#include "glad.h"

#include <objimporter.h>

#define RG_WND_ICON "platform/icon.png"

typedef struct RenderState {
	SDL_Window*     hwnd;
	SDL_GLContext   glctx;
	GLuint          shader;
	GuiDrawCallback guicb;

} RenderState;

static RenderState staticstate;

static SDL_Surface* icon_surface;
static Uint8*       icon_data_ptr;

static Engine::ObjImporter obj_importer;

static VertexBuffer* buffer;

static String txt_VertexShader = "#version 330 core\n"
"layout (location = 0) in vec3 v_pos;\n"
"layout (location = 1) in vec3 v_norm;\n"
"layout (location = 2) in vec3 v_tang;\n"
"layout (location = 3) in vec2 v_uv;\n"
"out vec3 o_norm;\n"
"out vec3 o_tang;\n"
"out vec2 o_uv;\n"
"uniform mat4 proj;\n"
"uniform mat4 view;\n"
"uniform mat4 mdl;\n"
"void main() {\n"
"    mat4 mvp = proj * view * mdl;\n"
"    gl_Position = mvp * vec4(v_pos, 1);\n"
"    o_norm = v_norm;\n"
"    o_tang = v_tang;\n"
"    o_uv   = v_uv;\n"
"}\n";

static String txt_PixelShader = "#version 330 core\n"
"in vec3 o_norm;\n"
"in vec3 o_tang;\n"
"in vec2 o_uv;\n"
"out vec4 color;\n"
"uniform sampler2D t_unit0;\n"
"void main() {\n"
"    color = texture(t_unit0, o_uv);\n"
"}\n";

// OpenGL 4.3
#if 0
void APIENTRY openglDebugCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length,
	const GLchar* message, const void* userParam) {
	RG_ERROR_MSG("[OpenGL] %s", message);
}
#endif

RenderState* InitializeRenderer(GuiDrawCallback guicb) {

	SDL_memset(&staticstate, 0, sizeof(RenderState));
	staticstate.guicb = guicb;

	// Load icon
	int w, h, c;
	icon_data_ptr = RG_STB_load_from_file(RG_WND_ICON, &w, &h, &c, 4);
	icon_surface = SDL_CreateRGBSurfaceFrom(icon_data_ptr, w, h, 32, 4 * w, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

	// Setup OpenGL attribs
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// Window
	staticstate.hwnd = SDL_CreateWindow("rgEngine - OpenGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!staticstate.hwnd) {
		RG_ERROR_MSG("Failed to create window!");
	}

	SDL_SetWindowIcon(staticstate.hwnd, icon_surface);
	SDL_SetWindowResizable(staticstate.hwnd, SDL_TRUE);

	// Load GL
	staticstate.glctx = SDL_GL_CreateContext(staticstate.hwnd);
	if (!staticstate.glctx) {
		RG_ERROR_MSG("Failed to create OpenGL context!");
	}

	gladLoadGL();
	glEnable(GL_DEPTH_TEST);

	//glEnable(GL_DEBUG_OUTPUT);
	//glDebugMessageCallback(openglDebugCallback, nullptr);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// ImGUI
	ImGui_ImplSDL2_InitForOpenGL(staticstate.hwnd, staticstate.glctx);
	ImGui_ImplOpenGL3_Init("#version 130");

	InitializeTextures();

	//Load shaders

	GLint success;
	GLchar infoLog[1024];

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &txt_VertexShader, NULL);
	glCompileShader(vertexShader);

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 1024, NULL, infoLog);
		rgLogError(RG_LOG_RENDER, "Vertex shader error: %s", infoLog);
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &txt_PixelShader, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 1024, NULL, infoLog);
		rgLogError(RG_LOG_RENDER, "Pixel shader error: %s", infoLog);
	}

	staticstate.shader = glCreateProgram();
	glAttachShader(staticstate.shader, vertexShader);
	glAttachShader(staticstate.shader, fragmentShader);
	glLinkProgram(staticstate.shader);

	glGetProgramiv(staticstate.shader, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(staticstate.shader, 1024, NULL, infoLog);
		rgLogError(RG_LOG_RENDER, "Linking error: %s", infoLog);
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	R3DStaticModelInfo info = {};
	obj_importer.ImportModel("gamedata/models/untitled2.obj", &info);
	//obj_importer.ImportModel("gamedata/models/doublesided_cape.obj", &info);
	//rgLogInfo(RG_LOG_RENDER, "Loaded model: %d %d %d %d", info.vCount, info.iCount, info.mCount, info.iType);
	buffer = MakeVBuffer(&info);

	return &staticstate;
}

void DestroyRenderer(RenderState* state) {

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	glDeleteProgram(state->shader);

	FreeVBuffer(buffer);

	DestroyTextures();

	SDL_GL_DeleteContext(state->glctx);
	SDL_DestroyWindow(state->hwnd);
}

void DoRender(RenderState* state, Engine::Camera* camera) {

	mat4 proj = *camera->GetProjection();
	mat4 view = *camera->GetView();
	mat4 model;
	mat4_translate(&model, {0, 0, 0});

	glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(state->shader);

	glUniformMatrix4fv(glGetUniformLocation(state->shader, "proj"), 1, GL_FALSE, proj.m);
	glUniformMatrix4fv(glGetUniformLocation(state->shader, "view"), 1, GL_FALSE, view.m);
	glUniformMatrix4fv(glGetUniformLocation(state->shader, "mdl"), 1, GL_FALSE, model.m);

	DrawBuffer(buffer);

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		switch (err) {
			case GL_INVALID_ENUM:      rgLogError(RG_LOG_RENDER, "GL_INVALID_ENUM"); break;
			case GL_INVALID_VALUE:     rgLogError(RG_LOG_RENDER, "GL_INVALID_VALUE"); break;
			case GL_INVALID_OPERATION: rgLogError(RG_LOG_RENDER, "GL_INVALID_OPERATION"); break;
			//case GL_STACK_OVERFLOW:  rgLogError(RG_LOG_RENDER, "GL_STACK_OVERFLOW"); break;
			//case GL_STACK_UNDERFLOW: rgLogError(RG_LOG_RENDER, "GL_STACK_UNDERFLOW"); break;
			case GL_OUT_OF_MEMORY:     rgLogError(RG_LOG_RENDER, "GL_OUT_OF_MEMORY"); break;
			default:                   rgLogError(RG_LOG_RENDER, "Unknown error!");
		}
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	staticstate.guicb();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	SDL_GL_SwapWindow(state->hwnd);
}