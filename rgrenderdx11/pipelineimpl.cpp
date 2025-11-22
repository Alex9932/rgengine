#include <rshared.h>
#include "rendertypesdx.h"
#include <filesystem.h>
#include <d3dcompiler.h>

RRenderpass* R_CreateRenderpass(RRenderDevice* dev, RRenderpassCreateInfo* info) {
	RRenderpass* rp = (RRenderpass*)dev->allocator->Allocate(sizeof(RRenderpass));
	rp->dev = dev;

	rp->rtv_count = info->rt_count;
	rp->dsv = NULL;
	rp->depth_stencil_state = NULL;

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;

	for (Uint32 i = 0; i < rp->rtv_count; i++) {
#if R_DXRENDER_DEBUG
		if (info->rts[i]->type != R_DX_RESOURCEVIEW_RTV) {
			rgLogError(RG_LOG_RENDER, "DX11 Renderer: RResourceView->type(rts[%d]) must be a RG_RESOURCEVIEW_TYPE_RTV or RG_RESOURCEVIEW_TYPE_BBV in RRenderpass creation!", i);
		}
#endif

		blendDesc.RenderTarget[i].BlendEnable = true;
		blendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		rp->rtv[i] = info->rts[i]->rtv;
	}

	dev->dxdev->CreateBlendState(&blendDesc, &rp->blend_state);

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.AntialiasedLineEnable = false;
	switch (info->cullmode) {
		case RG_RENDERPASS_CULLMODE_NONE: { rasterDesc.CullMode = D3D11_CULL_NONE;  break; }
		case RG_RENDERPASS_CULLMODE_FRONT: { rasterDesc.CullMode = D3D11_CULL_FRONT; break; }
		case RG_RENDERPASS_CULLMODE_BACK: { rasterDesc.CullMode = D3D11_CULL_BACK;  break; }
		default: { rasterDesc.CullMode = D3D11_CULL_NONE; break; }
	}
	//rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = false;
	switch (info->fillmode) {
		case RG_RENDERPASS_FILLMODE_SOLID: { rasterDesc.FillMode = D3D11_FILL_SOLID; break; }
		case RG_RENDERPASS_FILLMODE_WIREFRAME: { rasterDesc.FillMode = D3D11_FILL_WIREFRAME; break; }
		default: { rasterDesc.FillMode = D3D11_FILL_SOLID; break; }
	}
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.StencilEnable = false;

	if (info->dsv) {
		rasterDesc.DepthClipEnable = true;

#if R_DXRENDER_DEBUG
		if (info->dsv->type != R_DX_RESOURCEVIEW_DSV) {
			rgLogError(RG_LOG_RENDER, "DX11 Renderer: RResourceView->type(dst) must be a RG_RESOURCEVIEW_TYPE_DSV in RRenderpass creation!");
		}
#endif

		// Make depth-stencil state
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
		depthStencilDesc.StencilEnable = true;
		depthStencilDesc.StencilReadMask = 0xFF;
		depthStencilDesc.StencilWriteMask = 0xFF;
		depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		rp->dsv = info->dsv->dsv;
	}

	dev->dxdev->CreateDepthStencilState(&depthStencilDesc, &rp->depth_stencil_state);

	dev->dxdev->CreateRasterizerState(&rasterDesc, &rp->raster_state);

	return rp;
}

void R_DestroyRenderpass(RRenderpass* rp) {
	RRenderDevice* dev = rp->dev;
	rp->blend_state->Release();
	rp->raster_state->Release();
	if (rp->depth_stencil_state) {
		rp->depth_stencil_state->Release();
	}
	dev->allocator->Deallocate(rp);
}

