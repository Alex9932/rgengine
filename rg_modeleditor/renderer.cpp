#include "renderer.h"

#include "glad.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl_glext.h>
#include <engine.h>
#include <rgstb.h>
#include "texture.h"
#include "vertexbuffer.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl3.h>
#include "imgui_impl_opengl3.h"

#include <kinematicsmodel.h>

#include "_glshader.h"

#define RG_WND_ICON "platform/icon.png"

#define RG_MAX_BONES 1024

using namespace Engine;

#define FLAG_WIREFRAME   0x00000001
#define FLAG_SKELETON    0x00000002
#define FLAG_SHOWAXIS    0x00000004
#define FLAG_SHOWMESH    0x00000008
#define FLAG_ANIMDISABLE 0x00000010

typedef struct RenderState {
	SDL_Window*      hwnd;
	SDL_GLContext    glctx;
	GLuint           shader;
	GLuint           matrices_ubo;
	GuiDrawCallback  guicb;
	ivec2            wsize;
	Uint32           flags;
//	Bool             wireframe;
//	Bool             skeleton;
//	Bool             showaxis;
//	Bool             showmesh;
//	Bool             animDisable;
	Sint32           meshhilight;
	Sint32           cullmode;
	vec3             mdl_pos;
	vec3             mdl_rot;
	vec3             mdl_scale;
	mat4             shader_matrices[RG_MAX_BONES];
	KinematicsModel* kmodel;
} RenderState;

struct VBuffer {
	R3D_Vertex* vertices;
	Uint16*     indices;
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
};

struct RMakeVBufferInfo {
	VBuffer*    buffer;
	R3D_Vertex* vertices;
	Uint16*     indices;
	size_t      v_len;
	size_t      i_len;
};

struct RUpdateVBufferInfo {
	VBuffer*    buffer;
	R3D_Vertex* v_data;
	size_t      v_start;
	size_t      v_len;
	Uint16*     i_data;
	size_t      i_start;
	size_t      i_len;
};

static RenderState staticstate;

static SDL_Surface* icon_surface;
static Uint8*       icon_data_ptr;

static VertexBuffer* buffer;

static mat4 m4_identity;

static GLuint axis_texture;
static Uint32 axis_color[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000, 0xFFAAAAAA }; // RGBA (0xAABBGGRR)

static R3D_Vertex axis_vtx[] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f},// 0
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f},

	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0.375f, 0.0f},// 2
	{0, 1, 0, 0, 0, 0, 0, 0, 0, 0.375f, 0.0f},

	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0.625f, 0.0f}, //4
	{0, 0, 1, 0, 0, 0, 0, 0, 0, 0.625f, 0.0f},

	{-10, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -9, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -9, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -8, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -8, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -7, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -7, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -6, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -6, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -5, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -5, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -4, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -4, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -3, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -3, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -2, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -2, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -1, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ -1, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  0, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  0, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  1, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  1, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  2, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  2, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  3, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  3, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  4, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  4, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  5, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  5, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  6, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  6, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  7, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  7, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  8, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  8, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  9, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{  9, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f}, //47

	{-10, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0, -10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,  -9, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},//50
	{ 10, 0,  -9, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,  -8, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,  -8, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,  -7, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,  -7, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,  -6, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,  -6, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,  -5, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,  -5, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,  -4, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},//60
	{ 10, 0,  -4, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,  -3, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,  -3, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,  -2, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,  -2, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,  -1, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,  -1, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,   0, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,   0, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,   1, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},//70
	{ 10, 0,   1, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,   2, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,   2, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,   3, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,   3, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,   4, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,   4, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,   5, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,   5, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,   6, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},//80
	{ 10, 0,   6, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,   7, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,   7, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,   8, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,   8, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,   9, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,   9, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{-10, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{ 10, 0,  10, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f}

};
static Uint16     axis_idx[] = {
	0, 1, 2, 3, 4, 5,
	6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
	48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89
};
static VBuffer    axis_buffer;

static R3D_Vertex skel_vtx[4096] = {};
static Uint16     skel_idx[4096] = {};
static VBuffer    skel_buffer;

// OpenGL 4.3
#if 0
void APIENTRY openglDebugCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length,
	const GLchar* message, const void* userParam) {
	RG_ERROR_MSG("[OpenGL] %s", message);
}
#endif


static void RMakeVBuffer(RMakeVBufferInfo* info) {
	glGenVertexArrays(1, &info->buffer->vao);
	glBindVertexArray(info->buffer->vao);
	glGenBuffers(1, &info->buffer->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, info->buffer->vbo);
	glBufferData(GL_ARRAY_BUFFER, info->v_len, info->vertices, GL_STREAM_DRAW);
	glGenBuffers(1, &info->buffer->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, info->buffer->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, info->i_len, info->indices, GL_STREAM_DRAW);

	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// Normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// Tangent
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	// UV
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
	glEnableVertexAttribArray(3);
}

static void RDeleteBuffer(VBuffer* buffer) {
	glDeleteVertexArrays(1, &buffer->vao);
	glDeleteBuffers(1, &buffer->vbo);
	glDeleteBuffers(1, &buffer->ebo);
}

static void RUpdateVBuffer(RUpdateVBufferInfo* info) {
	glBindBuffer(GL_ARRAY_BUFFER, info->buffer->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, info->v_start, info->v_len, info->v_data);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, info->buffer->ebo);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, info->i_start, info->i_len, info->i_data);
}

