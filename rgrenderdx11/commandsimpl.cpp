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

static inline DXGI_FORMAT GetIndexType(IndexType type) {
	switch (type) {
	case RG_INDEX_U8:  return DXGI_FORMAT_R8_UINT;
	case RG_INDEX_U16: return DXGI_FORMAT_R16_UINT;
	case RG_INDEX_U32: return DXGI_FORMAT_R32_UINT;
	default:           return DXGI_FORMAT_R8_UINT;
	}
}

static RG_INLINE void CMD_BeginRenderpassImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RRenderpass* rp = (RRenderpass*)cmd->handle;
	RRenderpassClearInfo* clearinfo = (RRenderpassClearInfo*)cmd->buffer;
	RRenderDevice* dev = buffer->dev;

	// Set render targets, clear, etc.

	dev->dxctx->OMSetRenderTargets(rp->rtv_count, rp->rtv, rp->dsv);
	dev->dxctx->OMSetDepthStencilState(rp->depth_stencil_state, 1);
	dev->dxctx->RSSetState(rp->raster_state);

	if (cmd->_off0) {
		for (Uint32 i = 0; i < rp->rtv_count; i++) {
			dev->dxctx->ClearRenderTargetView(rp->rtv[i], clearinfo->color[i].array);
		}
		if (rp->dsv) {
			dev->dxctx->ClearDepthStencilView(rp->dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearinfo->depth, (Uint8)clearinfo->stencil);
		}
	}

	Float32 blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	dev->dxctx->OMSetBlendState(rp->blend_state, blendFactor, 0xffffffff);

	D3D11_VIEWPORT pViewport = {};
	pViewport.TopLeftX = rp->viewport.x;
	pViewport.TopLeftY = rp->viewport.y;
	pViewport.Width    = rp->viewport.width;
	pViewport.Height   = rp->viewport.height;
	pViewport.MinDepth = 0.0f;
	pViewport.MaxDepth = 1.0f;
	dev->dxctx->RSSetViewports(1, &pViewport);
}

static RG_INLINE void CMD_EndRenderpassImpl(RCommandBuffer* buffer, RCommand* cmd) {

}

static RG_INLINE void CMD_BindPipelineImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RPipeline* pl = (RPipeline*)cmd->handle;

	if (pl->type == RG_PIPELINE_TYPE_GRAPHICS) {
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
}

static RG_INLINE void CMD_BindVertexBufferImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RBuffer* vb = (RBuffer*)cmd->handle;
	Uint32 slot = cmd->data0;
	UINT stride = cmd->data1;
	UINT offset = cmd->data2;
	buffer->dev->dxctx->IASetVertexBuffers(slot, 1, &vb->buffer, &stride, &offset);
}

static RG_INLINE void CMD_BindIndexBufferImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RBuffer* ib = (RBuffer*)cmd->handle;
	IndexType indexFormat = (IndexType)cmd->data0;  // Index size in bytes
	buffer->dev->dxctx->IASetIndexBuffer(ib->buffer, GetIndexType(indexFormat), 0);
	buffer->dev->dxctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

static RG_INLINE void BindResourceView(RBindResourceViewInfo* info) {
	if(info->target == RG_PIPELINE_TYPE_COMPUTE) {
		// Compute pipeline
		if (info->type == R_DX_RESOURCEVIEW_SRV) {
			// Bind SRV
			info->rv->dev->dxctx->CSSetShaderResources(info->slot, 1, &info->rv->srv);
		} else if (info->type == R_DX_RESOURCEVIEW_UAV) {
			// Bind UAV
			info->rv->dev->dxctx->CSSetUnorderedAccessViews(info->slot, 1, &info->rv->uav, NULL);
		}
	} else {
		// Graphics pipeline
		if (info->type == R_DX_RESOURCEVIEW_SRV) {
			// Bind SRV
			info->rv->dev->dxctx->PSSetShaderResources(info->slot, 1, &info->rv->srv);
		} else if (info->type == R_DX_RESOURCEVIEW_UAV) {
			rgLogError(RG_LOG_RENDER, "R_DX11: Binding UAV resource views to graphics pipeline is not implemented yet.");
#if 0
			// Bind UAV
			info->rv->dev->dxctx->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, info->slot, 1, &info->rv->uav, NULL);
#endif
		}
	}
}

static RG_INLINE void CMD_BindResourceViewsImpl(RCommandBuffer* buffer, RCommand* cmd) {
	Uint32 count = cmd->_off0;
	RBindResourceViewInfo* infos = (RBindResourceViewInfo*)cmd->buffer;
	for (Uint32 i = 0; i < count; i++) {
		BindResourceView(&infos[i]);
	}
}

