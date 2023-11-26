#ifndef _SHADER_H
#define _SHADER_H

#include <d3d11.h>
#include <rgtypes.h>
#include <rgvector.h>
#include "dx11.h"

struct ShaderCode {
    void* data;
    int   len;
};

enum InputDataFormat {
    INPUT_R32_FLOAT          = DXGI_FORMAT_R32_FLOAT,
    INPUT_R32G32_FLOAT       = DXGI_FORMAT_R32G32_FLOAT,
    INPUT_R32G32B32_FLOAT    = DXGI_FORMAT_R32G32B32_FLOAT,
    INPUT_R32G32B32A32_FLOAT = DXGI_FORMAT_R32G32B32A32_FLOAT,
    INPUT_R32_UINT           = DXGI_FORMAT_R32_UINT,
    INPUT_R32G32_UINT        = DXGI_FORMAT_R32G32_UINT,
    INPUT_R32G32B32_UINT     = DXGI_FORMAT_R32G32B32_UINT,
    INPUT_R32G32B32A32_UINT  = DXGI_FORMAT_R32G32B32A32_UINT,
};

struct InputDescription {
    String name;
    Uint32 inputSlot;
    InputDataFormat format;
};

struct SamplerDescription {
    D3D11_TEXTURE_ADDRESS_MODE u;
    D3D11_TEXTURE_ADDRESS_MODE v;
    D3D11_TEXTURE_ADDRESS_MODE w;
};

struct PipelineDescription {
    Uint32              inputCount;
    InputDescription*   descriptions;
    SamplerDescription* sampler;
};

class Shader {
    public:
        Shader(PipelineDescription* description, String vs, String ps, bool compiled);
        ~Shader();
        void Bind();

    private:
        ShaderCode LoadShaders(String vs, String ps);
        ShaderCode LoadCompiledShaders(String vs, String ps);

        ID3D11VertexShader* vshader     = NULL;
        ID3D11PixelShader*  pshader     = NULL;
        ID3D11InputLayout*  layout      = NULL;
        ID3D11SamplerState* sampleState = NULL;

};

class ComputeShader {
    public:
        ComputeShader(String cs, bool compiled);
        ~ComputeShader();
        void Bind()                   { DX11_GetContext()->CSSetShader(cshader, NULL, 0); }
        void Dispatch(const ivec3& d) { DX11_GetContext()->Dispatch(d.x, d.y, d.z); }

    private:
        ID3D11ComputeShader* cshader = NULL;

};

#endif