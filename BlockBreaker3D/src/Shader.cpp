#include "Engine.h"
#include <iostream>

namespace BB3D
{
	SDL_GPUShader* CreateShaderFromFile(
		SDL_GPUDevice* device,
		const char* file_path,
		SDL_GPUShaderStage shader_stage,
		Uint32 sampler_count,
		Uint32 uniform_buffer_count,
		Uint32 storage_buffer_count,
		Uint32 storage_texture_count
	)
	{
		SDL_GPUShader* new_shader;

		SDL_GPUShaderFormat supported_formats = SDL_GetGPUShaderFormats(device);
		if (!(supported_formats & SDL_GPU_SHADERFORMAT_SPIRV))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Device context does not support the target shader format (SPIR-V)");
			std::abort();
		}

		size_t source_size = 0;
		void* shader_source = SDL_LoadFile(file_path, &source_size);
		if (!shader_source)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to locate file at: %s\n", file_path);
			std::abort();
		}

		SDL_GPUShaderCreateInfo shader_create_info = {};
		shader_create_info.code = static_cast<Uint8*>(shader_source);
		shader_create_info.code_size = source_size;
		shader_create_info.entrypoint = "main";
		shader_create_info.format = supported_formats;
		shader_create_info.stage = shader_stage;
		shader_create_info.num_samplers = sampler_count;
		shader_create_info.num_storage_textures = storage_texture_count;
		shader_create_info.num_storage_buffers = storage_buffer_count;
		shader_create_info.num_uniform_buffers = uniform_buffer_count;

		new_shader = SDL_CreateGPUShader(
			device,
			&shader_create_info
		);
		if (!new_shader)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create GPU shader: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_free(shader_source);

		return new_shader;
	}

}