static RG_INLINE void CMD_PushConstants(RCommandBuffer* buffer, RCommand* cmd) {
	void* data = cmd->buffer;
	//Uint32 size = cmd->_off1; // Size stored in unused field
	Uint16 stage = cmd->_off0;
	Uint32 size  = cmd->_off1;

	// For DX11, we can use constant buffers. Here we would ideally have a pre-allocated constant buffer to update.
	ID3D11Buffer* d3d11buffer = NULL;
	Uint8 access = 0;
	if (stage == RG_SHADER_TYPE_VERTEX) {
		d3d11buffer = buffer->dev->pc_vertex->buffer;
		access = buffer->dev->pc_vertex->access;
	}
	else if (stage == RG_SHADER_TYPE_PIXEL) {
		d3d11buffer = buffer->dev->pc_pixel->buffer;
		access = buffer->dev->pc_pixel->access;
	}

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

	// Bind constant buffer
	// !! USED SLOT 0 FOR PUSH CONSTANTS !!
	if (stage == RG_SHADER_TYPE_VERTEX) {
		buffer->dev->dxctx->VSSetConstantBuffers(0, 1, &d3d11buffer);
	}
	else if (stage == RG_SHADER_TYPE_PIXEL) {
		buffer->dev->dxctx->PSSetConstantBuffers(0, 1, &d3d11buffer);
	}

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
}

static RG_INLINE void CMD_DispatchImpl(RCommandBuffer* buffer, RCommand* cmd) {
	Uint32 groupcount_x = cmd->data0;
	Uint32 groupcount_y = cmd->data1;
	Uint32 groupcount_z = cmd->data2;
	buffer->dev->dxctx->Dispatch(groupcount_x, groupcount_y, groupcount_z);
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
			case R_CMD_BIND_RESOURCEVIEWS: { CMD_BindResourceViewsImpl(buffer, cmd); break; }
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
	return cmd;
}

void R_CmdBeginRenderpass(RCommandBuffer* cmdbuff, RRenderpassBeginInfo* info) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd    = R_CMD_BEGIN_RENDERPASS;
	cmd->handle = info->renderpass;
	cmd->_off0  = 0;
	if (info->clearinfo) {
		// WARN: Memory layout dependency
		cmd->_off0 = 1;
		SDL_memcpy(&cmd->buffer, info->clearinfo, sizeof(RRenderpassClearInfo));
	}
}

void R_CmdEndRenderpass(RCommandBuffer* cmdbuff) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd = R_CMD_END_RENDERPASS;
}

void R_CmdBindPipeline(RCommandBuffer* cmdbuff, RPipeline* pl) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd    = R_CMD_BIND_PIPELINE;
	cmd->handle = pl;
}

void R_CmdBindVertexBuffer(RCommandBuffer* cmdbuff, RBuffer* vb, Uint32 slot, Uint32 stride) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd    = R_CMD_BIND_VERTEX_BUFFER;
	cmd->handle = vb;
	cmd->data0  = slot;   // Bind slot
	cmd->data1  = stride; // Stride
	cmd->data2  = 0;      // Offset
}

void R_CmdBindIndexBuffer(RCommandBuffer* cmdbuff, RBuffer* ib, IndexType isize) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd    = R_CMD_BIND_INDEX_BUFFER;
	cmd->handle = ib;
	cmd->data0  = isize;
}

void R_CmdBindResourceViews(RCommandBuffer* cmdbuff, Uint32 count, RBindResourceViewInfo* views) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd = R_CMD_BIND_RESOURCEVIEWS;
	if (count >= 8) {
		rgLogError(RG_LOG_RENDER, "DX11 Renderer: R_CmdBindResourceViews supports max 8 resource views!");
	}
	cmd->_off0 = count; // Store count in unused field
	SDL_memcpy(cmd->buffer, views, sizeof(RBindResourceViewInfo) * count);
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
	cmd->cmd = R_CMD_DRAW_INDEXED;
	cmd->data0 = idxcount;
	cmd->data1 = idxstart;
}

void R_CmdPushConstants(RCommandBuffer* cmdbuff, void* buffer, Uint32 size, Uint32 stage) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd = R_CMD_PUSHCONSTANTS;
	cmd->_off0 = stage; // Use "unused" field to store pipeline stage
	cmd->_off1 = size;  // Use "unused" field to store size
	SDL_memcpy(cmd->buffer, buffer, SDL_min(size, 128)); // Copy max 128 bytes
}

void R_CmdImGuiRenderDrawData(RCommandBuffer* cmdbuff, void* data) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd = R_CMD_DRAW_IMGUI;

	Uint64 address = (Uint64)data;
	cmd->data0 = (Uint32)(address >> 32); // High part
	cmd->data1 = (Uint32)(address & 0xFFFFFFFF); // Low part
}

void R_CmdDispatch(RCommandBuffer* cmdbuff, Uint32 groupcount_x, Uint32 groupcount_y, Uint32 groupcount_z) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd = R_CMD_DISPATCH;
	cmd->data0 = groupcount_x;
	cmd->data1 = groupcount_y;
	cmd->data2 = groupcount_z;
}