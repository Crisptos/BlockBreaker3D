#include "Engine.h"
#include <iostream>
#include <vector>

namespace BB3D
{
	Mesh CreateMesh(SDL_GPUDevice* device, std::vector<Vertex> vertices, std::vector<Uint16> indices)
	{
		// upload vertex data to vbo
		//	- create transfer buffer
		//	- map transfer buffer mem and copy from cpu
		//	- begin copy pass
		//	- invoke upload command
		//	- end copy pass and submit

		Mesh new_mesh = {};

		Uint32 vbo_size = sizeof(Vertex) * vertices.size();
		Uint32 ibo_size = sizeof(Uint16) * indices.size();

		SDL_GPUBufferCreateInfo vbo_info = {};
		vbo_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		vbo_info.size = vbo_size;
		new_mesh.vbo = SDL_CreateGPUBuffer(device, &vbo_info);

		SDL_GPUBufferCreateInfo ibo_info = {};
		ibo_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
		ibo_info.size = ibo_size;
		new_mesh.ibo = SDL_CreateGPUBuffer(device, &ibo_info);

		SDL_GPUTransferBufferCreateInfo mesh_transfer_create_info = {};
		mesh_transfer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		mesh_transfer_create_info.size = vbo_size + ibo_size;
		SDL_GPUTransferBuffer* mesh_trans_buff = SDL_CreateGPUTransferBuffer(device, &mesh_transfer_create_info);

		void* mesh_trans_ptr = SDL_MapGPUTransferBuffer(device, mesh_trans_buff, false);

		//  Vertices|Indices
		// |------->|
		std::memcpy(mesh_trans_ptr, vertices.data(), vbo_size);
		std::memcpy(reinterpret_cast<Vertex*>(mesh_trans_ptr) + vertices.size(), indices.data(), ibo_size);

		SDL_UnmapGPUTransferBuffer(device, mesh_trans_buff);

		SDL_GPUCommandBuffer* mesh_copy_cmd_buff = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(mesh_copy_cmd_buff);

		SDL_GPUTransferBufferLocation mesh_trans_location = {};
		mesh_trans_location.transfer_buffer = mesh_trans_buff;
		mesh_trans_location.offset = 0;
		SDL_GPUBufferRegion vbo_region = {};
		vbo_region.buffer = new_mesh.vbo;
		vbo_region.offset = 0;
		vbo_region.size = vbo_size;
		SDL_GPUBufferRegion ibo_region = {};
		ibo_region.buffer = new_mesh.ibo;
		ibo_region.offset = 0;
		ibo_region.size = ibo_size;

		SDL_UploadToGPUBuffer(copy_pass, &mesh_trans_location, &vbo_region, false);
		mesh_trans_location.offset = vbo_size;
		SDL_UploadToGPUBuffer(copy_pass, &mesh_trans_location, &ibo_region, false);

		SDL_EndGPUCopyPass(copy_pass);
		SDL_ReleaseGPUTransferBuffer(device, mesh_trans_buff);
		if (!SDL_SubmitGPUCommandBuffer(mesh_copy_cmd_buff))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to submit copy command buffer to GPU: %s\n", SDL_GetError());
			std::abort();
		}

		return new_mesh;
	}

}