#include "Engine.h"
#include <iostream>
#include <stb_image.h>

namespace BB3D
{
	SDL_GPUTexture* CreateAndLoadTextureToGPU(SDL_GPUDevice* device, const char* filepath)
	{
		// Load texture
		//  Load the pixels
		//  Create texture on the GPU
		//  Upload pixels to GPU texture
		//  Assign texture UV coordinates to vertices
		//  Create sampler
		//  Sample using UV in fragment shader

		SDL_GPUTexture* new_texture = {};
		Image loaded_img = {};

		stbi_set_flip_vertically_on_load(true);
		unsigned char* image_data = stbi_load(filepath, &loaded_img.x, &loaded_img.y, &loaded_img.channels, 4);
		if (!image_data)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load texture file from: %s\n", filepath);
			std::abort();
		}

		SDL_GPUTextureCreateInfo tex_info = {};
		tex_info.type = SDL_GPU_TEXTURETYPE_2D;
		tex_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
		tex_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
		tex_info.width = loaded_img.x;
		tex_info.height = loaded_img.y;
		tex_info.layer_count_or_depth = 1;
		tex_info.num_levels = 1;
		new_texture = SDL_CreateGPUTexture(device, &tex_info);
		if (!new_texture)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create GPU texture: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_GPUTransferBufferCreateInfo tex_transfer_create_info = {};
		tex_transfer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		tex_transfer_create_info.size = 4 * (loaded_img.x * loaded_img.y);
		SDL_GPUTransferBuffer* tex_trans_buff = SDL_CreateGPUTransferBuffer(device, &tex_transfer_create_info);
		if (!tex_trans_buff)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create transfer buffer for File -> GPU Texture: %s\n", SDL_GetError());
			std::abort();
		}

		void* tex_trans_ptr = SDL_MapGPUTransferBuffer(device, tex_trans_buff, false);
		if (!tex_trans_ptr)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to map transfer buffer for File -> GPU Texture: %s\n", SDL_GetError());
			std::abort();
		}

		std::memcpy(tex_trans_ptr, image_data, 4 * (loaded_img.x * loaded_img.y));
		SDL_UnmapGPUTransferBuffer(device, tex_trans_buff);

		SDL_GPUCommandBuffer* tex_copy_cmd_buff = SDL_AcquireGPUCommandBuffer(device);
		if (!tex_copy_cmd_buff)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to acquire command buffer for copying to GPU texture: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_GPUCopyPass* tex_copy_pass = SDL_BeginGPUCopyPass(tex_copy_cmd_buff);
		if (!tex_copy_pass)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to begin copy pass: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_GPUTextureTransferInfo tex_trans_info = {};
		tex_trans_info.offset = 0;
		tex_trans_info.transfer_buffer = tex_trans_buff;
		SDL_GPUTextureRegion tex_trans_region = {};
		tex_trans_region.texture = new_texture;
		tex_trans_region.w = loaded_img.x;
		tex_trans_region.h = loaded_img.y;
		tex_trans_region.d = 1;
		SDL_UploadToGPUTexture(tex_copy_pass, &tex_trans_info, &tex_trans_region, false);
		SDL_EndGPUCopyPass(tex_copy_pass);
		if (!SDL_SubmitGPUCommandBuffer(tex_copy_cmd_buff))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to submit copy command buffer to GPU Texture: %s\n", SDL_GetError());
			std::abort();
		}
		stbi_image_free(image_data);
		SDL_ReleaseGPUTransferBuffer(device, tex_trans_buff);

		return new_texture;
	}

	SDL_GPUSampler* CreateSampler(SDL_GPUDevice* device, SDL_GPUFilter texture_filter)
	{
		SDL_GPUSampler* new_sampler = {};
		SDL_GPUSamplerCreateInfo sampler_info = {};
		sampler_info.min_filter = texture_filter;
		sampler_info.mag_filter = texture_filter;
		new_sampler = SDL_CreateGPUSampler(device, &sampler_info);

		return new_sampler;
	}

}