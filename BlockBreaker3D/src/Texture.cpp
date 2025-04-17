#include "Engine.h"
#include <iostream>
#include <stb_image.h>

namespace BB3D
{
	SDL_GPUTexture* CreateAndLoadCubeMapToGPU(SDL_GPUDevice* device, std::array<std::string, 6> filepaths)
	{
		SDL_GPUTexture* new_cubemap_texture = {};

		unsigned char* cube_faces_img_data[6];
		Image cube_faces_img_props = {};

		// TODO - Enforce cubemap props are the same per face
		for (int i = 0; i < 6; i++)
		{
			stbi_set_flip_vertically_on_load(false);
			cube_faces_img_data[i] = stbi_load(
				filepaths[i].c_str(), 
				&cube_faces_img_props.x, 
				&cube_faces_img_props.y,
				&cube_faces_img_props.channels,
				4
			);

			if (!cube_faces_img_data[i])
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load cubemap texture face file from: %s | idx = %d\n", filepaths[i].c_str(), i);
				std::abort();
			}
		}

		SDL_GPUTextureCreateInfo cubemap_info = {};
		cubemap_info.type = SDL_GPU_TEXTURETYPE_CUBE;
		cubemap_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
		cubemap_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
		cubemap_info.width = cube_faces_img_props.x;
		cubemap_info.height = cube_faces_img_props.y;
		cubemap_info.layer_count_or_depth = 6;
		cubemap_info.num_levels = 1;

		new_cubemap_texture = SDL_CreateGPUTexture(device, &cubemap_info);
		if (!new_cubemap_texture)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create GPU cubemap texture: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_GPUTransferBufferCreateInfo cubemap_transfer_create_info = {};
		cubemap_transfer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		cubemap_transfer_create_info.size = (4 * (cube_faces_img_props.x * cube_faces_img_props.y)); // face size in bytes
		SDL_GPUTransferBuffer* cubemap_trans_buff = SDL_CreateGPUTransferBuffer(device, &cubemap_transfer_create_info);
		if (!cubemap_trans_buff)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create transfer buffer for File -> GPU Texture: %s\n", SDL_GetError());
			std::abort();
		}

		for (int i = 0; i < 6; i++)
		{
			SDL_GPUCommandBuffer* cubemap_copy_cmd_buff = SDL_AcquireGPUCommandBuffer(device);
			if (!cubemap_copy_cmd_buff)
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to acquire command buffer for copying to GPU texture: %s\n", SDL_GetError());
				std::abort();
			}

			SDL_GPUCopyPass* cubemap_copy_pass = SDL_BeginGPUCopyPass(cubemap_copy_cmd_buff);
			if (!cubemap_copy_pass)
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to begin copy pass: %s\n", SDL_GetError());
				std::abort();
			}

			void* cubemap_trans_ptr = SDL_MapGPUTransferBuffer(device, cubemap_trans_buff, false);
			if (!cubemap_trans_ptr)
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to map transfer buffer for File -> GPU Texture: %s\n", SDL_GetError());
				std::abort();
			}

			std::memcpy(cubemap_trans_ptr, cube_faces_img_data[i], (4 * (cube_faces_img_props.x * cube_faces_img_props.y)));

			SDL_UnmapGPUTransferBuffer(device, cubemap_trans_buff);

			SDL_GPUTextureTransferInfo cubemap_trans_info = {};
			cubemap_trans_info.offset = 0;
			cubemap_trans_info.transfer_buffer = cubemap_trans_buff;
			cubemap_trans_info.pixels_per_row = cube_faces_img_props.x;
			//cubemap_trans_info.rows_per_layer = 1;
			SDL_GPUTextureRegion cubemap_trans_region = {};
			cubemap_trans_region.texture = new_cubemap_texture;
			cubemap_trans_region.w = cube_faces_img_props.x;
			cubemap_trans_region.h = cube_faces_img_props.y;
			cubemap_trans_region.d = 1;
			cubemap_trans_region.layer = SDL_GPU_CUBEMAPFACE_POSITIVEX + i;
			SDL_UploadToGPUTexture(cubemap_copy_pass, &cubemap_trans_info, &cubemap_trans_region, false);

			SDL_EndGPUCopyPass(cubemap_copy_pass);
			SDL_GPUFence* upload_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cubemap_copy_cmd_buff);
			if (!upload_fence)
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to submit copy command buffer to GPU Texture and acquire fence: %s\n", SDL_GetError());
				std::abort();
			}

			SDL_WaitForGPUFences(device, true, &upload_fence, 1);
			SDL_ReleaseGPUFence(device, upload_fence);
		}

		// Free stbi image data
		for (int i = 0; i < 6; i++)
		{
			stbi_image_free(cube_faces_img_data[i]);
		}

		SDL_ReleaseGPUTransferBuffer(device, cubemap_trans_buff);


		return new_cubemap_texture;
	}

	SDL_GPUTexture* CreateDepthTestTexture(SDL_GPUDevice* device, int render_target_w, int render_target_h)
	{
		SDL_GPUTexture* new_depth_texture = {};

		SDL_GPUTextureCreateInfo tex_info = {};
		tex_info.type = SDL_GPU_TEXTURETYPE_2D;
		tex_info.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
		tex_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
		tex_info.width = render_target_w;
		tex_info.height = render_target_h;
		tex_info.layer_count_or_depth = 1;
		tex_info.num_levels = 1;
		new_depth_texture = SDL_CreateGPUTexture(device, &tex_info);
		if (!new_depth_texture)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create GPU depth test texture: %s\n", SDL_GetError());
			std::abort();
		}

		return new_depth_texture;
	}

	SDL_GPUTexture* CreateAndLoadTextureToGPU(SDL_GPUDevice* device, const char* filepath)
	{
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