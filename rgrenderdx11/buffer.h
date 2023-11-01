#ifndef BUFFER_H
#define BUFFER_H

#include <d3d11.h>
#include <rgtypes.h>

enum BufferUsage {
	BUFFER_DEFAULT = D3D11_USAGE_DEFAULT, // GPU read/write
	BUFFER_DYNAMIC = D3D11_USAGE_DYNAMIC, // GPU read, CPU write
};

enum BufferType {
	BUFFER_NULL      = 0x0L,
	BUFFER_VERTEX    = D3D11_BIND_VERTEX_BUFFER,
	BUFFER_INDEX     = D3D11_BIND_INDEX_BUFFER,
	BUFFER_CONSTANT  = D3D11_BIND_CONSTANT_BUFFER,
	BUFFER_RESOURCE  = D3D11_BIND_SHADER_RESOURCE,
	BUFFER_UNORDERED = D3D11_BIND_UNORDERED_ACCESS
};

enum BufferAccess {
	BUFFER_GPU_ONLY  = 0,
	BUFFER_CPU_WRITE = D3D11_CPU_ACCESS_WRITE,
	BUFFER_CPU_READ  = D3D11_CPU_ACCESS_READ
};

struct BufferCreateInfo {
	Uint32		 length;
	UINT         type;
	BufferUsage  usage;
	BufferAccess access;
	Uint32       miscflags;
	Uint32       stride;
};

class Buffer {
	public:
		Buffer(BufferCreateInfo* info);
		virtual ~Buffer();

		void SetData(Uint32 offset, Uint32 length, void* data);
		ID3D11Buffer* GetHandle() { return this->buffer; }

	private:
		ID3D11Buffer* buffer = NULL;
		BufferAccess  access = BUFFER_GPU_ONLY;
};

#endif