RPipeline* R_CreatePipeline(RRenderDevice* dev, RPipelineCreateInfo* info) {
	RPipeline* pl = (RPipeline*)dev->allocator->Allocate(sizeof(RPipeline));
	pl->dev = dev;
	pl->type = info->type;
	if (info->type == RG_PIPELINE_TYPE_GRAPHICS) {
		D3D11_INPUT_ELEMENT_DESC layoutDescriptions[16];
		for (Uint32 i = 0; i < info->inputCount; i++) {
			layoutDescriptions[i].SemanticName = info->descriptions[i].name;
			layoutDescriptions[i].SemanticIndex = 0;
			layoutDescriptions[i].Format = GetFormat(info->descriptions[i].format);//(DXGI_FORMAT)info->descriptions[i].format;
			layoutDescriptions[i].InputSlot = info->descriptions[i].inputSlot;
			if (i == 0) { layoutDescriptions[i].AlignedByteOffset = 0; }
			else { layoutDescriptions[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT; }
			layoutDescriptions[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			layoutDescriptions[i].InstanceDataStepRate = 0;
		}
		dev->dxdev->CreateInputLayout(layoutDescriptions, info->inputCount, info->vertex_shader->buffer->GetBufferPointer(), info->vertex_shader->buffer->GetBufferSize(), &pl->layout);
		pl->vs = info->vertex_shader->vs;
		pl->ps = info->pixel_shader->ps;
		if (info->geometry_shader) {
			pl->gs = info->geometry_shader->gs;
		} else {
			pl->gs = NULL;
		}
	} else {
		pl->cs = info->compute_shader->cs;
	}

	return pl;
}

void R_DestroyPipeline(RPipeline* pl) {
	RRenderDevice* dev = pl->dev;
	if (pl->type == RG_PIPELINE_TYPE_GRAPHICS) {
		pl->layout->Release();
	}
	dev->allocator->Deallocate(pl);
}

static void LoadCompiledShader(RShader* shader, String file) {
	Resource* v_res = Engine::GetResource(file);

	switch (shader->type) {
		case RG_SHADER_TYPE_VERTEX:   { shader->dev->dxdev->CreateVertexShader(v_res->data, v_res->length, NULL, &shader->vs); break; }
		case RG_SHADER_TYPE_PIXEL:    { shader->dev->dxdev->CreatePixelShader(v_res->data, v_res->length, NULL, &shader->ps); break; }
		case RG_SHADER_TYPE_GEOMETRY: { shader->dev->dxdev->CreateGeometryShader(v_res->data, v_res->length, NULL, &shader->gs); break; }
		case RG_SHADER_TYPE_COMPUTE:  { shader->dev->dxdev->CreateComputeShader(v_res->data, v_res->length, NULL, &shader->cs); break; }
		default: break;
	}

	Engine::FreeResource(v_res);
}

static void LoadShaderFromSource(RShader* shader, String file) {
	ID3D10Blob* errmsg = 0;
	HRESULT     result;

	Resource* v_res = Engine::GetResource(file);

	String shader_type;
	String shader_version;
	String shader_entrypoint;

	switch (shader->type) {
		case RG_SHADER_TYPE_VERTEX: {
			shader_type = "vertex";
			shader_version = "vs_5_0";
			shader_entrypoint = "vmain";
			break;
		}
		case RG_SHADER_TYPE_PIXEL: {
			shader_type = "pixel";
			shader_version = "ps_5_0";
			shader_entrypoint = "pmain";
			break;
		}
		case RG_SHADER_TYPE_GEOMETRY: {
			shader_type = "geometry";
			shader_version = "gs_5_0";
			shader_entrypoint = "gmain";
			break;
		}
		case RG_SHADER_TYPE_COMPUTE: {
			shader_type = "compute";
			shader_version = "cs_5_0";
			shader_entrypoint = "cmain";
			break;
		}
		default: {
			shader_type = "unknown";
			shader_version = "?";
			shader_entrypoint = "main";
			break;
		}
	}

	result = D3DCompile(v_res->data, v_res->length, shader_type, NULL, NULL, shader_entrypoint, shader_version, D3D10_SHADER_ENABLE_STRICTNESS, 0, &shader->buffer, &errmsg);
	if (FAILED(result)) {
		rgLogError(RG_LOG_RENDER, "Error code: %x\n", result);
		if (errmsg) {
			char err[256];
			SDL_memset(err, 0, 256);
			size_t len = SDL_min(errmsg->GetBufferSize(), 255);
			SDL_memcpy(err, errmsg->GetBufferPointer(), len);
			rgLogError(RG_LOG_RENDER, "DX11 Shader compile: %s\n", err);
			goto _ret;
		}
	}

	switch (shader->type) {
		case RG_SHADER_TYPE_VERTEX:   { shader->dev->dxdev->CreateVertexShader(shader->buffer->GetBufferPointer(), shader->buffer->GetBufferSize(), NULL, &shader->vs); break; }
		case RG_SHADER_TYPE_PIXEL:    { shader->dev->dxdev->CreatePixelShader(shader->buffer->GetBufferPointer(), shader->buffer->GetBufferSize(), NULL, &shader->ps); break; }
		case RG_SHADER_TYPE_GEOMETRY: { shader->dev->dxdev->CreateGeometryShader(shader->buffer->GetBufferPointer(), shader->buffer->GetBufferSize(), NULL, &shader->gs); break; }
		case RG_SHADER_TYPE_COMPUTE:  { shader->dev->dxdev->CreateComputeShader(shader->buffer->GetBufferPointer(), shader->buffer->GetBufferSize(), NULL, &shader->cs); break; }
		default: break;
	}

	_ret:
	Engine::FreeResource(v_res);
}

RShader* R_CreateShader(RRenderDevice* dev, RShaderCreateInfo* info) {
	RShader* shader = (RShader*)dev->allocator->Allocate(sizeof(RShader));
	shader->dev  = dev;
	shader->type = info->type;
	shader->isCompiled = info->isCompiled;

	char path[256];
	char file[256];
	Engine::GetPath(path, 256, RG_PATH_SYSTEM, R_RENDERER_SHORTNAME);
	SDL_snprintf(file, 256, "%s/%s", path, info->name);

	if (info->isCompiled) {
		LoadCompiledShader(shader, file);
	}
	else {
		LoadShaderFromSource(shader, file);
	}

	return shader;
}

void R_DestroyShader(RShader* shader) {
	RRenderDevice* dev = shader->dev;
	switch (shader->type) {
		case RG_SHADER_TYPE_VERTEX:   { shader->vs->Release(); break; }
		case RG_SHADER_TYPE_PIXEL:    { shader->ps->Release(); break; }
		case RG_SHADER_TYPE_GEOMETRY: { shader->gs->Release(); break; }
		case RG_SHADER_TYPE_COMPUTE:  { shader->cs->Release(); break; }
		default: break;
	}
	shader->buffer->Release();
	dev->allocator->Deallocate(shader);
}