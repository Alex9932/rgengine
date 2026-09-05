#include <rshared.h>
#include "rendertypesdx.h"
#include <allocator.h>

#include "imgui_impl_dx11.h"

using namespace Engine;

//////////////////////////////////////////////////////////
// COMMAND BUFFER
//////////////////////////////////////////////////////////

RCommandBuffer* R_CreateCommandBuffer(RRenderDevice* dev, RCommandBufferCreateInfo* info) {
	RCommandBuffer* cmdbuff = (RCommandBuffer*)dev->allocator->Allocate(sizeof(RCommandBuffer));
	cmdbuff->dev = dev;
	cmdbuff->max_commands = info->maxcmds;
	char allocname[64];
	SDL_snprintf(allocname, 64, "DX11 Commandpool (0x%p)", cmdbuff);
	if (info->maxcmds == 0) { info->maxcmds = R_MAX_COMMANDS_PER_BUFFER; }
	cmdbuff->pool = RG_NEW_CLASS(dev->allocator, LinearAllocator)(allocname, info->maxcmds *sizeof(RCommand));
	cmdbuff->commands_recorded = 0;
	return cmdbuff;
}

void R_DestroyCommandBuffer(RCommandBuffer* cmdbuff) {
	RRenderDevice* dev = cmdbuff->dev;
	cmdbuff->pool->Deallocate();
	RG_DELETE_CLASS(dev->allocator, LinearAllocator, cmdbuff->pool);
	dev->allocator->Deallocate(cmdbuff);
}

void R_ResetCommandBuffer(RCommandBuffer* cmdbuff) {
	cmdbuff->commands_recorded = 0;
	cmdbuff->pool->Deallocate();
}

void R_BeginCommandBuffer(RCommandBuffer* buffer) { /* DO NOTHING */ }
void R_EndCommandBuffer(RCommandBuffer* buffer)   { /* DO NOTHING */ }

static RG_INLINE DXGI_FORMAT GetIndexType(IndexType type) {
	switch (type) {
	case RG_INDEX_U8:  return DXGI_FORMAT_R8_UINT;
	case RG_INDEX_U16: return DXGI_FORMAT_R16_UINT;
	case RG_INDEX_U32: return DXGI_FORMAT_R32_UINT;
	default:           return DXGI_FORMAT_R8_UINT;
	}
}

static RG_INLINE DXResourceMapping* GetResourceMapping(Uint16 resid, RPipeline* pl) {
	// Use lookup table
	//Uint8 set     = (resid & 0xFF00) >> 8;
	//Uint8 binding = (resid & 0x00FF);

	for (Uint32 i = 0; i < pl->bindings; i++) {
		if (pl->map_table[i].idx == resid) {
			return &pl->map_table[i];
		}
	}

	return NULL;

}

static RG_INLINE void CMD_BeginRenderpassImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RRenderpass* rp = (RRenderpass*)cmd->handle;
	Uint64 fb_ptr = *((Uint64*)&cmd->buffer[0]);
	RFramebuffer* fb = (RFramebuffer*)fb_ptr;
	RRenderpassClearInfo* clearinfo = (RRenderpassClearInfo*)&cmd->buffer[8];
	RRenderDevice* dev = buffer->dev;

	// Set render targets, clear, etc.

	dev->dxctx->OMSetRenderTargets(fb->rtv_count, fb->rtv, fb->dsv);

	if (cmd->_off0) {
		for (Uint32 i = 0; i < fb->rtv_count; i++) {
			dev->dxctx->ClearRenderTargetView(fb->rtv[i], clearinfo->color[i].array);
		}
		if (fb->dsv) {
			dev->dxctx->ClearDepthStencilView(fb->dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearinfo->depth, (Uint8)clearinfo->stencil);
		}
	}


	D3D11_VIEWPORT pViewport = {};
	pViewport.TopLeftX = rp->viewport.x;
	pViewport.TopLeftY = rp->viewport.y;
	pViewport.Width    = rp->viewport.width;
	pViewport.Height   = rp->viewport.height;
	pViewport.MinDepth = 0.0f;
	pViewport.MaxDepth = 1.0f;
	dev->dxctx->RSSetViewports(1, &pViewport);

