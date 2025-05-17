#include "vertexbuffer.h"
#include "glad.h"
#include "renderer.h"

#define VERTEXBUFFER_MATERIALS 512

typedef struct IndexPair {
	Uint32 start;
	Uint32 count;
} IndexPair;

typedef struct VertexBuffer {
	Bool      isLoaded;
	GLuint    vbo;
	GLuint    ebo;
	GLuint    vao;
	Uint32    meshes;
	Uint32    tcount;
	Uint32    indexsize;
	Uint32    mat[VERTEXBUFFER_MATERIALS]; // material index
	IndexPair pairs[VERTEXBUFFER_MATERIALS];
	Texture*  textures[VERTEXBUFFER_MATERIALS*3];
	vec3      colors[VERTEXBUFFER_MATERIALS];
} VertexBuffer;

static VertexBuffer buffer = {};

VertexBuffer* GetVertexbuffer() {
	return &buffer;
}

void MakeVBuffer(R3DStaticModelInfo* info, String mdlpath) {

	glGenVertexArrays(1, &buffer.vao);
	glBindVertexArray(buffer.vao);

	glGenBuffers(1, &buffer.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(R3D_Vertex) * info->vCount, info->vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &buffer.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, info->iType * info->iCount, info->indices, GL_STATIC_DRAW);

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

	buffer.indexsize = info->iType;
	buffer.meshes = info->mCount;
	buffer.tcount = info->matCount;
	for (Uint32 i = 0; i < info->mCount; i++) {
		buffer.mat[i] = info->mInfo[i].materialIdx;
		buffer.pairs[i].start = info->mInfo[i].indexOffset;
		buffer.pairs[i].count = info->mInfo[i].indexCount;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	char alb_path[256];
	char nrm_path[256];
	char pbr_path[256];
	for (Uint32 i = 0; i < info->matCount; i++) {
		SDL_snprintf(alb_path, 256, "%s", info->matInfo[i].texture);
		//SDL_snprintf(alb_path, 256, "gamedata/textures/%s.png", info->matInfo[i].texture);
		//SDL_snprintf(nrm_path, 256, "gamedata/textures/%s_norm.png", info->matInfo[i].texture);
		//SDL_snprintf(pbr_path, 256, "gamedata/textures/%s_pbr.png", info->matInfo[i].texture);
		buffer.textures[i * 3]     = GetTexture(alb_path);
		//buffer.textures[i * 3 + 1] = GetTexture(nrm_path);
		//buffer.textures[i * 3 + 2] = GetTexture(pbr_path);
		buffer.colors[i] = info->matInfo[i].color;
	}

	buffer.isLoaded = true;
}

void FreeVBuffer(VertexBuffer* buffer) {
	if (!buffer->isLoaded) {
		return;
	}

	buffer->isLoaded = false;
	glDeleteVertexArrays(1, &buffer->vao);
	glDeleteBuffers(1, &buffer->vbo);
	glDeleteBuffers(1, &buffer->ebo);
	Texture* t;
	for (Uint32 i = 0; i < buffer->tcount; i++) {
		t = buffer->textures[i * 3];
		if (t) { FreeTexture(t); }
		t = buffer->textures[i * 3 + 1];
		if (t) { FreeTexture(t); }
		t = buffer->textures[i * 3 + 2];
		if (t) { FreeTexture(t); }
	}
}

void DrawBuffer(RenderState* state, VertexBuffer* ptr) {
	if (!ptr->isLoaded) {
		// Do not render (NO MODEL)
		return;
	}

	glBindVertexArray(ptr->vao);

	GLenum vtype = GL_UNSIGNED_INT;
	switch (ptr->indexsize) {
		case 2:  { vtype = GL_UNSIGNED_SHORT; break; }
		case 4:  { vtype = GL_UNSIGNED_INT; break; }
		default: { vtype = GL_UNSIGNED_INT; break; }
	}

	for (Uint32 i = 0; i < ptr->meshes; i++) {

		Uint32 txidx = ptr->mat[i];

		SetMaterialColor(state, ptr->colors[txidx]);

		glActiveTexture(GL_TEXTURE0);
		if(ptr->textures[txidx * 3]) { glBindTexture(GL_TEXTURE_2D, ptr->textures[txidx * 3]->tex_id); }
		else { glBindTexture(GL_TEXTURE_2D, 0); }

		glActiveTexture(GL_TEXTURE1);
		if (ptr->textures[txidx * 3 + 1]) { glBindTexture(GL_TEXTURE_2D, ptr->textures[txidx * 3 + 1]->tex_id); }
		else { glBindTexture(GL_TEXTURE_2D, 0); }

		glActiveTexture(GL_TEXTURE2);
		if (ptr->textures[txidx * 3 + 2]) { glBindTexture(GL_TEXTURE_2D, ptr->textures[txidx * 3 + 2]->tex_id); }
		else { glBindTexture(GL_TEXTURE_2D, 0); }

		glDrawElements(GL_TRIANGLES, ptr->pairs[i].count, vtype, (void*)(ptr->pairs[i].start * ptr->indexsize));
	}
}