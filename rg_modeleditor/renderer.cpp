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

#define RG_WND_ICON "platform/icon.png"

typedef struct RenderState {
	SDL_Window*     hwnd;
	SDL_GLContext   glctx;
	GLuint          shader;
	GuiDrawCallback guicb;
	ivec2           wsize;
	Bool            wireframe;
	Bool            showaxis;
} RenderState;

static RenderState staticstate;

static SDL_Surface* icon_surface;
static Uint8*       icon_data_ptr;

static VertexBuffer* buffer;

static mat4 m4_identity;

static GLuint axis_texture;
static GLuint axis_vao;
static GLuint axis_vbo;
static GLuint axis_ebo;

static Uint32 axis_color[] = {0xFF0000FF, 0xFF00FF00, 0xFFFF0000}; // RGBA (0xAABBGGRR)

static R3D_Vertex axis_vtx[] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f},

	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5f, 0.0f},
	{0, 1, 0, 0, 0, 0, 0, 0, 0, 0.5f, 0.0f},

	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
	{0, 0, 1, 0, 0, 0, 0, 0, 0, 1.0f, 0.0f},
};

static Uint16 axis_idx[] = {0, 1, 2, 3, 4, 5};

static String txt_VertexShader = "#version 330 core\n"
"layout (location = 0) in vec3 v_pos;\n"
"layout (location = 1) in vec3 v_norm;\n"
"layout (location = 2) in vec3 v_tang;\n"
"layout (location = 3) in vec2 v_uv;\n"
"out vec3 o_pos;\n"
"out vec3 o_norm;\n"
"out vec3 o_tang;\n"
"out vec2 o_uv;\n"
"out mat3 o_TBN;\n"
"uniform mat4 proj;\n"
"uniform mat4 view;\n"
"uniform mat4 mdl;\n"
"void main() {\n"
"    mat4 vp = proj * view;\n"
"    vec4 pos4 = vec4(v_pos, 1);\n"
"    vec4 nrm4 = vec4(v_norm, 0);\n"
"    vec4 tan4 = vec4(v_tang, 0);\n"
"    vec3 N = (nrm4 * mdl).xyz;\n"
"    vec3 T = (tan4 * mdl).xyz;\n"
"    vec3 B = normalize(cross(N, T));\n"
"    o_TBN  = mat3(T, B, N);\n"
"    o_pos  = (mdl * pos4).xyz;\n"
"    o_norm = v_norm;\n"
"    o_tang = v_tang;\n"
"    o_uv   = v_uv;\n"
"    gl_Position = vp * vec4(o_pos, 1);\n"
"}\n";