#if R_DXRENDER_DEBUG
	DX11_PollInfoQueue(dev);
#endif
}

static RG_INLINE void CMD_EndRenderpassImpl(RCommandBuffer* buffer, RCommand* cmd) {

}

static RG_INLINE void CMD_BindPipelineImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RRenderDevice* dev = buffer->dev;
	RPipeline* pl = (RPipeline*)cmd->handle;

	if (pl->type == RG_PIPELINE_TYPE_GRAPHICS) {


		dev->dxctx->OMSetDepthStencilState(pl->depth_stencil_state, 1);
		dev->dxctx->RSSetState(pl->raster_state);

		Float32 blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		dev->dxctx->OMSetBlendState(pl->blend_state, blendFactor, 0xffffffff);


		// Bind graphics pipeline
		buffer->dev->dxctx->IASetInputLayout(pl->layout);
		buffer->dev->dxctx->VSSetShader(pl->vs, NULL, 0);
		buffer->dev->dxctx->PSSetShader(pl->ps, NULL, 0);
		if (pl->gs) {
			buffer->dev->dxctx->GSSetShader(pl->gs, NULL, 0);
		}
		else {
			buffer->dev->dxctx->GSSetShader(NULL, NULL, 0);
		}

		buffer->dev->dxctx->CSSetShader(NULL, NULL, 0);

		ID3D11UnorderedAccessView* uav = NULL;
		buffer->dev->dxctx->CSSetUnorderedAccessViews(0, 1, &uav, NULL);

	} else {
		// Bind compute pipeline
		buffer->dev->dxctx->VSSetShader(NULL, NULL, 0);
		buffer->dev->dxctx->PSSetShader(NULL, NULL, 0);
		buffer->dev->dxctx->GSSetShader(NULL, NULL, 0);
		buffer->dev->dxctx->CSSetShader(pl->cs, NULL, 0);
	}
#if R_DXRENDER_DEBUG
	DX11_PollInfoQueue(dev);
#endif
}

static RG_INLINE void CMD_BindVertexBufferImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RBuffer* vb = (RBuffer*)cmd->handle;
	Uint32 slot = cmd->data0;
	UINT stride = cmd->data1;
	UINT offset = cmd->data2;
	buffer->dev->dxctx->IASetVertexBuffers(slot, 1, &vb->buffer, &stride, &offset);
#if R_DXRENDER_DEBUG
	DX11_PollInfoQueue(buffer->dev);
#endif
}

static RG_INLINE void CMD_BindIndexBufferImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RBuffer* ib = (RBuffer*)cmd->handle;
	IndexType indexFormat = (IndexType)cmd->data0;  // Index size in bytes
	buffer->dev->dxctx->IASetIndexBuffer(ib->buffer, GetIndexType(indexFormat), 0);
	buffer->dev->dxctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#if R_DXRENDER_DEBUG
	DX11_PollInfoQueue(buffer->dev);
#endif
}

