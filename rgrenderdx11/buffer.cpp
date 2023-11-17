#include "buffer.h"
#include "dx11.h"
#include <engine.h>

static Uint64 buffersLength = 0;

Buffer::Buffer(BufferCreateInfo* info) {
    this->access    = info->access;
    this->bufferLen = info->length;
    D3D11_BUFFER_DESC buff = {};
    buff.Usage          = (D3D11_USAGE)info->usage;
    buff.ByteWidth      = info->length;
    buff.BindFlags      = info->type;
    buff.CPUAccessFlags = info->access;
    buff.MiscFlags      = info->miscflags;
    buff.StructureByteStride = info->stride;
    HRESULT result = DX11_GetDevice()->CreateBuffer(&buff, NULL, &this->buffer);
    RG_ASSERT_MSG(this->buffer, "Unable to create buffer: D3D11Buffer");
    buffersLength += this->bufferLen;
}

Buffer::~Buffer() {
    this->buffer->Release();
    buffersLength -= this->bufferLen;
}

void Buffer::SetData(Uint32 offset, Uint32 length, void* data) {
    if (this->access == BUFFER_GPU_ONLY) {
        // Ignore offset and length arguments
        DX11_GetContext()->UpdateSubresource(this->buffer, 0, NULL, data, 0, 0);
    } else {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        DX11_GetContext()->Map(this->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        char* mdata = (char*)mappedResource.pData;
        SDL_memcpy(&mdata[offset], data, length);
        DX11_GetContext()->Unmap(this->buffer, 0);
    }
}

Uint64 GetBufferMemory() {
    return buffersLength;
}