static void CheckOpenGL() {
	//rgLogInfo(RG_LOG_RENDER, "OpenGL %s", glGetString(GL_VERSION));
	//rgLogInfo(RG_LOG_RENDER, "GLSL %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	int major, minor, profile;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile);
	rgLogInfo(RG_LOG_RENDER, "Created OpenGL context: %d.%d, Profile: %s",
		major, minor,
		(profile == SDL_GL_CONTEXT_PROFILE_CORE) ? "Core" : "Compatibility");
}

RenderState* InitializeRenderer(GuiDrawCallback guicb) {

	SDL_memset(&staticstate, 0, sizeof(RenderState));
	staticstate.guicb = guicb;

	staticstate.flags = FLAG_SHOWAXIS | FLAG_SKELETON | FLAG_SHOWMESH;

	//staticstate.showaxis = 1;
	//staticstate.skeleton = 1;
	//staticstate.showmesh = 1;

	staticstate.meshhilight = -1;

	staticstate.mdl_pos   = { 0, 0, 0 };
	staticstate.mdl_rot   = { 0, 0, 0 };
	staticstate.mdl_scale = { 1, 1, 1 };

	m4_identity = MAT4_IDENTITY();

	// Load icon
	int w, h, c;
	icon_data_ptr = RG_STB_load_from_file(RG_WND_ICON, &w, &h, &c, 4);
	//icon_surface = SDL_CreateRGBSurfaceFrom(icon_data_ptr, w, h, 32, 4 * w, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	icon_surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_ABGR8888, icon_data_ptr, w * 4);

	SDL_Init(SDL_INIT_VIDEO);
	// Setup OpenGL attribs
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#if 1

	// Window
	//staticstate.hwnd = SDL_CreateWindow("rgEngine - OpenGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	//staticstate.hwnd = SDL_CreateWindow("rgEngine - OpenGL", 800, 600, SDL_WINDOW_OPENGL); // Zaebalo 800x600
	staticstate.hwnd = SDL_CreateWindow("rgEngine - OpenGL", 1280, 720, SDL_WINDOW_OPENGL);
	if (!staticstate.hwnd) {
		RG_ERROR_MSG("Failed to create window!");
	}
#else
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "rgEngine - OpenGL");
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 800);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 600);
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);

	staticstate.hwnd = SDL_CreateWindowWithProperties(props);
	if (!staticstate.hwnd) {
		RG_ERROR_MSG("Failed to create window!");
	}
