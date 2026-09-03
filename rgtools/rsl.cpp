#define _CRT_SECURE_NO_WARNINGS
#include "rsl.h"

#include <stdio.h>

#include <spirv_cross/spirv_hlsl.hpp>
#include <d3dcompiler.h>

#define DX_RESOURCE_TYPE_PUSHCONSTANT_BINDING 0xFF // For runtime mapping

#define DX_RESOURCE_TYPE_PUSHCONSTANT   0x00 // b (255)
#define DX_RESOURCE_TYPE_TEXTURE        0x01 // t
#define DX_RESOURCE_TYPE_SAMPLER        0x02 // s
#define DX_RESOURCE_TYPE_CONSTANTBUFFER 0x03 // b
#define DX_RESOURCE_TYPE_STORAGEBUFFER  0x04 // t/u (SRV/UAV)

struct MappingEntry {
	uint8_t set;     // Native Vulkan set
	uint8_t binding; // Native Vulkan binding
	uint8_t reg;     // Resource register
	uint8_t slot;    // Resource slot
};

void rsl_build(String in, String out) {


	printf("Reading SPV\n");
	// Read SPV
	FILE* spv = fopen(in, "rb");
	if (!spv) {
		printf("No input file!\n");
		return;
	}
	fseek(spv, 0, SEEK_END);
	size_t len = ftell(spv);
	fseek(spv, 0, SEEK_SET);
	void* buffer = malloc(len);
	if (!buffer) {
		printf("Out of memory!\n");
		return;
	}
	fread(buffer, 1, len, spv);
	fclose(spv);
	
	if (len % 4 != 0) {
		printf("Invalid SPIR-V! (size: %d)\n", len);
		return;
	}

	// Parse SPV

	printf("Process SPV\n");
	spirv_cross::CompilerHLSL compiler((uint32_t*)buffer, len / 4);
	spirv_cross::CompilerHLSL::Options options;
	options.shader_model = 50;
	compiler.set_hlsl_options(options);

	spirv_cross::ShaderResources resources = compiler.get_shader_resources();

	uint32_t next_t_slot = 0; // SRV (texture, lim ~128)
	uint32_t next_s_slot = 0; // SS  (sampler, lim ~16)
	uint32_t next_u_slot = 0; // UAV (r/w buffer, lim ~8)
	uint32_t next_b_slot = 0; // CB  (constant buffer, lim ~14)
	std::vector<MappingEntry> mapping_table;


	printf("Remapping resources\n");
	auto RemapResource = [&](uint32_t res_id, uint32_t type, uint32_t set, uint32_t binding) {
		spirv_cross::HLSLResourceBinding hlsl_binding;
		hlsl_binding.stage = compiler.get_execution_model();
		hlsl_binding.desc_set = set;
		hlsl_binding.binding = binding;
		uint32_t assigned_slot = 0;

		switch (type) {
			case DX_RESOURCE_TYPE_CONSTANTBUFFER: { // b
				assigned_slot = next_b_slot++;
				hlsl_binding.cbv.register_space = 0;
				hlsl_binding.cbv.register_binding = assigned_slot;
				compiler.add_hlsl_resource_binding(hlsl_binding);
				break;
			}
			case DX_RESOURCE_TYPE_PUSHCONSTANT: { // b
				assigned_slot = next_b_slot++;
				hlsl_binding.cbv.register_space = 0;
				hlsl_binding.cbv.register_binding = assigned_slot;

				spirv_cross::RootConstants root_constant;
				root_constant.start   = 0;
				root_constant.end     = 256; // In Vulkan 128 bytes guaranteed, but in d3d11 this doesn't matter, since we will use a cbuffer to map it anyway.
				root_constant.binding = assigned_slot;
				root_constant.space   = 0;
				compiler.set_root_constant_layouts({ root_constant });

				compiler.add_hlsl_resource_binding(hlsl_binding);
				break;
			}
			case DX_RESOURCE_TYPE_TEXTURE: { // t
				assigned_slot = next_t_slot++;
				hlsl_binding.srv.register_space = 0;
				hlsl_binding.srv.register_binding = assigned_slot;
				compiler.add_hlsl_resource_binding(hlsl_binding);
				break;
			}
			case DX_RESOURCE_TYPE_SAMPLER: { // s
				assigned_slot = next_s_slot++;
				hlsl_binding.sampler.register_space = 0;
				hlsl_binding.sampler.register_binding = assigned_slot;
				compiler.add_hlsl_resource_binding(hlsl_binding);
				break;
			}
			case DX_RESOURCE_TYPE_STORAGEBUFFER: { // t/u
				auto flags = compiler.get_buffer_block_flags(res_id);
				bool is_readonly = flags.get(spv::DecorationNonWritable);

				if (is_readonly) { // t
					assigned_slot = next_t_slot++;
					hlsl_binding.srv.register_space = 0;
					hlsl_binding.srv.register_binding = assigned_slot;
				}
				else { // u
					assigned_slot = next_u_slot++;
					hlsl_binding.uav.register_space = 0;
					hlsl_binding.uav.register_binding = assigned_slot;
				}
				compiler.add_hlsl_resource_binding(hlsl_binding);
				break;
			}
		}

		MappingEntry entry = {};
		entry.set = set;
		entry.binding = binding;
		entry.reg = type;
		entry.slot = assigned_slot;
		mapping_table.push_back(entry);
	};

	// Uniform buffers
	for (const auto& res : resources.uniform_buffers) {
		uint32_t set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);
		RemapResource(res.id, DX_RESOURCE_TYPE_CONSTANTBUFFER, set, binding); // b
	}

	// Push constant
	// Loop? Remove this?
	//for (const auto& res : resources.push_constant_buffers) {
	//	RemapResource(res.id, RESOURCE_PUSHCONSTANT, 0, RESOURCE_PUSHCONSTANT_BINDING); // b (next empty slot)
	//}
	RemapResource(resources.push_constant_buffers[0].id, DX_RESOURCE_TYPE_PUSHCONSTANT, 0, DX_RESOURCE_TYPE_PUSHCONSTANT_BINDING); // b (next empty slot)

	// Textures
	for (const auto& res : resources.separate_images) {
		uint32_t set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);
		RemapResource(res.id, DX_RESOURCE_TYPE_TEXTURE, set, binding); // t
	}

	// Samplers
	for (const auto& res : resources.separate_samplers) {
		uint32_t set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);
		RemapResource(res.id, DX_RESOURCE_TYPE_SAMPLER, set, binding); // s
	}

	// Storage buffers
	for (const auto& res : resources.storage_buffers) {
		uint32_t set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);
		RemapResource(res.id, DX_RESOURCE_TYPE_STORAGEBUFFER, set, binding); // t/u
	}

	std::string s = compiler.compile();


	printf("Saving mappings\n");
	// Save HLSL source
	char _out[512];
	SDL_snprintf(_out, 512, "%s.hlsl", out);
	FILE* res = fopen(_out, "w");
	if (!res) {
		printf("No hlsl generated!\n");
		return;
	}
	fwrite(s.c_str(), 1, s.size(), res);
	fclose(res);

	// Save mappings
	SDL_snprintf(_out, 512, "%s.map", out);
	FILE* map = fopen(_out, "wb");
	if (!map) {
		printf("No mapping generated!\n");
		return;
	}
	for (size_t i = 0; i < mapping_table.size(); i++) {
		fwrite(&mapping_table[i], sizeof(MappingEntry), 1, map);
	}
	fclose(map);


	printf("Compile DXBC\n");

	// Compile HLSL
	ID3DBlob* shader_blob = NULL;
	ID3DBlob* error_blob  = NULL;

	spv::ExecutionModel model = compiler.get_execution_model();
	String target = "NULL";
	if (model == spv::ExecutionModelVertex) {
		target = "vs_5_0";
	} else if (model == spv::ExecutionModelGeometry) {
		target = "gs_5_0";
	} else if (model == spv::ExecutionModelFragment) {
		target = "ps_5_0";
	} else if (model == spv::ExecutionModelGLCompute) {
		target = "cs_5_0";
	}

	HRESULT hr = D3DCompile(
		s.c_str(), s.size(),
		NULL, NULL, NULL, "main",
		target,
		D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
		&shader_blob, &error_blob
	);
	if (FAILED(hr)) {
		if (error_blob) {
			printf("D3DCompile: %s\n", (const char*)error_blob->GetBufferPointer());
			error_blob->Release();
		}
		printf("No DXBC generated!\n");
		return;
	}

	printf("Saving DXBC (%lu)\n", shader_blob->GetBufferSize());

	// Save binary blob
	FILE* dxbc = fopen(out, "wb");
	if (!dxbc) {
		printf("No DXBC generated!\n");
		return;
	}
	fwrite(shader_blob->GetBufferPointer(), 1, shader_blob->GetBufferSize(), dxbc);
	fclose(dxbc);
	shader_blob->Release();

}