static String txt_PixelShader = "#version 330 core\n"
"in vec3 o_pos;\n"
"in vec3 o_norm;\n"
"in vec3 o_tang;\n"
"in vec2 o_uv;\n"
"in mat3 o_TBN;\n"
"out vec4 p_color;\n"
"uniform sampler2D t_unit0;\n"
"uniform sampler2D t_unit1;\n"
"uniform sampler2D t_unit2;\n"
"uniform vec3 viewpos;\n"
"uniform vec3 mat_color;\n"
"uniform int calclight;\n"
"#define PI 3.14159265359\n"
"float DistributionGGX(vec3 N, vec3 H, float roughness) {\n"
"    float a = roughness * roughness;\n"
"    float a2 = a * a;\n"
"    float NdotH = max(dot(N, H), 0.0);\n"
"    float NdotH2 = NdotH * NdotH;\n"
"    float denom = (NdotH2 * (a2 - 1.0) + 1.0);\n"
"    return a2 / (PI * denom * denom);\n"
"}\n"
"float GeometrySchlickGGX(float NdotV, float roughness) {\n"
"    float r = (roughness + 1.0);\n"
"    float k = (r * r) / 8.0;\n"
"    return NdotV / (NdotV * (1.0 - k) + k);\n"
"}\n"
"float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {\n"
"    float NdotV = max(dot(N, V), 0.0);\n"
"    float NdotL = max(dot(N, L), 0.0);\n"
"    float ggx2 = GeometrySchlickGGX(NdotV, roughness);\n"
"    float ggx1 = GeometrySchlickGGX(NdotL, roughness);\n"
"    return ggx1 * ggx2;\n"
"}\n"
"vec3 FresnelSchlick(float cosTheta, vec3 F0) {\n"
"    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);\n"
"}\n"
"\n"
// N - normal; V - Viewdir; P - World-space position; R - Roughness; M - metallic; A - albedo
"vec3 CalculateLight(vec3 N, vec3 V, vec3 P, float M, float R, vec3 A) {\n"
"    vec3 L = normalize(vec3(0, 0.2, 1.8));\n"
"    vec3 C = vec3(1, 0.9, 0.8) * 1.8;\n"
"    vec3 Lo = vec3(0.0);\n"
"    vec3 H = normalize(V + L);\n"
// Do not calculate attenuation
"    vec3 radiance = C;\n"
"    float NDF = DistributionGGX(N, H, R);\n"
"    float G = GeometrySmith(N, V, L, R);\n"
"    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), Lo);\n"
"    vec3 kS = F;\n"
"    vec3 kD = vec3(1.0) - kS;\n"
"    kD *= 1.0 - M;\n"
"    vec3 numerator = NDF * G * F;\n"
"    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;\n"
"    vec3 specular = numerator / denominator;\n"
"    float NdotL = max(dot(N, L), 0.0);\n"
"    Lo += (kD * A / PI + specular) * radiance * NdotL;\n"
"    return Lo;\n"
"}\n"
"\n"
"void main() {\n"
"    vec4 alb = texture(t_unit0, o_uv);\n"
"    vec4 nrm = texture(t_unit1, o_uv);\n"
"    vec4 pbr = texture(t_unit2, o_uv);\n"
"    vec3 N;\n"
"#if 0\n"
"    N = normalize(nrm.xyz * 2.0 - 1.0);\n"
"    N = normalize(N * o_TBN);\n"
"#else\n"
"    N = normalize(o_norm);\n" // Disable normal mappings
"#endif\n"
"    vec3 V = normalize(viewpos - o_pos);\n"
"    vec3 light = vec3(1);\n"
"    if(calclight > 0) {\n"
"        light = vec3(0.3);\n"
"        light += CalculateLight(N, V, o_pos, pbr.x, pbr.y, alb.xyz);\n"
"    }\n"
"    p_color = vec4(alb.xyz * mat_color * light, 1);\n"
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
	staticstate.showaxis = 1;

	m4_identity = MAT4_IDENTITY();

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

	SDL_GetWindowSize(staticstate.hwnd, &staticstate.wsize.x, &staticstate.wsize.y);
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

	glGenTextures(1, &axis_texture);
	glBindTexture(GL_TEXTURE_2D, axis_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 3, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, axis_color);
	glGenerateMipmap(GL_TEXTURE_2D);

	glGenVertexArrays(1, &axis_vao);
	glBindVertexArray(axis_vao);
	glGenBuffers(1, &axis_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, axis_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axis_vtx), axis_vtx, GL_STATIC_DRAW);
	glGenBuffers(1, &axis_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axis_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(axis_idx), axis_idx, GL_STATIC_DRAW);

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

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// ImGUI
	ImGui_ImplSDL2_InitForOpenGL(staticstate.hwnd, staticstate.glctx);
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
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	glDeleteProgram(state->shader);

	FreeVBuffer(buffer);

	DestroyTextures();

	SDL_GL_DeleteContext(state->glctx);
	SDL_DestroyWindow(state->hwnd);
}

static void DrawAxis() {
	glBindVertexArray(axis_vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, axis_texture);
	glDrawElements(GL_LINES, 6, GL_UNSIGNED_SHORT, 0);
}

void DoRender(RenderState* state, Engine::Camera* camera) {

	mat4 proj = *camera->GetProjection();
	mat4 view = *camera->GetView();
	mat4 model;
	mat4_translate(&model, {0, 0, 0});

	vec3 pos = camera->GetTransform()->GetPosition();

	glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (state->wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glUseProgram(state->shader);

	// VS uniforms
	glUniformMatrix4fv(glGetUniformLocation(state->shader, "proj"), 1, GL_FALSE, proj.m);
	glUniformMatrix4fv(glGetUniformLocation(state->shader, "view"), 1, GL_FALSE, view.m);
	glUniformMatrix4fv(glGetUniformLocation(state->shader, "mdl"), 1, GL_FALSE, model.m);

	// PS uniforms
	glUniform1i(glGetUniformLocation(state->shader, "t_unit0"), 0);   // albedo
	glUniform1i(glGetUniformLocation(state->shader, "t_unit1"), 1);   // normal
	glUniform1i(glGetUniformLocation(state->shader, "t_unit2"), 2);   // pbr
	glUniform3f(glGetUniformLocation(state->shader, "mat_color"), 1, 1, 1);
	glUniform3f(glGetUniformLocation(state->shader, "viewpos"), pos.x, pos.y, pos.z);
	glUniform1i(glGetUniformLocation(state->shader, "calclight"), 1); // Do light calculation

	//rgLogInfo(RG_LOG_RENDER, "Camera: %f %f %f", pos.x, pos.y, pos.z);

	DrawBuffer(state, buffer);

	glUniformMatrix4fv(glGetUniformLocation(state->shader, "mdl"), 1, GL_FALSE, m4_identity.m);
	glUniform3f(glGetUniformLocation(state->shader, "mat_color"), 1, 1, 1);
	glUniform1i(glGetUniformLocation(state->shader, "calclight"), 0);

	if (state->showaxis) {
		DrawAxis();
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
	ImGui_ImplSDL2_NewFrame();
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

Bool* GetRenderWireframe(RenderState* state) {
	return &state->wireframe;
}

void SetMaterialColor(RenderState* state, const vec3& color) {
	glUniform3f(glGetUniformLocation(state->shader, "mat_color"), color.r, color.g, color.b);
}