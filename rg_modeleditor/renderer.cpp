#include "renderer.h"

//#include <window.h>

#include <SDL2/SDL.h>
//#include <SDL2/SDL_opengl.h>
#include <engine.h>
#include <rgstb.h>

#include "glad.h"

#define RG_WND_ICON "platform/icon.png"

typedef struct RenderState {
	SDL_Window*   hwnd;
	SDL_GLContext glctx;
	GLuint        shader;
} RenderState;

static RenderState staticstate;

static SDL_Surface* icon_surface;
static Uint8*       icon_data_ptr;

static GLuint VAO;
static GLuint VBO;
static GLuint EBO;
static GLuint texture0;
static GLuint texture1;

static String txt_VertexShader = "#version 330 core\n"
"layout (location = 0) in vec3 v_pos;\n"
"layout (location = 1) in vec3 v_color;\n"
"layout (location = 2) in vec2 v_uv;\n"
"out vec3 o_color;\n"
"out vec2 o_uv;\n"
"void main() {\n"
"    gl_Position = vec4(v_pos, 1);\n"
"    o_color     = v_color;\n"
"    o_uv        = v_uv;\n"
"}\n";

static String txt_PixelShader = "#version 330 core\n"
"in vec3 o_color;\n"
"in vec2 o_uv;\n"
"out vec4 color;\n"
"uniform sampler2D t_unit0;\n"
"void main() {\n"
"    color = texture(t_unit0, o_uv) * vec4(o_color, 1);\n"
"}\n";

// OpenGL 4.3
#if 0
void APIENTRY openglDebugCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length,
	const GLchar* message, const void* userParam) {
	RG_ERROR_MSG("[OpenGL] %s", message);
}
#endif

RenderState* InitializeRenderer() {

	SDL_memset(&staticstate, 0, sizeof(RenderState));

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

	// Load GL
	staticstate.glctx = SDL_GL_CreateContext(staticstate.hwnd);
	if (!staticstate.glctx) {
		RG_ERROR_MSG("Failed to create OpenGL context!");
	}

	gladLoadGL();

	//glEnable(GL_DEBUG_OUTPUT);
	//glDebugMessageCallback(openglDebugCallback, nullptr);

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


	float vertices[] = {
	//  x, y, z              r, g, b           u, v
		-0.8f,  0.8f, 0.0f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
		-0.8f, -0.8f, 0.0f,  1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
		-0.1f, -0.8f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		-0.1f,  0.8f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f, 0.0f,

		 0.1f,  0.8f, 0.0f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
		 0.1f, -0.8f, 0.0f,  1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
		 0.8f, -0.8f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 0.8f,  0.8f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f, 0.0f
	};

	unsigned int indices[] = {
		0, 1, 2,
		0, 2, 3,

		4, 5, 6,
		4, 6, 7
	};

	// Vertex buffer
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Texture
	glGenTextures(1, &texture0);
	glBindTexture(GL_TEXTURE_2D, texture0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	Uint8* data = RG_STB_load_from_file("platform/textures/128.png", &width, &height, &nrChannels, 4);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		rgLogError(RG_LOG_RENDER, "Texture loading error!");
	}
	RG_STB_image_free(data);


	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	data = RG_STB_load_from_file("platform/textures/grid.png", &width, &height, &nrChannels, 4);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		rgLogError(RG_LOG_RENDER, "Texture loading error!");
	}
	RG_STB_image_free(data);

	return &staticstate;
}

void DestroyRenderer(RenderState* state) {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(state->shader);

	glDeleteTextures(1, &texture0);
	glDeleteTextures(1, &texture1);

	SDL_GL_DeleteContext(state->glctx);
	SDL_DestroyWindow(state->hwnd);
}

void DoRender(RenderState* state) {
	glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(state->shader);

	glBindVertexArray(VAO);

	glBindTexture(GL_TEXTURE_2D, texture0);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, texture1);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6*sizeof(Uint32)));
	glBindVertexArray(0);

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

	SDL_GL_SwapWindow(state->hwnd);
}