static RG_INLINE void CMD_BindDescriptorImpl(RCommandBuffer* buffer, RCommand* cmd) {
	//RDescriptorEntry* entry = (RDescriptorEntry*)cmd->buffer;
	//Uint8 stage = (cmd->_off0 & 0xF000) >> 12; // Upper 4 bits
	//Uint8 reg   = (cmd->_off0 & 0x0F00) >> 8;  // Middle 4 bits
	//Uint8 slot  = (cmd->_off0 & 0x00FF);       // Lower 8 bits

	RRenderDevice* dev = buffer->dev;

	Uint8 stage = (Uint8)cmd->_off1;
	Uint8 type  = (cmd->_off0 & 0xFF00) >> 8;
	Uint8 slot  = (cmd->_off0 & 0x00FF);
	
	if (stage == DXRM_STAGE_VERTEX) {
		switch (type) {
			case DX_RESOURCE_TYPE_TEXTURE:        { dev->dxctx->VSSetShaderResources(slot, 1, (ID3D11ShaderResourceView**)&cmd->handle); break; }
			case DX_RESOURCE_TYPE_SAMPLER:        { dev->dxctx->VSSetSamplers(slot, 1, (ID3D11SamplerState**)&cmd->handle); break; }
			case DX_RESOURCE_TYPE_CONSTANTBUFFER: { dev->dxctx->VSSetConstantBuffers(slot, 1, &((RBuffer*)cmd->handle)->buffer); break; }
			case DX_RESOURCE_TYPE_STORAGE_SRV:    { dev->dxctx->VSSetShaderResources(slot, 1, (ID3D11ShaderResourceView**)&cmd->handle); break; }
			// dev->dxctx->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, info->slot, 1, &info->rv->uav, NULL);
			//case DX_RESOURCE_TYPE_STORAGE_UAV:    { dev->dxctx->VSSetUnorderedAccessViews(slot, 1, (ID3D11UnorderedAccessView**)&cmd->handle, NULL); break; }
		}
	} else if (stage == DXRM_STAGE_GEOMETRY) {
		switch (type) {
			case DX_RESOURCE_TYPE_TEXTURE:        { dev->dxctx->GSSetShaderResources(slot, 1, (ID3D11ShaderResourceView**)&cmd->handle); break; }
			case DX_RESOURCE_TYPE_SAMPLER:        { dev->dxctx->GSSetSamplers(slot, 1, (ID3D11SamplerState**)&cmd->handle); break; }
			case DX_RESOURCE_TYPE_CONSTANTBUFFER: { dev->dxctx->GSSetConstantBuffers(slot, 1, &((RBuffer*)cmd->handle)->buffer); break; }
			case DX_RESOURCE_TYPE_STORAGE_SRV:    { dev->dxctx->GSSetShaderResources(slot, 1, (ID3D11ShaderResourceView**)&cmd->handle); break; }
			// dev->dxctx->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, info->slot, 1, &info->rv->uav, NULL);
			//case DX_RESOURCE_TYPE_STORAGE_UAV:    { dev->dxctx->GSSetUnorderedAccessViews(slot, 1, (ID3D11UnorderedAccessView**)&cmd->handle, NULL); break; }
		}
	} else if (stage == DXRM_STAGE_PIXEL) {
		switch (type) {
			case DX_RESOURCE_TYPE_TEXTURE:        { dev->dxctx->PSSetShaderResources(slot, 1, (ID3D11ShaderResourceView**)&cmd->handle); break; }
			case DX_RESOURCE_TYPE_SAMPLER:        { dev->dxctx->PSSetSamplers(slot, 1, (ID3D11SamplerState**)&cmd->handle); break; }
			case DX_RESOURCE_TYPE_CONSTANTBUFFER: { dev->dxctx->PSSetConstantBuffers(slot, 1, &((RBuffer*)cmd->handle)->buffer); break; }
			case DX_RESOURCE_TYPE_STORAGE_SRV:    { dev->dxctx->PSSetShaderResources(slot, 1, (ID3D11ShaderResourceView**)&cmd->handle); break; }
			// dev->dxctx->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, info->slot, 1, &info->rv->uav, NULL);
			//case DX_RESOURCE_TYPE_STORAGE_UAV:    { dev->dxctx->PSSetUnorderedAccessViews(slot, 1, (ID3D11UnorderedAccessView**)&cmd->handle, NULL); break; }
		}
	} else if (stage == DXRM_STAGE_COMPUTE) {
		switch (type) {
			case DX_RESOURCE_TYPE_TEXTURE:        { dev->dxctx->CSSetShaderResources(slot, 1, (ID3D11ShaderResourceView**)&cmd->handle); break; }
			case DX_RESOURCE_TYPE_SAMPLER:        { dev->dxctx->CSSetSamplers(slot, 1, (ID3D11SamplerState**)&cmd->handle); break; }
			case DX_RESOURCE_TYPE_CONSTANTBUFFER: { dev->dxctx->CSSetConstantBuffers(slot, 1, &((RBuffer*)cmd->handle)->buffer); break; }
			case DX_RESOURCE_TYPE_STORAGE_SRV:    { dev->dxctx->CSSetShaderResources(slot, 1, (ID3D11ShaderResourceView**)&cmd->handle); break; }
			case DX_RESOURCE_TYPE_STORAGE_UAV:    { dev->dxctx->CSSetUnorderedAccessViews(slot, 1, (ID3D11UnorderedAccessView**)&cmd->handle, NULL); break; }
		}
	}
#if R_DXRENDER_DEBUG
	DX11_PollInfoQueue(dev);
#endif

}

