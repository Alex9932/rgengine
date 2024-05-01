#define _CRT_SECURE_NO_WARNINGS
#include "shader.h"

#include "rgrenderdx11.h"
#include "dx11.h"
#include <rgvector.h>
#include <engine.h>
#include <stdio.h>
#include <d3dcompiler.h>

#include <filesystem.h>

// Compile shaders
// fxc /T vs_5_0 /E vmain /Fo vmain.cso main.vs
// fxc /T ps_5_0 /E pmain /Fo pmain.cso main.ps

void Shader::ClassConstructor(PipelineDescription* description, String vs, String ps, String gs, bool compiled) {
    ShaderCode vsBuffer;
    if (compiled) { vsBuffer = LoadCompiledShaders(vs, ps, gs); }
    else { vsBuffer = LoadShaders(vs, ps, gs); }

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

    if (description->sampler) {
        samplerDesc.AddressU = description->sampler->u;
        samplerDesc.AddressV = description->sampler->v;
        samplerDesc.AddressW = description->sampler->w;
    } else {
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    }
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
    if (gshader) {
        gshader->Release();
    }
    sampleState->Release();
}

void Shader::Bind() {
    DX11_GetContext()->IASetInputLayout(layout);
    DX11_GetContext()->VSSetShader(vshader, NULL, 0);
    DX11_GetContext()->PSSetShader(pshader, NULL, 0);
    if (gshader) {
        DX11_GetContext()->GSSetShader(gshader, NULL, 0);
    } else {
        DX11_GetContext()->GSSetShader(NULL, NULL, 0);
    }
    DX11_GetContext()->PSSetSamplers(0, 1, &sampleState);
}

static void PrintErrorMessage(ID3D10Blob* b) {
    char* buff = (char*)RGetAllocator()->Allocate(b->GetBufferSize() + 1);
    SDL_memset(buff, 0, b->GetBufferSize() + 1);
    SDL_memcpy(buff, b->GetBufferPointer(), b->GetBufferSize());
    rgLogError(RG_LOG_RENDER, "Error: %s\n", buff);
    RGetAllocator()->Deallocate(buff);
}

ShaderCode Shader::LoadShaders(String vs, String ps, String gs) {
    ShaderCode code = {};

    ID3D10Blob* errmsg = 0;
    ID3D10Blob* vsBuffer = 0;
    ID3D10Blob* psBuffer = 0;
    ID3D10Blob* gsBuffer = 0;
    HRESULT     result;

    Resource* v_res = Engine::GetResource(vs);

    result = D3DCompile(v_res->data, v_res->length, "vertex", NULL, NULL, "vmain", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &vsBuffer, &errmsg);
    if (FAILED(result)) {
        rgLogError(RG_LOG_RENDER, "Error code: %x\n", result);
        if (errmsg) {
            PrintErrorMessage(errmsg);
        }
    }
    Engine::FreeResource(v_res);

    Resource* p_res = Engine::GetResource(ps);
    result = D3DCompile(p_res->data, p_res->length, "pixel", NULL, NULL, "pmain", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &psBuffer, &errmsg);
    if (FAILED(result)) {
        rgLogError(RG_LOG_RENDER, "Error code: %x\n", result);
        if (errmsg) {
            PrintErrorMessage(errmsg);
        }
    }
    Engine::FreeResource(p_res);

    DX11_GetDevice()->CreateVertexShader(vsBuffer->GetBufferPointer(), vsBuffer->GetBufferSize(), NULL, &vshader);
    DX11_GetDevice()->CreatePixelShader(psBuffer->GetBufferPointer(), psBuffer->GetBufferSize(), NULL, &pshader);

    code.len = vsBuffer->GetBufferSize();
    code.data = RGetAllocator()->Allocate(code.len);
    SDL_memcpy(code.data, vsBuffer->GetBufferPointer(), code.len);

    vsBuffer->Release();
    psBuffer->Release();

    // Create geometry shader if needed
    if (gs) {
        Resource* g_res = Engine::GetResource(gs);
        result = D3DCompile(g_res->data, g_res->length, "geometry", NULL, NULL, "gmain", "gs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &gsBuffer, &errmsg);
        if (FAILED(result)) {
            rgLogError(RG_LOG_RENDER, "Error code: %x\n", result);
            if (errmsg) {
                PrintErrorMessage(errmsg);
            }
        }
        Engine::FreeResource(g_res);

        DX11_GetDevice()->CreateGeometryShader(gsBuffer->GetBufferPointer(), gsBuffer->GetBufferSize(), NULL, &gshader);

        gsBuffer->Release();
    }

    return code;
}

ShaderCode Shader::LoadCompiledShaders(String vs, String ps, String gs) {
    rgLogInfo(RG_LOG_RENDER, "Loading shaders: %s %s\n", vs, ps);

    HRESULT result;

    void* vbuffer = NULL;
    long  vlen = 0;

    Resource* v_res = Engine::GetResource(vs);
    vlen = v_res->length;
    vbuffer = RGetAllocator()->Allocate(v_res->length);
    SDL_memcpy(vbuffer, v_res->data, v_res->length);
    Engine::FreeResource(v_res);
    result = DX11_GetDevice()->CreateVertexShader(vbuffer, vlen, NULL, &vshader);

    Resource* p_res = Engine::GetResource(ps);
    result = DX11_GetDevice()->CreatePixelShader(p_res->data, p_res->length, NULL, &pshader);
    Engine::FreeResource(p_res);

    if (gs) {
        Resource* g_res = Engine::GetResource(gs);
        result = DX11_GetDevice()->CreateGeometryShader(g_res->data, g_res->length, NULL, &gshader);
        Engine::FreeResource(g_res);

    }

    ShaderCode code;
    code.len = vlen;
    code.data = vbuffer;

    return code;
}

ComputeShader::ComputeShader(String cs, bool compiled) {
    HRESULT     result;
    ID3DBlob*   csBlob = NULL;
    ID3D10Blob* errmsg = NULL;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
    if (Engine::IsDebug()) {
        flags |= D3DCOMPILE_DEBUG;
    }

    Resource* c_res = Engine::GetResource(cs);

    if (!compiled) {
        result = D3DCompile(c_res->data, c_res->length, "pixel", NULL, NULL, "main", "cs_5_0", flags, 0, &csBlob, &errmsg);
        if (FAILED(result)) {
            rgLogError(RG_LOG_RENDER, "Error code: %x\n", result);
            if (errmsg) {
                PrintErrorMessage(errmsg);
            }
        }

        DX11_GetDevice()->CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), NULL, &this->cshader);
        csBlob->Release();

    } else {
        DX11_GetDevice()->CreateComputeShader(c_res->data, c_res->length, NULL, &this->cshader);
    }

    Engine::FreeResource(c_res);

}

ComputeShader::~ComputeShader() {
    this->cshader->Release();
}