#endif

	// Load GL
	staticstate.glctx = SDL_GL_CreateContext(staticstate.hwnd);
	if (!staticstate.glctx) {
		RG_ERROR_MSG("Failed to create OpenGL context!");
	}
	SDL_GL_MakeCurrent(staticstate.hwnd, staticstate.glctx);

	SDL_SetWindowPosition(staticstate.hwnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_GetWindowSize(staticstate.hwnd, &staticstate.wsize.x, &staticstate.wsize.y);
	SDL_SetWindowIcon(staticstate.hwnd, icon_surface);
	SDL_SetWindowResizable(staticstate.hwnd, true);

	gladLoadGL();

	// Check OpenGL Context
	CheckOpenGL();

	glEnable(GL_DEPTH_TEST);

	rgLogInfo(RG_LOG_RENDER, "Renderer: %s", glGetString(GL_RENDERER));

	//glEnable(GL_DEBUG_OUTPUT);
	//glDebugMessageCallback(openglDebugCallback, nullptr);

	glGenTextures(1, &axis_texture);
	glBindTexture(GL_TEXTURE_2D, axis_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, axis_color);
	glGenerateMipmap(GL_TEXTURE_2D);

	RMakeVBufferInfo mkvbuffinfo = {};
	mkvbuffinfo.buffer   = &axis_buffer;
	mkvbuffinfo.vertices = axis_vtx;
	mkvbuffinfo.indices  = axis_idx;
	mkvbuffinfo.v_len    = sizeof(axis_vtx);
	mkvbuffinfo.i_len    = sizeof(axis_idx);
	RMakeVBuffer(&mkvbuffinfo);

	mkvbuffinfo.buffer   = &skel_buffer;
	mkvbuffinfo.vertices = skel_vtx;
	mkvbuffinfo.indices  = skel_idx;
	mkvbuffinfo.v_len    = sizeof(skel_vtx);
	mkvbuffinfo.i_len    = sizeof(skel_idx);
	RMakeVBuffer(&mkvbuffinfo);

	glGenBuffers(1, &staticstate.matrices_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, staticstate.matrices_ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4) * RG_MAX_BONES, NULL, GL_STREAM_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, staticstate.matrices_ubo);


	for (Uint32 i = 0; i < RG_MAX_BONES; i++) {
		staticstate.shader_matrices[i] = MAT4_IDENTITY();
	}

	glBindBuffer(GL_UNIFORM_BUFFER, staticstate.matrices_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4) * RG_MAX_BONES, staticstate.shader_matrices);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// ImGUI
	ImGui_ImplSDL3_InitForOpenGL(staticstate.hwnd, staticstate.glctx);
	ImGui_ImplOpenGL3_Init();

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

	buffer = GetVertexbuffer();

	return &staticstate;
}

void DestroyRenderer(RenderState* state) {

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	//	ImGui::DestroyContext();

	glDeleteTextures(1, &axis_texture);
	glDeleteBuffers(1, &staticstate.matrices_ubo);
	RDeleteBuffer(&axis_buffer);
	RDeleteBuffer(&skel_buffer);

	glDeleteProgram(state->shader);

	FreeVBuffer(buffer);

	DestroyTextures();

	SDL_GL_DestroyContext(state->glctx);
	SDL_DestroyWindow(state->hwnd);
}

static void DrawAxis() {
	glBindVertexArray(axis_buffer.vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, axis_texture);
	glDrawElements(GL_LINES, 90, GL_UNSIGNED_SHORT, 0);
}

static mat4 bone_mats[1024];

static void DrawSkeleton(RenderState* state) {
	if (!state->kmodel) return;

	//if (buffer->skeleton->bone_count == 0) return;
	Uint32 idx = 0;

	KinematicsModel* mdl = state->kmodel;


	for (Uint32 i = 0; i < mdl->GetBoneCount(); i++) {
		Bone* b = mdl->GetBone(i);

		mat4 parent_t = MAT4_IDENTITY();
		mat4 parent_o = MAT4_IDENTITY();

		mat4 local_t = b->transform;
		mat4 local_o = b->offset;

		if (b->parent != -1) {
			Bone* pb = mdl->GetBone(b->parent);
			parent_t = pb->transform;
			parent_o = pb->offset;
		}

		vec4 pos1 = { 0, 0, 0, 1 };
		
		vec4 bpos = parent_t * pos1;
		vec4 ppos = local_t  * pos1;

		//if (b->parent != -1) {
		//	Bone* pb = mdl->GetBone(b->parent);
		//	bpos = pb->position;
		//}

		//ppos = bpos + b->position;

		skel_vtx[idx * 2 + 0] = {};
		skel_vtx[idx * 2 + 1] = {};
		skel_vtx[idx * 2 + 0].pos.x = ppos.x;
		skel_vtx[idx * 2 + 0].pos.y = ppos.y;
		skel_vtx[idx * 2 + 0].pos.z = ppos.z;
		skel_vtx[idx * 2 + 0].uv.x = 1.0f;
		skel_vtx[idx * 2 + 0].uv.y = 0.0f;
		skel_vtx[idx * 2 + 1].pos.x = bpos.x;
		skel_vtx[idx * 2 + 1].pos.y = bpos.y;
		skel_vtx[idx * 2 + 1].pos.z = bpos.z;
		skel_vtx[idx * 2 + 1].uv.x = 1.0f;
		skel_vtx[idx * 2 + 1].uv.y = 0.0f;
		idx++;
		if (idx >= 2048) break;

	}
#if 0
	for (Uint32 i = 0; i < buffer->skeleton->bone_count; i++) {
		R3D_Bone* bone = &buffer->skeleton->bones[i];
		if (bone->parent_index == -1) continue;
		R3D_Bone* parent = &buffer->skeleton->bones[bone->parent_index];
		vec4 bpos = { 0, 0, 0, 1 };
		vec4 ppos = { 0, 0, 0, 1 };
		mat4 bmat = state->shader_matrices[i];
		mat4 pmat = state->shader_matrices[bone->parent_index];
		bpos = m4_mul_vec4(&bmat, bpos);
		ppos = m4_mul_vec4(&pmat, ppos);
		skel_vtx[idx * 2 + 0].x = ppos.x;
		skel_vtx[idx * 2 + 0].y = ppos.y;
		skel_vtx[idx * 2 + 0].z = ppos.z;
		skel_vtx[idx * 2 + 1].x = bpos.x;
		skel_vtx[idx * 2 + 1].y = bpos.y;
		skel_vtx[idx * 2 + 1].z = bpos.z;
		idx++;
		if (idx >= 2048) break;
	}
#endif

	for (Uint16 i = 0; i < idx * 2; i++) {
		skel_idx[i] = i;
	}

	RUpdateVBufferInfo updbuffinfo = {};
	updbuffinfo.buffer  = &skel_buffer;
	updbuffinfo.v_data  = skel_vtx;
	updbuffinfo.v_start = 0;
	updbuffinfo.v_len   = sizeof(R3D_Vertex) * idx * 2;
	updbuffinfo.i_data  = skel_idx;
	updbuffinfo.i_start = 0;
	updbuffinfo.i_len = sizeof(Uint16) * idx * 2;
	RUpdateVBuffer(&updbuffinfo);


	glBindVertexArray(skel_buffer.vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, axis_texture);
	glDrawElements(GL_LINES, idx * 2, GL_UNSIGNED_SHORT, 0);

}