static RG_INLINE void CMD_BindSamplerImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RSampler* sampler = (RSampler*)cmd->handle;
	Uint32 slot = cmd->data0;
	// Pixel shader only (unified with Vulkan backend)
	// TODO: Mark "stage" as deprecated?
#if 0
	Uint32 stage = cmd->data1;
	if (stage == RG_SHADER_TYPE_VERTEX) {
		buffer->dev->dxctx->VSSetSamplers(slot, 1, &sampler->state);
	}
	else if (stage == RG_SHADER_TYPE_PIXEL) {
#endif
		buffer->dev->dxctx->PSSetSamplers(slot, 1, &sampler->state);
#if 0
	}
	else if (stage == RG_SHADER_TYPE_COMPUTE) {
		buffer->dev->dxctx->CSSetSamplers(slot, 1, &sampler->state);
	}
#endif
#if R_DXRENDER_DEBUG
	DX11_PollInfoQueue(buffer->dev);
#endif
}

static RG_INLINE void CMD_UpdatePushConstants(RCommandBuffer* buffer, RCommand* cmd) {
	RRenderDevice* dev = buffer->dev;
	void* data = cmd->buffer;

	Uint8 size  = cmd->_off1;

	ID3D11Buffer* d3d11buffer = dev->pushconstant->buffer;
	Uint8         access      = dev->pushconstant->access;

	// Update constant buffer
	if (access == RG_BUFFER_ACCESS_GPU_ONLY) {
		// Use UpdateSubresource
		// !! WARN: No partial updates supported here !!
		buffer->dev->dxctx->UpdateSubresource(d3d11buffer, 0, NULL, data, 0, 0);
	}
	else if (access == RG_BUFFER_ACCESS_CPU_WRITE) {
		// Map the buffer and copy data
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT result = buffer->dev->dxctx->Map(d3d11buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (SUCCEEDED(result)) {
			SDL_memcpy(mappedResource.pData, data, size);
			buffer->dev->dxctx->Unmap(d3d11buffer, 0);
		}
	}
#if R_DXRENDER_DEBUG
	DX11_PollInfoQueue(dev);
#endif
}

static RG_INLINE void CMD_PushConstants(RCommandBuffer* buffer, RCommand* cmd) {
	RRenderDevice* dev = buffer->dev;
	void* data = cmd->buffer;

	Uint8 slot  = (cmd->_off0 & 0x00FF);
	Uint8 stage = (cmd->_off0 & 0xFF00) >> 8;

	// Bind constant buffer
	switch (stage) {
		case DXRM_STAGE_VERTEX:   { dev->dxctx->VSSetConstantBuffers(slot, 1, &dev->pushconstant->buffer); break; }
		case DXRM_STAGE_GEOMETRY: { dev->dxctx->GSSetConstantBuffers(slot, 1, &dev->pushconstant->buffer); break; }
		case DXRM_STAGE_PIXEL:    { dev->dxctx->PSSetConstantBuffers(slot, 1, &dev->pushconstant->buffer); break; }
		case DXRM_STAGE_COMPUTE:  { dev->dxctx->CSSetConstantBuffers(slot, 1, &dev->pushconstant->buffer); break; }
	}
#if R_DXRENDER_DEBUG
	DX11_PollInfoQueue(dev);
#endif
}

static RG_INLINE void CMD_DrawImGuiImpl(RCommandBuffer* buffer, RCommand* cmd) {
	// Restore pointer
	void* rawpointer = (void*)(((Uint64)cmd->data0 << 32) | (Uint64)cmd->data1);
	ImGui_ImplDX11_RenderDrawData((ImDrawData*)rawpointer);
}

static RG_INLINE void CMD_DrawIndexdImpl(RCommandBuffer* buffer, RCommand* cmd) {
	Uint32 idxcount = cmd->data0;
	Uint32 idxstart = cmd->data1;
	buffer->dev->dxctx->DrawIndexed(idxcount, idxstart, 0);
#if R_DXRENDER_DEBUG
	DX11_PollInfoQueue(buffer->dev);
#endif
}

static RG_INLINE void CMD_DispatchImpl(RCommandBuffer* buffer, RCommand* cmd) {
	Uint32 groupcount_x = cmd->data0;
	Uint32 groupcount_y = cmd->data1;
	Uint32 groupcount_z = cmd->data2;
	buffer->dev->dxctx->Dispatch(groupcount_x, groupcount_y, groupcount_z);
#if R_DXRENDER_DEBUG
	DX11_PollInfoQueue(buffer->dev);
#endif
}

void R_SubmitCommandBuffer(RCommandBufferSubmitInfo* info) {
	RCommandBuffer* buffer = info->buffer;

	// Execute commands

	for (Uint32 i = 0; i < buffer->commands_recorded; i++) {
		RCommand* cmd = (RCommand*)((Uint8*)buffer->pool->GetBasePointer() + i * sizeof(RCommand));
		switch (cmd->cmd) {
			case R_CMD_NOP: { break; }
			case R_CMD_BEGIN_RENDERPASS:   { CMD_BeginRenderpassImpl(buffer, cmd); break; }
			case R_CMD_END_RENDERPASS:     { CMD_EndRenderpassImpl(buffer, cmd); break; }
			case R_CMD_BIND_PIPELINE:      { CMD_BindPipelineImpl(buffer, cmd); break; }
			case R_CMD_BIND_VERTEX_BUFFER: { CMD_BindVertexBufferImpl(buffer, cmd); break; }
			case R_CMD_BIND_INDEX_BUFFER:  { CMD_BindIndexBufferImpl(buffer, cmd); break; }
			case R_CMD_BIND_DESCRIPTOR:    { CMD_BindDescriptorImpl(buffer, cmd); break; }
			case R_CMD_BIND_SAMPLER:       { CMD_BindSamplerImpl(buffer, cmd); break; }
			case R_CMD_UPDPUSHCONSTANTS:   { CMD_UpdatePushConstants(buffer, cmd); break; }
			case R_CMD_PUSHCONSTANTS:      { CMD_PushConstants(buffer, cmd); break; }
			case R_CMD_DRAW_IMGUI:         { CMD_DrawImGuiImpl(buffer, cmd); break; }
			//case R_CMD_DRAW:               { CMD_DrawImpl(buffer, cmd); break; }
			case R_CMD_DRAW_INDEXED:       { CMD_DrawIndexdImpl(buffer, cmd); break; }
			case R_CMD_DISPATCH:           { CMD_DispatchImpl(buffer, cmd); break; }

			default: {
#if R_DXRENDER_DEBUG
				rgLogError(RG_LOG_RENDER, "DX11 Renderer: Invalid or unimplemented opcode in command buffer!");
#endif
				break;
			}
		}
	}

}

///////////
// COMMANDS

static RG_INLINE RCommand* AllocateNextCommand(RCommandBuffer* cmdbuff) {
	RCommand* cmd = (RCommand*)cmdbuff->pool->Allocate(sizeof(RCommand));
	cmdbuff->commands_recorded++;
	if (!cmd) {
		__debugbreak();
	}
	return cmd;
}

void R_CmdBeginRenderpass(RCommandBuffer* cmdbuff, RRenderpassBeginInfo* info) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	if (!cmd) { return; }
	cmd->cmd    = R_CMD_BEGIN_RENDERPASS;
	cmd->handle = NULL;
	cmd->_off0  = 0;

	// Use default renderpass
	if (!info) {
		RRenderDevice* dev = cmdbuff->dev;
		cmd->handle = dev->default_renderpass;
		SDL_memcpy(cmd->buffer, &dev->default_framebuffers[dev->currentframe], sizeof(RFramebuffer*));
		return;
	}

	cmd->handle = info->renderpass;

	// WARN: Memory layout dependency

	// Store framebuffer pointer
	SDL_memcpy(cmd->buffer, &info->framebuffer, sizeof(RFramebuffer*));

	// Store clear info
	if (info->clearinfo) {
		cmd->_off0 = 1;
		SDL_memcpy(&cmd->buffer[8], info->clearinfo, sizeof(RRenderpassClearInfo));
	}
}

