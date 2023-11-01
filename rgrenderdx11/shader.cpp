#define _CRT_SECURE_NO_WARNINGS
#include "shader.h"

#include "rgrenderdx11.h"
#include "dx11.h"
#include <rgvector.h>
#include <engine.h>
#include <stdio.h>
#include <d3dcompiler.h>

// Compile shaders
// fxc /T vs_5_0 /E vmain /Fo vmain.cso main.vs
// fxc /T ps_5_0 /E pmain /Fo pmain.cso main.ps

Shader::Shader(PipelineDescription* description, String vs, String ps, bool compiled) {
    ShaderCode vsBuffer;
    if (compiled) { vsBuffer = LoadCompiledShaders(vs, ps); }
    else { vsBuffer = LoadShaders(vs, ps); }

    vec3 pos;
    vec3 normal;
    vec3 tangent;
    vec2 uv;

    D3D11_INPUT_ELEMENT_DESC* layoutDescriptions =
        (D3D11_INPUT_ELEMENT_DESC*)RGetAllocator()->Allocate(sizeof(D3D11_INPUT_ELEMENT_DESC) * description->inputCount);

    for (Uint32 i = 0; i < description->inputCount; i++) {
        layoutDescriptions[i].SemanticName = description->descriptions[i].name;
        layoutDescriptions[i].SemanticIndex = 0;
        layoutDescriptions[i].Format = (DXGI_FORMAT)description->descriptions[i].format;
        layoutDescriptions[i].InputSlot = description->descriptions[i].inputSlot;
        if (i == 0) { layoutDescriptions[i].AlignedByteOffset = 0; }
        else { layoutDescriptions[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT; }
        layoutDescriptions[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        layoutDescriptions[i].InstanceDataStepRate = 0;
    }

    HRESULT res = DX11_GetDevice()->CreateInputLayout(layoutDescriptions, description->inputCount, vsBuffer.data, vsBuffer.len, &layout);
    RGetAllocator()->Deallocate(vsBuffer.data);
    RGetAllocator()->Deallocate(layoutDescriptions);

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0;
    samplerDesc.BorderColor[1] = 0;
    samplerDesc.BorderColor[2] = 0;
    samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    DX11_GetDevice()->CreateSamplerState(&samplerDesc, &sampleState);
}

Shader::~Shader() {
    layout->Release();
    vshader->Release();
    pshader->Release();
    sampleState->Release();
}

void Shader::Bind() {
    DX11_GetContext()->IASetInputLayout(layout);
    DX11_GetContext()->VSSetShader(vshader, NULL, 0);
    DX11_GetContext()->PSSetShader(pshader, NULL, 0);
    DX11_GetContext()->PSSetSamplers(0, 1, &sampleState);
}

static void PrintErrorMessage(ID3D10Blob* b) {
    char* buff = (char*)RGetAllocator()->Allocate(b->GetBufferSize() + 1);
    SDL_memset(buff, 0, b->GetBufferSize() + 1);
    SDL_memcpy(buff, b->GetBufferPointer(), b->GetBufferSize());
    rgLogError(RG_LOG_RENDER, "Error: %s\n", buff);
    RGetAllocator()->Deallocate(buff);
}

ShaderCode Shader::LoadShaders(String vs, String ps) {
    ShaderCode code = {};
#if 1
    ID3D10Blob* errmsg = 0;
    ID3D10Blob* vsBuffer = 0;
    ID3D10Blob* psBuffer = 0;
    HRESULT     result;

    wchar_t VERTEXSHADER[256];
    wchar_t PIXELSHADER[256];
    wsprintf(VERTEXSHADER, L"%hs", vs);
    wsprintf(PIXELSHADER, L"%hs", ps);

    //result = D3DCompile(vbuffer, vlen, "vertex", NULL, NULL, "vmain", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &vsBuffer, &errmsg);
    //result = D3DCompile(pbuffer, plen, "pixel", NULL, NULL, "pmain", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &psBuffer, &errmsg);

    result = D3DCompileFromFile(VERTEXSHADER, NULL, NULL, "vmain", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &vsBuffer, &errmsg);
    if (FAILED(result)) {
        rgLogError(RG_LOG_RENDER, "Error code: %x\n", result);
        if (errmsg) {
            PrintErrorMessage(errmsg);
        }
    }
    result = D3DCompileFromFile(PIXELSHADER, NULL, NULL, "pmain", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &psBuffer, &errmsg);
    if (FAILED(result)) {
        rgLogError(RG_LOG_RENDER, "Error code: %x\n", result);
        if (errmsg) {
            PrintErrorMessage(errmsg);
        }
    }

    DX11_GetDevice()->CreateVertexShader(vsBuffer->GetBufferPointer(), vsBuffer->GetBufferSize(), NULL, &vshader);
    DX11_GetDevice()->CreatePixelShader(psBuffer->GetBufferPointer(), psBuffer->GetBufferSize(), NULL, &pshader);

    code.len = vsBuffer->GetBufferSize();
    code.data = RGetAllocator()->Allocate(code.len);
    SDL_memcpy(code.data, vsBuffer->GetBufferPointer(), code.len);

    vsBuffer->Release();
    psBuffer->Release();
#endif
    return code;
}

ShaderCode Shader::LoadCompiledShaders(String vs, String ps) {
    rgLogInfo(RG_LOG_RENDER, "Loading shaders: %s %s\n", vs, ps);
    ID3D10Blob* errmsg = 0;
    ID3D10Blob* vsBuffer = 0;
    ID3D10Blob* psBuffer = 0;
    HRESULT     result;

    void* vbuffer = NULL;
    void* pbuffer = NULL;
    long  vlen = 0;
    long  plen = 0;

    FILE* vfile = fopen(vs, "r");
    if (!vfile) {
        rgLogError(RG_LOG_RENDER, "VS File not found!\n");
    }
    fseek(vfile, 0, SEEK_END);
    vlen = ftell(vfile);
    fseek(vfile, 0, SEEK_SET);
    vbuffer = RGetAllocator()->Allocate(vlen);
    ZeroMemory(vbuffer, vlen);
    fread(vbuffer, 1, vlen, vfile);
    fclose(vfile);

    FILE* pfile = fopen(ps, "r");
    if (!pfile) {
        rgLogError(RG_LOG_RENDER, "PS File not found!\n");
    }
    fseek(pfile, 0, SEEK_END);
    plen = ftell(pfile);
    fseek(pfile, 0, SEEK_SET);
    pbuffer = RGetAllocator()->Allocate(plen);
    ZeroMemory(pbuffer, plen);
    fread(pbuffer, 1, plen, pfile);
    fclose(pfile);

    result = DX11_GetDevice()->CreateVertexShader(vbuffer, vlen, NULL, &vshader);
    result = DX11_GetDevice()->CreatePixelShader(pbuffer, plen, NULL, &pshader);

    ShaderCode code;
    code.len = vlen;
    code.data = vbuffer;

    RGetAllocator()->Deallocate(pbuffer);

    return code;
}

ComputeShader::ComputeShader(String cs, bool compiled) {
    HRESULT     result;
    ID3DBlob*   csBlob = NULL;
    ID3D10Blob* errmsg = NULL;
    wchar_t     COMPUTESHADER[256];

    wsprintf(COMPUTESHADER, L"%hs", cs);

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
    if (Engine::IsDebug()) {
        flags |= D3DCOMPILE_DEBUG;
    }

    result = D3DCompileFromFile(COMPUTESHADER, NULL, NULL, "main", "cs_5_0", flags, 0, &csBlob, &errmsg);
    if (FAILED(result)) {
        rgLogError(RG_LOG_RENDER, "Error code: %x\n", result);
        if (errmsg) {
            PrintErrorMessage(errmsg);
        }
    }

    result = DX11_GetDevice()->CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), NULL, &this->cshader);
    csBlob->Release();

}

ComputeShader::~ComputeShader() {
    this->cshader->Release();
}