void CalculateModelMatrix(RenderState* state, mat4* m) {
	mat4_model(m, state->mdl_pos, state->mdl_rot, state->mdl_scale);
}

void DoRender(RenderState* state, Engine::Camera* camera) {

	mat4 proj = *camera->GetProjection();
	mat4 view = *camera->GetView();
	mat4 model;
	CalculateModelMatrix(state, &model);
	//mat4_model(&model, state->mdl_pos, state->mdl_rot, state->mdl_scale);
	//mat4_translate(&model, {0, 0, 0});

	vec3 pos = camera->GetTransform()->GetPosition();

	glClearColor(0.015f, 0.015f, 0.015f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//if (state->wireframe) {
	if (RG_CHECK_FLAG(state->flags, FLAG_WIREFRAME)) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	switch (state->cullmode) {
		case 0: // None
			glDisable(GL_CULL_FACE);
			break;
		case 1: // Back
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			break;
		case 2: // Front
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			break;
		default:
			break;
	}

	glUseProgram(state->shader);

	// VS uniforms
	glUniformMatrix4fv(glGetUniformLocation(state->shader, "proj"), 1, GL_FALSE, proj.m);
	glUniformMatrix4fv(glGetUniformLocation(state->shader, "view"), 1, GL_FALSE, view.m);
	glUniformMatrix4fv(glGetUniformLocation(state->shader, "mdl"), 1, GL_FALSE, model.m);
	glUniform1i(glGetUniformLocation(state->shader, "animDisable"), RG_CHECK_FLAG(state->flags, FLAG_ANIMDISABLE));

	// PS uniforms
	glUniform1i(glGetUniformLocation(state->shader, "t_unit0"), 0);   // albedo
	glUniform1i(glGetUniformLocation(state->shader, "t_unit1"), 1);   // normal
	glUniform1i(glGetUniformLocation(state->shader, "t_unit2"), 2);   // pbr
	glUniform3f(glGetUniformLocation(state->shader, "mat_color"), 1, 1, 1);
	glUniform3f(glGetUniformLocation(state->shader, "viewpos"), pos.x, pos.y, pos.z);
	glUniform1i(glGetUniformLocation(state->shader, "calclight"), 1); // Do light calculation

	//rgLogInfo(RG_LOG_RENDER, "Camera: %f %f %f", pos.x, pos.y, pos.z);
	
	glUniformBlockBinding(state->shader, glGetUniformBlockIndex(state->shader, "BoneMatrices"), 0);

	glBindBuffer(GL_UNIFORM_BUFFER, state->matrices_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4) * RG_MAX_BONES, state->shader_matrices);

	if (RG_CHECK_FLAG(state->flags, FLAG_SHOWMESH)) {
		DrawBuffer(state, buffer);
	}

	glUniformMatrix4fv(glGetUniformLocation(state->shader, "mdl"), 1, GL_FALSE, m4_identity.m);
	glUniform1i(glGetUniformLocation(state->shader, "animDisable"), 1);
	glUniform3f(glGetUniformLocation(state->shader, "mat_color"), 1, 1, 1);
	glUniform1i(glGetUniformLocation(state->shader, "calclight"), 0);
	glUniform1i(glGetUniformLocation(state->shader, "flipuv"), 0);

	if (RG_CHECK_FLAG(state->flags, FLAG_SHOWAXIS)) {
		DrawAxis();
	}

	if (RG_CHECK_FLAG(state->flags, FLAG_SKELETON)) {
		DrawSkeleton(state);
	}

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
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	staticstate.guicb();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	SDL_GL_SwapWindow(state->hwnd);
}