void R_CmdEndRenderpass(RCommandBuffer* cmdbuff) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	if (!cmd) { return; }
	cmd->cmd = R_CMD_END_RENDERPASS;
}

void R_CmdBindPipeline(RCommandBuffer* cmdbuff, RPipeline* pl) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	if (!cmd) { return; }
	cmd->cmd    = R_CMD_BIND_PIPELINE;
	cmd->handle = pl;
	cmdbuff->pipeline = pl; // Save current pipeline
}

void R_CmdBindVertexBuffer(RCommandBuffer* cmdbuff, RBuffer* vb, Uint32 slot, Uint32 stride) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	if (!cmd) { return; }
	cmd->cmd    = R_CMD_BIND_VERTEX_BUFFER;
	cmd->handle = vb;
	cmd->data0  = slot;   // Bind slot
	cmd->data1  = stride; // Stride
	cmd->data2  = 0;      // Offset
}

void R_CmdBindIndexBuffer(RCommandBuffer* cmdbuff, RBuffer* ib, IndexType isize) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	if (!cmd) { return; }
	cmd->cmd    = R_CMD_BIND_INDEX_BUFFER;
	cmd->handle = ib;
	cmd->data0  = isize;
}

void R_CmdBindDescriptorSets(RCommandBuffer* cmdbuff, RBindDescriptorSetsInfo* info) {
	for (size_t i = 0; i < info->count; i++) {

		RDescriptorSet* set = info->sets[i];
		Uint8 setid = info->startslot + i;

		for (size_t j = 0; j < set->entry_count; j++) {
			RDescriptorEntry* entry = &set->entrys[j];
			Uint8 binding = entry->binding;
			Uint16 idx = ((Uint16)setid << 8) | binding;
			
			DXResourceMapping* table = GetResourceMapping(idx, cmdbuff->pipeline);
			if (!table) { continue; } // No bind point

			RCommand* cmd = NULL;
			for (Uint32 k = 0; k < DXRM_STAGE_MAX; k++) {
				DXRM* mapping = &table->mappings[k];
				if (!mapping->valid) { continue; }

				cmd = AllocateNextCommand(cmdbuff);
				if (!cmd) { return; }
				cmd->cmd = R_CMD_BIND_DESCRIPTOR;

				Uint16 type = mapping->type;

				if (mapping->type == DX_RESOURCE_TYPE_TEXTURE) {
					cmd->handle = entry->srv;
				}
				//else if (mapping->type == DX_RESOURCE_TYPE_SAMPLER) { // Skip this (samplers is bound other way)
				else if (mapping->type == DX_RESOURCE_TYPE_CONSTANTBUFFER) {
					cmd->handle = entry->buffer;
				}
				else if (mapping->type == DX_RESOURCE_TYPE_STORAGEBUFFER &&
					RG_CHECK_FLAG(entry->buffer->type, RG_BUFFER_TYPE_UNORDERED)) {
					cmd->handle = entry->uav;
					type = DX_RESOURCE_TYPE_STORAGE_UAV;
				}
				else if (mapping->type == DX_RESOURCE_TYPE_STORAGEBUFFER &&
					RG_CHECK_FLAG(entry->buffer->type, RG_BUFFER_TYPE_STRUCTURED)) {
					cmd->handle = entry->srv;
					type = DX_RESOURCE_TYPE_STORAGE_SRV;
				}

				cmd->_off0 = (type << 8) | mapping->slot; // Resource type | slot
				cmd->_off1 = k; // Pipeline stage
			}
		}
	}
}