void ResizeRender(RenderState* state) {
	SDL_GetWindowSize(state->hwnd, &state->wsize.x, &state->wsize.y);
	glViewport(0, 0, state->wsize.x, state->wsize.y);
}

void GetRenderSize(RenderState* state, ivec2* dst) {
	*dst = state->wsize;
}

vec3* GetRenderMdlposPtr(RenderState* state)   { return &state->mdl_pos;        }
vec3* GetRenderMdlrotPtr(RenderState* state)   { return &state->mdl_rot;        }
vec3* GetRenderMdlsizePtr(RenderState* state)  { return &state->mdl_scale;      }

mat4* GetRenderBoneMatPtr(RenderState* state)  { return state->shader_matrices; }

Bool GetRenderWireframe(RenderState* state)   { return RG_CHECK_FLAG(state->flags, FLAG_WIREFRAME);  }
Bool GetRenderSkeleton(RenderState* state)    { return RG_CHECK_FLAG(state->flags, FLAG_SKELETON);   }
Bool GetRenderShowmesh(RenderState* state)    { return RG_CHECK_FLAG(state->flags, FLAG_SHOWMESH);   }
Bool GetRenderAnimDisable(RenderState* state) { return RG_CHECK_FLAG(state->flags, FLAG_ANIMDISABLE);}

void SetRenderWireframe(RenderState* state, Bool b) {
	if (b) { state->flags |= FLAG_WIREFRAME; }
	else   { state->flags &= ~FLAG_WIREFRAME; }
}

void SetRenderSkeleton(RenderState* state, Bool b) {
	if (b) { state->flags |= FLAG_SKELETON; }
	else   { state->flags &= ~FLAG_SKELETON; }
}

void SetRenderShowmesh(RenderState* state, Bool b) {
	if (b) { state->flags |= FLAG_SHOWMESH; }
	else   { state->flags &= ~FLAG_SHOWMESH; }
}

void SetRenderAnimDisable(RenderState* state, Bool b) {
	if (b) { state->flags |= FLAG_ANIMDISABLE; }
	else   { state->flags &= ~FLAG_ANIMDISABLE; }
}

void SetRenderKModel(RenderState* state, KinematicsModel* mdl) { state->kmodel = mdl; }

void SetRenderMeshHilight(RenderState* state, Sint32 meshid) { state->meshhilight = meshid; }
void SetRenderCullMode(RenderState* state, Sint32 mode) { state->cullmode = mode; }

void SetMaterialState(RenderState* state, VertexBuffer* vb, Uint32 mat, Uint32 meshid) {
	vec3 color = vb->colors[mat];

	// Flags
	glUniform1i(glGetUniformLocation(state->shader, "flipuv"), vb->pairs->flipuv);

	// Set material color
	glUniform3f(glGetUniformLocation(state->shader, "mat_color"), color.r, color.g, color.b);

	if (state->meshhilight == meshid) {
		glUniform3f(glGetUniformLocation(state->shader, "mat_color"), 2.8, 2.4, 0.4);
	}

	// Bind textures
	glActiveTexture(GL_TEXTURE0);
	if (vb->textures[mat * 3]) { glBindTexture(GL_TEXTURE_2D, vb->textures[mat * 3]->tex_id); }
	else { glBindTexture(GL_TEXTURE_2D, 0); }

	glActiveTexture(GL_TEXTURE1);
	if (vb->textures[mat * 3 + 1]) { glBindTexture(GL_TEXTURE_2D, vb->textures[mat * 3 + 1]->tex_id); }
	else { glBindTexture(GL_TEXTURE_2D, 0); }

	glActiveTexture(GL_TEXTURE2);
	if (vb->textures[mat * 3 + 2]) { glBindTexture(GL_TEXTURE_2D, vb->textures[mat * 3 + 2]->tex_id); }
	else { glBindTexture(GL_TEXTURE_2D, 0); }
}