void R_CmdBindSampler(RCommandBuffer* cmdbuff, RSampler* sampler, Uint32 slot, Uint32 stage) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	if (!cmd) { return; }
	cmd->cmd    = R_CMD_BIND_SAMPLER;
	cmd->handle = sampler;
	cmd->data0  = slot;
	cmd->data1  = stage;
}

/*
void R_CmdDraw(RCommandBuffer* cmdbuff, Uint32 idxcount, Uint32 idxstart) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd = R_CMD_DRAW;
	cmd->data0 = idxcount;
	cmd->data1 = idxstart;
}
*/

void R_CmdDrawIndexed(RCommandBuffer* cmdbuff, Uint32 idxcount, Uint32 idxstart) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	if (!cmd) { return; }
	cmd->cmd = R_CMD_DRAW_INDEXED;
	cmd->data0 = idxcount;
	cmd->data1 = idxstart;
}

void R_CmdPushConstants(RCommandBuffer* cmdbuff, void* buffer, Uint32 size) {

	// Update buffer
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	if (!cmd) { return; }
	cmd->cmd = R_CMD_UPDPUSHCONSTANTS;
	cmd->_off1 = size;
	SDL_memcpy(cmd->buffer, buffer, SDL_min(size, 128)); // Copy max 128 bytes

	// Bind resource
	Uint16 idx = 0x00FF; // Push constant address (set = 0, binding = 0xFF)
	DXResourceMapping* table = GetResourceMapping(idx, cmdbuff->pipeline);
	if (!table) { return; } // No bind point

	for (Uint32 i = 0; i < DXRM_STAGE_MAX; i++) {
		DXRM* mapping = &table->mappings[i];
		if (!mapping->valid) { continue; }

		cmd = AllocateNextCommand(cmdbuff);
		if (!cmd) { return; }
		cmd->cmd = R_CMD_PUSHCONSTANTS;
		cmd->_off0 = (i << 8) | mapping->slot; // Stage | Slot
	}

}

void R_CmdImGuiRenderDrawData(RCommandBuffer* cmdbuff, void* data) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	if (!cmd) { return; }
	cmd->cmd = R_CMD_DRAW_IMGUI;

	Uint64 address = (Uint64)data;
	cmd->data0 = (Uint32)(address >> 32); // High part
	cmd->data1 = (Uint32)(address & 0xFFFFFFFF); // Low part
}

void R_CmdDispatch(RCommandBuffer* cmdbuff, Uint32 groupcount_x, Uint32 groupcount_y, Uint32 groupcount_z) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	if (!cmd) { return; }
	cmd->cmd = R_CMD_DISPATCH;
	cmd->data0 = groupcount_x;
	cmd->data1 = groupcount_y;
	cmd->data2 = groupcount_z;
}

void R_CmdUseImage(RCommandBuffer* cmdbuff, RImage* image, Uint32 usage) {
	// NOP
	return;
}